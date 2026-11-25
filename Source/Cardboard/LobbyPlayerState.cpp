#include "LobbyPlayerState.h"

#include "LobbyGameState.h"
#include "Net/UnrealNetwork.h"

void ALobbyPlayerState::SetReady(bool bNewReady)
{
	if (!HasAuthority())
	{
		return;
	}

	bIsReady = bNewReady;
	OnRep_IsReady();
}

void ALobbyPlayerState::SetIsHost(bool bNewIsHost)
{
	if (!HasAuthority())
	{
		return;
	}

	bIsHost = bNewIsHost;
	OnRep_IsReady();
}

// Also the RepNotify for bIsHost: either change means the lobby UI should refresh.
void ALobbyPlayerState::OnRep_IsReady()
{
	if (ALobbyGameState* LobbyGameState = GetWorld()->GetGameState<ALobbyGameState>())
	{
		LobbyGameState->NotifyReadyStateChanged();
	}
}

void ALobbyPlayerState::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALobbyPlayerState, bIsReady);
	DOREPLIFETIME(ALobbyPlayerState, bIsHost);
}