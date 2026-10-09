#include "CardGameState.h"

#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "SeatedPawn.h"

void ACardGameState::EndMatch(int32 InWinningSeat)
{
	if (!HasAuthority() || IsMatchOver())
	{
		return;
	}

	WinningSeat = InWinningSeat;

	// OnRep doesn't run on the server, and the listen server host needs the event too.
	OnRep_WinningSeat();
}

void ACardGameState::OnRep_WinningSeat()
{
	for (TActorIterator<ASeatedPawn> It(GetWorld()); It; ++It)
	{
		It->NotifyMatchEnded(WinningSeat);
	}

	OnMatchEnded.Broadcast(WinningSeat);
}

void ACardGameState::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACardGameState, WinningSeat);
}
