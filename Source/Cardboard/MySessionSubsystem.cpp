#include "MySessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineIdentityInterface.h"

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
}

void UMySessionSubsystem::Deinitialize()
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(
			CreateSessionCompleteHandle
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
            Warning,
            TEXT("CreateLobby failed: A session already exists.")
        );

        OnSessionCreated.Broadcast(false);
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

void UMySessionSubsystem::DestroyLobby()
{
	if (!SessionInterface.IsValid())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("DestroyLobby: Session Interface is invalid.")
		);

		return;
	}

	if (SessionInterface->GetNamedSession(NAME_GameSession) == nullptr)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("DestroyLobby: No active session.")
		);

		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Destroying current session.")
	);

	SessionInterface->DestroySession(NAME_GameSession);
}