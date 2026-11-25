#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "LobbyPlayerState.generated.h"

UCLASS()
class CARDBOARD_API ALobbyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	UPROPERTY(ReplicatedUsing = OnRep_IsReady, BlueprintReadOnly, Category = "Lobby")
	bool bIsReady = false;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Lobby")
	void SetReady(bool bNewReady);

	UFUNCTION(BlueprintPure, Category = "Lobby")
	bool IsReady() const { return bIsReady; }

	// True for the player who hosts the session (the listen server's own player).
	// Set by the server, so every machine agrees on who the host is.
	UFUNCTION(BlueprintPure, Category = "Lobby")
	bool IsHost() const { return bIsHost; }

	// Server only, called by ALobbyGameState when the player joins.
	void SetIsHost(bool bNewIsHost);

protected:

	UPROPERTY(ReplicatedUsing = OnRep_IsReady)
	bool bIsHost = false;

	UFUNCTION()
	void OnRep_IsReady();

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;
};