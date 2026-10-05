#include "MySessionSubsystem.h"

#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Interfaces/OnlineIdentityInterface.h"

#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "Misc/CoreDelegates.h"


void UMySessionSubsystem::Initialize(
	FSubsystemCollectionBase& Collection
)
{
	Super::Initialize(Collection);

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();

	if (!OnlineSubsystem)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("MySessionSubsystem: OnlineSubsystem not found")
		);

		return;
	}

	SessionInterface = OnlineSubsystem->GetSessionInterface();

	if (SessionInterface.IsValid())
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("MySessionSubsystem: Session Interface initialized")
		);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("MySessionSubsystem: Session Interface is invalid")
		);
	}

	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(
			this,
			&UMySessionSubsystem::HandleNetworkFailure
		);
	}

	FCoreDelegates::OnPreExit.AddUObject(
		this,
		&UMySessionSubsystem::HandlePreExit
	);
}

void UMySessionSubsystem::Deinitialize()
{
	bShuttingDown = true;

	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
	}

	FCoreDelegates::OnPreExit.RemoveAll(this);
	
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(
			CreateSessionCompleteHandle
		);

		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(
			FindSessionsCompleteHandle
		);

		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(
			JoinSessionCompleteHandle
		);

		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(
			DestroySessionCompleteHandle
		);

		if (SessionInterface->GetNamedSession(NAME_GameSession))
		{
			SessionInterface->DestroySession(NAME_GameSession);
		}
	}

	SessionInterface.Reset();

	Super::Deinitialize();
}

void UMySessionSubsystem::CreateLobby(
    const FString& LobbyName,
    ESessionPrivacy Privacy
)
{
    if (!SessionInterface.IsValid())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("CreateLobby failed: Session Interface is invalid.")
        );

        OnSessionCreated.Broadcast(false);
        return;
    }

    if (LobbyName.IsEmpty())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("CreateLobby failed: Lobby name is empty.")
        );

        OnSessionCreated.Broadcast(false);
        return;
    }

	if (SessionInterface->GetNamedSession(NAME_GameSession) != nullptr)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("CreateLobby: Existing session found. Destroying it before creating a new one.")
		);

		if (bIsDestroyingSession)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("CreateLobby: Session is already being destroyed.")
			);

			OnSessionCreated.Broadcast(false);
			return;
		}

		bCreateAfterDestroy = true;
		PendingLobbyName = LobbyName;
		PendingPrivacy = Privacy;

		bIsDestroyingSession = true;

		DestroySessionCompleteHandle =
			SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
				FOnDestroySessionCompleteDelegate::CreateUObject(
					this,
					&UMySessionSubsystem::OnDestroySessionComplete
				)
			);

		const bool bStarted =
			SessionInterface->DestroySession(NAME_GameSession);

		if (!bStarted)
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("CreateLobby: Failed to start cleanup of existing session.")
			);

			SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(
				DestroySessionCompleteHandle
			);

			bIsDestroyingSession = false;
			bCreateAfterDestroy = false;
			PendingLobbyName.Empty();

			OnSessionCreated.Broadcast(false);
		}

		return;
	}

    IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();

    if (!OnlineSubsystem)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("CreateLobby failed: OnlineSubsystem not found.")
        );

        OnSessionCreated.Broadcast(false);
        return;
    }

    IOnlineIdentityPtr IdentityInterface =
        OnlineSubsystem->GetIdentityInterface();

    if (!IdentityInterface.IsValid())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("CreateLobby failed: Identity Interface is invalid.")
        );

        OnSessionCreated.Broadcast(false);
        return;
    }

    FUniqueNetIdPtr UserId =
        IdentityInterface->GetUniquePlayerId(0);

    if (!UserId.IsValid())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("CreateLobby failed: Unique user ID is invalid.")
        );

        OnSessionCreated.Broadcast(false);
        return;
    }

    TSharedPtr<FOnlineSessionSettings> SessionSettings =
        MakeShared<FOnlineSessionSettings>();

    SessionSettings->NumPublicConnections = 2;
    SessionSettings->NumPrivateConnections = 0;

    SessionSettings->bIsLANMatch = false;
    SessionSettings->bShouldAdvertise = true;

    SessionSettings->bAllowJoinInProgress = true;
    SessionSettings->bAllowInvites = true;

    SessionSettings->bUsesPresence = true;
    SessionSettings->bUseLobbiesIfAvailable = true;

    if (Privacy == ESessionPrivacy::Public)
    {
        SessionSettings->bAllowJoinViaPresence = true;
        SessionSettings->bAllowJoinViaPresenceFriendsOnly = false;
    }
    else
    {
        SessionSettings->bAllowJoinViaPresence = false;
        SessionSettings->bAllowJoinViaPresenceFriendsOnly = true;
    }

    SessionSettings->Set(
        FName(TEXT("LOBBY_NAME")),
        LobbyName,
        EOnlineDataAdvertisementType::ViaOnlineServiceAndPing
    );

    CreateSessionCompleteHandle =
        SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
            FOnCreateSessionCompleteDelegate::CreateUObject(
                this,
                &UMySessionSubsystem::OnCreateSessionComplete
            )
        );

    UE_LOG(
        LogTemp,
        Log,
        TEXT("Creating Steam lobby: %s"),
        *LobbyName
    );

    const bool bStarted =
        SessionInterface->CreateSession(
            *UserId,
            NAME_GameSession,
            *SessionSettings
        );

    if (!bStarted)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("CreateSession failed to start.")
        );

        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(
            CreateSessionCompleteHandle
        );

        OnSessionCreated.Broadcast(false);
    }
}

