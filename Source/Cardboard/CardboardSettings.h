#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CardboardSettings.generated.h"

class UDataTable;
struct FCardDefinition;
struct FDeckDefinition;

// Project Settings → Game → Cardboard. Saved to Config/DefaultGame.ini.
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Cardboard"))
class CARDBOARD_API UCardboardSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	// Every card in the game (DataTable with row structure CardDefinition).
	UPROPERTY(Config, EditAnywhere, Category = "Cards", meta = (RequiredAssetDataTags = "RowStructure=/Script/Cardboard.CardDefinition"))
	TSoftObjectPtr<UDataTable> CardDataTable;

	// The decks players can pick in the lobby (DataTable with row structure DeckDefinition).
	// The first row is the default deck.
	UPROPERTY(Config, EditAnywhere, Category = "Cards", meta = (RequiredAssetDataTags = "RowStructure=/Script/Cardboard.DeckDefinition"))
	TSoftObjectPtr<UDataTable> DeckDataTable;

	// Row ID of The Coin in the card table: given to the player who goes second, never in the deck.
	// Playing it gives +1 mana for that turn instead of going on the table.
	UPROPERTY(Config, EditAnywhere, Category = "Cards")
	FName CoinCardId = TEXT("Coin");

	static FName GetCoinCardId();

	static UDataTable* GetCardDataTable();

	// Null if the ID isn't in the table (or no table is set).
	static const FCardDefinition* FindCard(FName CardId);

	static UDataTable* GetDeckDataTable();

	// Null if the ID isn't in the deck table (or no table is set).
	static const FDeckDefinition* FindDeck(FName DeckId);

	// Deck IDs in table order.
	static TArray<FName> GetDeckIds();

	// The first deck in the table, or None without one.
	static FName GetDefaultDeckId();
};
