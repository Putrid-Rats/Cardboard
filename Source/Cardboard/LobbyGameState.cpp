#include "LobbyGameState.h"

void ALobbyGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);

	OnLobbyPlayersChanged.Broadcast();
}

void ALobbyGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);

	OnLobbyPlayersChanged.Broadcast();
}