void UMySessionSubsystem::OnCreateSessionComplete(
	FName SessionName,
	bool bWasSuccessful
)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(
			CreateSessionCompleteHandle
		);
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("OnCreateSessionComplete: %s / Success: %s"),
		*SessionName.ToString(),
		bWasSuccessful ? TEXT("true") : TEXT("false")
	);

	if (!bWasSuccessful)
	{
		OnSessionCreated.Broadcast(false);
		return;
	}

	OnSessionCreated.Broadcast(true);

	UWorld* World = GetWorld();

	if (!World)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("CreateSession succeeded, but World is invalid.")
		);

		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Travelling to lobby as listen server.")
	);

	World->ServerTravel(TEXT("/Game/TCG_Main/Maps/L_Lobby?listen"));
}

void UMySessionSubsystem::FindLobbies()
{
    if (!SessionInterface.IsValid())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("FindLobbies failed: Session Interface is invalid.")
        );

        AvailableSessions.Empty();
        OnSessionsFound.Broadcast();

        return;
    }


    IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();

    if (!OnlineSubsystem)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("FindLobbies failed: OnlineSubsystem not found.")
        );

        AvailableSessions.Empty();
        OnSessionsFound.Broadcast();

        return;
    }


    IOnlineIdentityPtr IdentityInterface =
        OnlineSubsystem->GetIdentityInterface();

    if (!IdentityInterface.IsValid())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("FindLobbies failed: Identity Interface is invalid.")
        );

        AvailableSessions.Empty();
        OnSessionsFound.Broadcast();

        return;
    }


    FUniqueNetIdPtr UserId =
        IdentityInterface->GetUniquePlayerId(0);

    if (!UserId.IsValid())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("FindLobbies failed: Unique user ID is invalid.")
        );

        AvailableSessions.Empty();
        OnSessionsFound.Broadcast();

        return;
    }


    AvailableSessions.Empty();


    SessionSearch =
        MakeShared<FOnlineSessionSearch>();

    SessionSearch->bIsLanQuery = false;
    SessionSearch->MaxSearchResults = 50;

	SessionSearch->QuerySettings.Set(
		SEARCH_LOBBIES,
		true,
		EOnlineComparisonOp::Equals
	);


    FindSessionsCompleteHandle =
        SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
            FOnFindSessionsCompleteDelegate::CreateUObject(
                this,
                &UMySessionSubsystem::OnFindSessionsComplete
            )
        );


    UE_LOG(
        LogTemp,
        Log,
        TEXT("Searching for Steam lobbies...")
    );


    const bool bStarted =
        SessionInterface->FindSessions(
            *UserId,
            SessionSearch.ToSharedRef()
        );


    if (!bStarted)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("FindSessions failed to start.")
        );

        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(
            FindSessionsCompleteHandle
        );

        AvailableSessions.Empty();
        OnSessionsFound.Broadcast();
    }
}

