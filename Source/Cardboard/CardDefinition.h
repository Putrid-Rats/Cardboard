#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CardDefinition.generated.h"

// Built-in keyword a card can have. To add one, add an entry here; the sheet then accepts its name.
UENUM(BlueprintType)
enum class ECardTrait : uint8
{
	None,

	// Must be destroyed before the owning player can be attacked.
	Taunt,

	// Can attack past Taunt cards.
	Fly,

	// Can't be targeted by other cards until it attacks for the first time.
	Stealth
};

// One row of the card sheet (Data/Cards.csv, imported as the DT_Cards DataTable).
// The row name is the card ID, so it isn't repeated as a field here.
USTRUCT(BlueprintType)
struct FCardDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	FText CardName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card", meta = (ClampMin = 0))
	int32 Cost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card", meta = (ClampMin = 0))
	int32 Attack = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card", meta = (ClampMin = 1))
	int32 Health = 1;

	// In the sheet: None, Taunt, Fly or Stealth.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	ECardTrait Trait = ECardTrait::None;

	// Reserved for card abilities later. Empty = no ability.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	FName AbilityId;
};
