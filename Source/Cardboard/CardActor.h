#pragma once

#include "CoreMinimal.h"
#include "CardDefinition.h"
#include "GameFramework/Actor.h"
#include "CardActor.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UFont;

// One card in the world, the size of a standard 2.5" x 3.5" trading card. Thin along X, width along Y, height along Z.
// The C++ default mesh is the engine cube; BP_CardActor swaps in the real card model.
// The front face is the -X side: in the hand it faces the camera, on the table it faces up.
// SetCard fills the texts on the front from the card sheet (DT_Cards).
UCLASS()
class CARDBOARD_API ACardActor : public AActor
{
	GENERATED_BODY()

public:

	ACardActor();

	// Standard trading card size in cm (2.5" x 3.5").
	static constexpr float CardWidth = 6.35f;
	static constexpr float CardHeight = 8.89f;
	static constexpr float CardThickness = 0.2f;

	// In BP_CardActor: the card model, rotated and scaled so its front faces -X at 6.35 x 8.89 cm.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	TObjectPtr<UStaticMeshComponent> CardMesh;

	// Used when a card has no Material in the sheet, and for blank cards (opponent's hand).
	// Empty = keep the mesh's own material.
	UPROPERTY(EditDefaultsOnly, Category = "Card")
	TObjectPtr<UMaterialInterface> DefaultMaterial;

	// Font for all texts on the card. Must be an Offline font with distance field alpha (import settings
	// copied from the engine's RobotoDistanceField), since Text Render materials expect one. Empty = engine default.
	UPROPERTY(EditDefaultsOnly, Category = "Card|Text")
	TObjectPtr<UFont> TextFont;

	// Material for all texts: an instance of DefaultTextMaterialOpaque with its Font parameter set to TextFont.
	// Needed with a custom font, or the letters render garbled.
	UPROPERTY(EditDefaultsOnly, Category = "Card|Text")
	TObjectPtr<UMaterialInterface> TextMaterial;

	// Applies TextFont / TextMaterial in the Blueprint viewport too.
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	TObjectPtr<UTextRenderComponent> NameText;

	// Empty when the card has no trait.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	TObjectPtr<UTextRenderComponent> TraitText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	TObjectPtr<UTextRenderComponent> CostText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	TObjectPtr<UTextRenderComponent> AttackText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	TObjectPtr<UTextRenderComponent> HealthText;

	// Shows this card's name and stats. An ID missing from the sheet shows the ID and "?" stats.
	UFUNCTION(BlueprintCallable, Category = "Card")
	void SetCard(FName InCardId);

	// Blank card: no texts. Used for the opponent's hand, whose cards this machine doesn't know.
	UFUNCTION(BlueprintCallable, Category = "Card")
	void ClearCard();

	UFUNCTION(BlueprintPure, Category = "Card")
	FName GetCardId() const { return CardId; }

	// Which hand or board entry this visual shows (FHandCard / FBoardCard InstanceId).
	int32 InstanceId = 0;

	// Placeholder trait marker under a board card: gold for Taunt, purple while Stealthed.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	TObjectPtr<UStaticMeshComponent> TraitFrame;

	// Placeholder green outline while the card can attack this turn.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	TObjectPtr<UStaticMeshComponent> ReadyFrame;

	// Board cards: current stats and combat state, from the table.
	void SetBoardState(int32 Attack, int32 Health, ECardTrait Trait, bool bStealthed, bool bCanAttack);

	// Board cards, on every machine, whenever the state changes. Hook your Taunt/Stealth/Fly effects here
	// (and hide TraitFrame / ReadyFrame if you replace them).
	UFUNCTION(BlueprintImplementableEvent, Category = "Card")
	void OnBoardStateChanged(bool bTaunting, bool bStealthed, bool bFlying, bool bCanAttack);

	// Board cards, on every machine: the card just lost Damage health (it may be about to be removed).
	UFUNCTION(BlueprintImplementableEvent, Category = "Card")
	void OnDamaged(int32 Damage);

private:

	FName CardId;

	// Health shown last time, to tell when damage was taken. -1 = not on the board yet.
	int32 DisplayedHealth = -1;

	UTextRenderComponent* CreateCardText(FName Name, float Horizontal, float Vertical, float Size, FColor Color);
	UStaticMeshComponent* CreateFrame(FName Name, float Margin, float BehindOffset);
	void ApplyMaterial(UMaterialInterface* Material);
	static void SetFrameColor(UStaticMeshComponent* Frame, const FLinearColor& Color);
};
