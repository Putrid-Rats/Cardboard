#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CardActor.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

// One card in the world. Placeholder look: the engine cube scaled to a standard
// 2.5" x 3.5" trading card. Thin along X, width along Y, height along Z.
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

	// Swap the mesh in a Blueprint child once there's a real card model.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	TObjectPtr<UStaticMeshComponent> CardMesh;

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

private:

	FName CardId;

	UTextRenderComponent* CreateCardText(FName Name, float Horizontal, float Vertical, float Size, FColor Color);
};
