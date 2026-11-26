#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Materials/MaterialInterface.h"
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
	Stealth,

	// Can attack the turn it's played.
	Charge
};

// What a card does when it's played from the hand (a "battlecry"), using the row's AbilityValue.
// To add one: add an entry here, handle it in ASeatedPawn::ApplyOnPlayAbility and describe it in GetAbilityText.
UENUM(BlueprintType)
enum class ECardAbility : uint8
{
	None,

	// The opponent draws AbilityValue cards (burned if their hand is full; fatigue if their deck is empty).
	OpponentDraws,

	// You draw AbilityValue cards.
	Draw,

	// You restore AbilityValue health (up to your maximum).
	Heal,

	// Deals AbilityValue damage to the opponent.
	DamageOpponent
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

	// In the sheet: None, Taunt, Fly, Stealth or Charge.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	ECardTrait Trait = ECardTrait::None;

	// What happens when the card is played. In the sheet: None, OpponentDraws, Draw, Heal or DamageOpponent.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	ECardAbility Ability = ECardAbility::None;

	// How strong the ability is (cards drawn, health restored, damage dealt).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card", meta = (ClampMin = 0))
	int32 AbilityValue = 0;

	// The card's look: a material instance of M_Card with this card's textures (front art + back).
	// Empty = the card actor's DefaultMaterial.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	TSoftObjectPtr<UMaterialInterface> Material;

	// Short rules line for the card face, e.g. "Taunt", "Foe draws 2" or "Charge, 2 dmg to foe".
	FText GetRulesText() const
	{
		TArray<FString> Parts;

		if (Trait != ECardTrait::None)
		{
			Parts.Add(UEnum::GetDisplayValueAsText(Trait).ToString());
		}

		switch (Ability)
		{
		case ECardAbility::OpponentDraws:  Parts.Add(FString::Printf(TEXT("Foe draws %d"), AbilityValue)); break;
		case ECardAbility::Draw:           Parts.Add(FString::Printf(TEXT("Draw %d"), AbilityValue)); break;
		case ECardAbility::Heal:           Parts.Add(FString::Printf(TEXT("Heal %d"), AbilityValue)); break;
		case ECardAbility::DamageOpponent: Parts.Add(FString::Printf(TEXT("%d dmg to foe"), AbilityValue)); break;
		default: break;
		}

		return FText::FromString(FString::Join(Parts, TEXT(", ")));
	}
};

// One entry in a deck list: a card from the card sheet and how many copies of it.
USTRUCT(BlueprintType)
struct FDeckCard
{
	GENERATED_BODY()

	// Row ID in the card sheet (DT_Cards).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck")
	FName CardId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck", meta = (ClampMin = 1))
	int32 Copies = 2;
};

// One row of the deck list (Data/Decks.json, imported as the DT_Decks DataTable).
// The row name is the deck ID. Players pick one in the lobby.
USTRUCT(BlueprintType)
struct FDeckDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck")
	FText DeckName;

	// One or two sentences on how the deck wins, shown in the lobby.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck")
	TArray<FDeckCard> Cards;

	int32 GetCardCount() const
	{
		int32 Count = 0;

		for (const FDeckCard& Card : Cards)
		{
			Count += Card.Copies;
		}

		return Count;
	}
};
