#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CardboardSettings.generated.h"

class UDataTable;
class USoundClass;
class USoundMix;
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

	// Row ID of The Coin in the card table: given to the player who goes second, never in the deck.
	// Playing it gives +1 mana for that turn instead of going on the table.
	UPROPERTY(Config, EditAnywhere, Category = "Cards")
	FName CoinCardId = TEXT("Coin");

	static FName GetCoinCardId();

	// Volume sliders: this sound mix gets a volume override per sound class.
	// Music and Effects classes should have Master as their parent class.
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundMix> VolumeSoundMix;

	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundClass> MusicSoundClass;

	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundClass> EffectsSoundClass;

	static UDataTable* GetCardDataTable();

	// Null if the ID isn't in the table (or no table is set).
	static const FCardDefinition* FindCard(FName CardId);
};
