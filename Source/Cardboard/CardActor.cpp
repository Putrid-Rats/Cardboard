#include "CardActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ACardActor::ACardActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("CardRoot"));

	CardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CardMesh"));
	CardMesh->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	if (CubeMesh.Succeeded())
	{
		CardMesh->SetStaticMesh(CubeMesh.Object);
	}

	// The engine cube is 100 cm on each side.
	CardMesh->SetRelativeScale3D(FVector(CardThickness, CardWidth, CardHeight) / 100.0f);
	CardMesh->SetCastShadow(false);

	// Only mouse traces (Visibility) hit cards, for hovering and dragging later.
	CardMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CardMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	CardMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}
