#include "CardPlayerState.h"

#include "CardboardSettings.h"
#include "Net/UnrealNetwork.h"

FName ACardPlayerState::GetSelectedDeck() const
{
	return UCardboardSettings::FindDeck(SelectedDeck) ? SelectedDeck : UCardboardSettings::GetDefaultDeckId();
}

void ACardPlayerState::ServerSelectDeck_Implementation(FName DeckId)
{
	if (UCardboardSettings::FindDeck(DeckId))
	{
		SelectedDeck = DeckId;
	}
}

void ACardPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	// Seamless travel spawns a new player state for the next level's GameMode; carry the pick over.
	if (ACardPlayerState* NewState = Cast<ACardPlayerState>(PlayerState))
	{
		NewState->SelectedDeck = SelectedDeck;
	}
}

void ACardPlayerState::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACardPlayerState, SelectedDeck);
}
