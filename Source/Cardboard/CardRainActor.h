#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CardRainActor.generated.h"

class UBoxComponent;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

// Menu background: cards falling and tumbling through a box, each showing a random card's art from DT_Cards.
// A card that leaves the bottom comes back in at the top as a different card.
// Place it in front of the menu camera and size the box (Area) in the level; it's only drawn in the editor.
UCLASS()
class CARDBOARD_API ACardRainActor : public AActor
{
	GENERATED_BODY()

public:

	ACardRainActor();

	// The card model (card_blank).
	UPROPERTY(EditAnywhere, Category = "Card Rain")
	TObjectPtr<UStaticMesh> CardMesh;

	// 1 = real size (like BP_CardActor's 0.85 model scale); bigger reads better from a distance.
	UPROPERTY(EditAnywhere, Category = "Card Rain", meta = (ClampMin = 0.01))
	float CardScale = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Card Rain", meta = (ClampMin = 1, ClampMax = 200))
	int32 CardCount = 30;

	// Falling speed in cm/s, picked per card between X and Y.
	UPROPERTY(EditAnywhere, Category = "Card Rain")
	FVector2D FallSpeed = FVector2D(15.0f, 40.0f);

	// Tumbling speed in degrees/s, picked per card between X and Y.
	UPROPERTY(EditAnywhere, Category = "Card Rain")
	FVector2D SpinSpeed = FVector2D(20.0f, 90.0f);

	// How far a card drifts side to side while falling, in cm, and how fast.
	UPROPERTY(EditAnywhere, Category = "Card Rain")
	float SwayAmount = 20.0f;

	UPROPERTY(EditAnywhere, Category = "Card Rain")
	float SwaySpeed = 1.0f;

	// Cards are physics bodies that bump into each other instead of passing through.
	// Needs simple collision on Card Mesh (Static Mesh editor > Collision > Add Box Simplified Collision).
	UPROPERTY(EditAnywhere, Category = "Card Rain")
	bool bCardsCollide = true;

	// The area the cards fall through. Drag its size in the level.
	UPROPERTY(VisibleAnywhere, Category = "Card Rain")
	TObjectPtr<UBoxComponent> Area;

	virtual void Tick(float DeltaSeconds) override;

protected:

	virtual void BeginPlay() override;

private:

	struct FFallingCard
	{
		TObjectPtr<UStaticMeshComponent> Mesh;
		FVector Start;
		float Speed = 0.0f;
		FVector SpinAxis = FVector::UpVector;
		float Spin = 0.0f;
		float SwayPhase = 0.0f;
		float Fallen = 0.0f;
	};

	TArray<FFallingCard> Cards;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> CardMaterials;

	// bCardsCollide, and the mesh has collision to simulate with.
	bool bPhysics = false;

	void TickPhysicsCard(FFallingCard& Card, const FTransform& AreaTransform, const FVector& Extent, float Time);

	// Gives the card a new card face, speed and spin, and puts it at Height (relative to the area's centre).
	void ResetCard(FFallingCard& Card, float Height);
};
