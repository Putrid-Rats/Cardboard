#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CardPlayerState.generated.h"

// Player state shared by the lobby and the match: remembers which deck the player picked.
// The pick survives seamless travel both ways (lobby -> match -> Back to Lobby) through CopyProperties.
// Set as the PlayerState Class in GM_Gameplay_TCG; the lobby uses ALobbyPlayerState, which builds on it.
UCLASS()
class CARDBOARD_API ACardPlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	// The picked deck, or the default deck (first in DT_Decks) if none or an unknown one is picked.
	UFUNCTION(BlueprintPure, Category = "Deck")
	FName GetSelectedDeck() const;

	// Called by the owning player's UI. Unknown deck IDs are ignored.
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Deck")
	void ServerSelectDeck(FName DeckId);

protected:

	UPROPERTY(Replicated)
	FName SelectedDeck;

	virtual void CopyProperties(APlayerState* PlayerState) override;

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;
};
