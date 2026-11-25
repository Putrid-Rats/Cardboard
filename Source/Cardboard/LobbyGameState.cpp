#include "LobbyGameState.h"

#include "GameFramework/PlayerController.h"
#include "LobbyPlayerState.h"

void ALobbyGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);

	// The host is the listen server's own player: its controller is local on the server.
	if (HasAuthority())
	{
		if (ALobbyPlayerState* LobbyPlayerState = Cast<ALobbyPlayerState>(PlayerState))
		{
			const APlayerController* Controller = Cast<APlayerController>(PlayerState->GetOwner());
			LobbyPlayerState->SetIsHost(Controller && Controller->IsLocalController());
		}
	}

	OnLobbyPlayersChanged.Broadcast();
}

ALobbyPlayerState* ALobbyGameState::GetHostPlayerState() const
{
	for (APlayerState* PlayerState : PlayerArray)
	{
		ALobbyPlayerState* LobbyPlayerState = Cast<ALobbyPlayerState>(PlayerState);

		if (LobbyPlayerState && LobbyPlayerState->IsHost())
		{
			return LobbyPlayerState;
		}
	}

	return nullptr;
}

ALobbyPlayerState* ALobbyGameState::GetGuestPlayerState() const
{
	for (APlayerState* PlayerState : PlayerArray)
	{
		ALobbyPlayerState* LobbyPlayerState = Cast<ALobbyPlayerState>(PlayerState);

		if (LobbyPlayerState && !LobbyPlayerState->IsHost())
		{
			return LobbyPlayerState;
		}
	}

	return nullptr;
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