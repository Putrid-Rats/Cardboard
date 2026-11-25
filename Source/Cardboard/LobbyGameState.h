#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LobbyGameState.generated.h"

class ALobbyPlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyPlayersChanged);

UCLASS()
class CARDBOARD_API ALobbyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void NotifyReadyStateChanged();

	// True when the lobby is full (2 players) and every player is ready.
	UFUNCTION(BlueprintPure, Category = "Lobby")
	bool AreAllPlayersReady() const;

	// The host's player state (left lobby slot), or null if not known yet.
	// Use these instead of PlayerArray order, which can differ between machines.
	UFUNCTION(BlueprintPure, Category = "Lobby")
	ALobbyPlayerState* GetHostPlayerState() const;

	// The other player's state (right lobby slot), or null while nobody has joined.
	UFUNCTION(BlueprintPure, Category = "Lobby")
	ALobbyPlayerState* GetGuestPlayerState() const;

	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FOnLobbyPlayersChanged OnLobbyPlayersChanged;
};