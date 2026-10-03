#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MySessionSubsystem.generated.h"


UENUM(BlueprintType)
enum class ESessionPrivacy : uint8
{
    Public,
    FriendsOnly
};


USTRUCT(BlueprintType)
struct CARDBOARD_API FSessionInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Session")
    FString LobbyName;

    UPROPERTY(BlueprintReadOnly, Category = "Session")
    int32 CurrentPlayers = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Session")
    int32 MaxPlayers = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Session")
    int32 SessionIndex = INDEX_NONE;
};


// ---------------------------------------------------------
// SESSION DELEGATES
// ---------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnSessionCreated,
    bool,
    bWasSuccessful
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnSessionDestroyed,
    bool,
    bWasSuccessful
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(
    FOnSessionsFound
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnSessionJoined,
    bool,
    bWasSuccessful
);


// ---------------------------------------------------------
// SESSION SUBSYSTEM
// ---------------------------------------------------------

UCLASS()
class CARDBOARD_API UMySessionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;


    // -----------------------------------------------------
    // CREATE
    // -----------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Sessions")
    void CreateLobby(
        const FString& LobbyName,
        ESessionPrivacy Privacy
    );


    // -----------------------------------------------------
    // FIND
    // -----------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Sessions")
    void FindLobbies();


    // -----------------------------------------------------
    // JOIN
    // -----------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Sessions")
    void JoinLobby(
        int32 SessionIndex
    );


    // -----------------------------------------------------
    // DESTROY
    // -----------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Sessions")
    void DestroyLobby();


    // -----------------------------------------------------
    // RESULTS
    // -----------------------------------------------------

    UPROPERTY(
        BlueprintReadOnly,
        Category = "Sessions"
    )
    TArray<FSessionInfo> AvailableSessions;


    // -----------------------------------------------------
    // EVENTS
    // -----------------------------------------------------

    UPROPERTY(
        BlueprintAssignable,
        Category = "Sessions"
    )
    FOnSessionCreated OnSessionCreated;

    UPROPERTY(
        BlueprintAssignable,
        Category = "Sessions"
    )
    FOnSessionsFound OnSessionsFound;

    UPROPERTY(
        BlueprintAssignable,
        Category = "Sessions"
    )
    FOnSessionJoined OnSessionJoined;

    UPROPERTY(
        BlueprintAssignable,
        Category = "Sessions"
    )
    FOnSessionDestroyed OnSessionDestroyed;


private:

    // -----------------------------------------------------
    // ONLINE SESSION
    // -----------------------------------------------------

    IOnlineSessionPtr SessionInterface;

    TSharedPtr<FOnlineSessionSearch> SessionSearch;


    // -----------------------------------------------------
    // DELEGATE HANDLES
    // -----------------------------------------------------

    FDelegateHandle CreateSessionCompleteHandle;

    FDelegateHandle FindSessionsCompleteHandle;

    FDelegateHandle JoinSessionCompleteHandle;

    FDelegateHandle DestroySessionCompleteHandle;


    // -----------------------------------------------------
    // CALLBACKS
    // -----------------------------------------------------

    void OnCreateSessionComplete(
        FName SessionName,
        bool bWasSuccessful
    );

    void OnFindSessionsComplete(
        bool bWasSuccessful
    );

    void OnJoinSessionComplete(
        FName SessionName,
        EOnJoinSessionCompleteResult::Type Result
    );

    void OnDestroySessionComplete(
        FName SessionName,
        bool bWasSuccessful
    );
};