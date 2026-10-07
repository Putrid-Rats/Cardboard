#include "LobbyGameState.h"

#include "LobbyPlayerState.h"

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

void ALobbyGameState::NotifyReadyStateChanged()
{
	OnLobbyPlayersChanged.Broadcast();
}

bool ALobbyGameState::AreAllPlayersReady() const
{
	if (PlayerArray.Num() < 2)
	{
		return false;
	}

	for (APlayerState* PlayerState : PlayerArray)
	{
		const ALobbyPlayerState* LobbyPlayerState = Cast<ALobbyPlayerState>(PlayerState);

		if (!LobbyPlayerState || !LobbyPlayerState->IsReady())
		{
			return false;
		}
	}

	return true;
}