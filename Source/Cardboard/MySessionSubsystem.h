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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnSessionCreated,
	bool,
	bWasSuccessful
);

UCLASS()
class CARDBOARD_API UMySessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Sessions")
	void CreateLobby(
		const FString& LobbyName,
		ESessionPrivacy Privacy
	);

	UFUNCTION(BlueprintCallable, Category = "Sessions")
	void DestroyLobby();

	UPROPERTY(BlueprintAssignable, Category = "Sessions")
	FOnSessionCreated OnSessionCreated;

private:

	IOnlineSessionPtr SessionInterface;

	FDelegateHandle CreateSessionCompleteHandle;

	void OnCreateSessionComplete(
		FName SessionName,
		bool bWasSuccessful
	);
};