void UMySessionSubsystem::OnFindSessionsComplete(
	bool bWasSuccessful
)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(
			FindSessionsCompleteHandle
		);
	}


	AvailableSessions.Empty();


	if (!bWasSuccessful || !SessionSearch.IsValid())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("FindSessions failed.")
		);

		OnSessionsFound.Broadcast();

		return;
	}


	UE_LOG(
		LogTemp,
		Log,
		TEXT("Found %d sessions."),
		SessionSearch->SearchResults.Num()
	);


	for (int32 Index = 0;
		 Index < SessionSearch->SearchResults.Num();
		 ++Index)
	{
		const FOnlineSessionSearchResult& Result =
			SessionSearch->SearchResults[Index];


		FString LobbyName;

		Result.Session.SessionSettings.Get(
			FName(TEXT("LOBBY_NAME")),
			LobbyName
		);


		FSessionInfo Info;

		Info.LobbyName =
			LobbyName.IsEmpty()
				? TEXT("Unnamed Lobby")
				: LobbyName;

		Info.MaxPlayers =
			Result.Session.SessionSettings.NumPublicConnections;

		Info.CurrentPlayers =
			Info.MaxPlayers -
			Result.Session.NumOpenPublicConnections;

		Info.SessionIndex = Index;


		AvailableSessions.Add(Info);


		UE_LOG(
			LogTemp,
			Log,
			TEXT(
				"Session %d: %s | Players: %d/%d"
			),
			Index,
			*Info.LobbyName,
			Info.CurrentPlayers,
			Info.MaxPlayers
		);
	}


	OnSessionsFound.Broadcast();
}

void UMySessionSubsystem::JoinLobby(
    int32 SessionIndex
)
{
    if (!SessionInterface.IsValid())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("JoinLobby failed: Session Interface is invalid.")
        );

        OnSessionJoined.Broadcast(false);

        return;
    }


    if (!SessionSearch.IsValid())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("JoinLobby failed: No search has been performed.")
        );

        OnSessionJoined.Broadcast(false);

        return;
    }


    if (!SessionSearch->SearchResults.IsValidIndex(SessionIndex))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT(
                "JoinLobby failed: Invalid session index %d."
            ),
            SessionIndex
        );

        OnSessionJoined.Broadcast(false);

        return;
    }


    IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();

    if (!OnlineSubsystem)
    {
        OnSessionJoined.Broadcast(false);
        return;
    }


    IOnlineIdentityPtr IdentityInterface =
        OnlineSubsystem->GetIdentityInterface();

    if (!IdentityInterface.IsValid())
    {
        OnSessionJoined.Broadcast(false);
        return;
    }


    FUniqueNetIdPtr UserId =
        IdentityInterface->GetUniquePlayerId(0);

    if (!UserId.IsValid())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("JoinLobby failed: User ID is invalid.")
        );

        OnSessionJoined.Broadcast(false);

        return;
    }


    JoinSessionCompleteHandle =
        SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
            FOnJoinSessionCompleteDelegate::CreateUObject(
                this,
                &UMySessionSubsystem::OnJoinSessionComplete
            )
        );


	if (AvailableSessions.IsValidIndex(SessionIndex))
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT(
				"Joining session %d: %s"
			),
			SessionIndex,
			*AvailableSessions[SessionIndex].LobbyName
		);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT(
				"Joining session %d"
			),
			SessionIndex
		);
	}


    const bool bStarted =
        SessionInterface->JoinSession(
            *UserId,
            NAME_GameSession,
            SessionSearch->SearchResults[SessionIndex]
        );


    if (!bStarted)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("JoinSession failed to start.")
        );

        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(
            JoinSessionCompleteHandle
        );

        OnSessionJoined.Broadcast(false);
    }
}

