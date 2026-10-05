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
}