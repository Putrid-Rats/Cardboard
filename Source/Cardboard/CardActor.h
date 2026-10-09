#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CardActor.generated.h"

class UStaticMeshComponent;

// One card in the world. Placeholder look: the engine cube scaled to a standard
// 2.5" x 3.5" trading card. Thin along X, width along Y, height along Z,
// so with X pointing away from the camera the face looks straight at the player.
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
};