void UMySessionSubsystem::OnJoinSessionComplete(
	FName SessionName,
	EOnJoinSessionCompleteResult::Type Result
)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(
			JoinSessionCompleteHandle
		);
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"OnJoinSessionComplete result: %d"
		),
		static_cast<int32>(Result)
	);

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Join session failed.")
		);

		OnSessionJoined.Broadcast(false);
		return;
	}

	FString ConnectString;

	if (!SessionInterface->GetResolvedConnectString(
			SessionName,
			ConnectString
		))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"Join succeeded but could not resolve connect string."
			)
		);

		OnSessionJoined.Broadcast(false);
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();

	if (!GameInstance)
	{
		OnSessionJoined.Broadcast(false);
		return;
	}

	APlayerController* PlayerController =
		GameInstance->GetFirstLocalPlayerController();

	if (!PlayerController)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"Join succeeded but PlayerController is invalid."
			)
		);

		OnSessionJoined.Broadcast(false);
		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"Join successful. Travelling to host: %s"
		),
		*ConnectString
	);

	OnSessionJoined.Broadcast(true);

	PlayerController->ClientTravel(
		ConnectString,
		ETravelType::TRAVEL_Absolute
	);
}

void UMySessionSubsystem::DestroyLobby()
{
	if (!SessionInterface.IsValid())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("DestroyLobby: Session Interface is invalid.")
		);

		OnSessionDestroyed.Broadcast(false);
		return;
	}

	if (bIsDestroyingSession)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("DestroyLobby: Session destruction already in progress.")
		);

		return;
	}

	if (SessionInterface->GetNamedSession(NAME_GameSession) == nullptr)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("DestroyLobby: No active session. Treating cleanup as successful.")
		);

		OnSessionDestroyed.Broadcast(true);
		return;
	}

	bIsDestroyingSession = true;

	DestroySessionCompleteHandle =
		SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(
				this,
				&UMySessionSubsystem::OnDestroySessionComplete
			)
		);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Destroying current session.")
	);

	const bool bStarted =
		SessionInterface->DestroySession(NAME_GameSession);

	if (!bStarted)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("DestroyLobby: DestroySession failed to start.")
		);

		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(
			DestroySessionCompleteHandle
		);

		bIsDestroyingSession = false;

		OnSessionDestroyed.Broadcast(false);
	}
}

void UMySessionSubsystem::OnDestroySessionComplete(
	FName SessionName,
	bool bWasSuccessful
)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(
			DestroySessionCompleteHandle
		);
	}

	bIsDestroyingSession = false;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("OnDestroySessionComplete: %s / Success: %s"),
		*SessionName.ToString(),
		bWasSuccessful ? TEXT("true") : TEXT("false")
	);

	// We destroyed an old session specifically because the user
	// requested to host again.
	if (bCreateAfterDestroy)
	{
		const FString LobbyName = PendingLobbyName;
		const ESessionPrivacy Privacy = PendingPrivacy;

		bCreateAfterDestroy = false;
		PendingLobbyName.Empty();

		if (!bWasSuccessful)
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("Could not destroy old session before creating new lobby.")
			);

			OnSessionCreated.Broadcast(false);
			return;
		}

		UE_LOG(
			LogTemp,
			Log,
			TEXT("Old session destroyed. Creating new lobby: %s"),
			*LobbyName
		);

		CreateLobby(LobbyName, Privacy);
		return;
	}

	// Normal Leave / cleanup.
	OnSessionDestroyed.Broadcast(bWasSuccessful);
}

void UMySessionSubsystem::HandleNetworkFailure(
	UWorld* World,
	UNetDriver* NetDriver,
	ENetworkFailure::Type FailureType,
	const FString& ErrorString
)
{
	if (bShuttingDown)
	{
		return;
	}

	if (!World || !SessionInterface.IsValid())
	{
		return;
	}

	// We only want the CLIENT to react to losing its connection.
	// A host should not leave its own lobby because a client disconnected.
	if (NetDriver && NetDriver->IsServer())
	{
		return;
	}

	if (SessionInterface->GetNamedSession(NAME_GameSession) == nullptr)
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"Unexpected network failure. Type=%d Error=%s"
		),
		static_cast<int32>(FailureType),
		*ErrorString
	);

	if (bIsDestroyingSession)
	{
		return;
	}

	// Tell the local game immediately.
	OnUnexpectedDisconnect.Broadcast();

	// Clean up the local session state.
	DestroyLobby();
}

void UMySessionSubsystem::HandlePreExit()
{
	bShuttingDown = true;

	if (!SessionInterface.IsValid())
	{
		return;
	}

	if (SessionInterface->GetNamedSession(NAME_GameSession) == nullptr)
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Application exiting. Destroying local GameSession.")
	);

	SessionInterface->DestroySession(NAME_GameSession);
}