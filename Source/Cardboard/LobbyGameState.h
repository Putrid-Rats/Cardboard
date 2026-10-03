#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LobbyGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyPlayersChanged);

UCLASS()
class CARDBOARD_API ALobbyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FOnLobbyPlayersChanged OnLobbyPlayersChanged;
};