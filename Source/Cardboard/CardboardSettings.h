#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CardboardSettings.generated.h"

class UDataTable;
struct FCardDefinition;

// Project Settings → Game → Cardboard. Saved to Config/DefaultGame.ini.
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Cardboard"))
class CARDBOARD_API UCardboardSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	// Every card in the game (DataTable with row structure CardDefinition).
	// For now it's also the deck: every row is one card in it.
	UPROPERTY(Config, EditAnywhere, Category = "Cards", meta = (RequiredAssetDataTags = "RowStructure=/Script/Cardboard.CardDefinition"))
	TSoftObjectPtr<UDataTable> CardDataTable;

	static UDataTable* GetCardDataTable();

	// Null if the ID isn't in the table (or no table is set).
	static const FCardDefinition* FindCard(FName CardId);
};
