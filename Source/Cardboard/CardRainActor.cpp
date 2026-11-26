#include "CardRainActor.h"

#include "CardboardSettings.h"
#include "CardDefinition.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"

ACardRainActor::ACardRainActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Area = CreateDefaultSubobject<UBoxComponent>(TEXT("Area"));
	Area->SetBoxExtent(FVector(50.0f, 300.0f, 200.0f));
	Area->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Area->SetHiddenInGame(true);
	RootComponent = Area;
}

void ACardRainActor::BeginPlay()
{
	Super::BeginPlay();

	// Every card's art from the card sheet (rows without a material, like The Coin, are skipped).
	if (const UDataTable* CardTable = UCardboardSettings::GetCardDataTable())
	{
		CardTable->ForeachRow<FCardDefinition>(TEXT("CardRain"), [this](const FName&, const FCardDefinition& Definition)
		{
			if (UMaterialInterface* Material = Definition.Material.LoadSynchronous())
			{
				CardMaterials.Add(Material);
			}
		});
	}

	if (!CardMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardRainActor: set Card Mesh (card_blank)"));
		return;
	}

	// Physics needs a collision shape on the mesh; without one, fall back to cards passing through each other.
	const UBodySetup* BodySetup = CardMesh->GetBodySetup();
	const bool bMeshHasCollision = BodySetup && BodySetup->AggGeom.GetElementCount() > 0;
	bPhysics = bCardsCollide && bMeshHasCollision;

	if (bCardsCollide && !bMeshHasCollision)
	{
		UE_LOG(LogTemp, Warning, TEXT("CardRainActor: %s has no simple collision, so cards can't collide. Open it > Collision > Add Box Simplified Collision."), *CardMesh->GetName());
	}

	const float HalfHeight = Area->GetUnscaledBoxExtent().Z;

	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this);
		Mesh->SetStaticMesh(CardMesh);
		Mesh->SetCastShadow(false);
		Mesh->SetupAttachment(Area);

		if (bPhysics)
		{
			// Cards only collide with each other (and any other physics bodies), never with the level or camera.
			Mesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
			Mesh->SetCollisionObjectType(ECC_PhysicsBody);
			Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
			Mesh->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
			Mesh->SetUseCCD(true);
			Mesh->SetAngularDamping(0.2f);
		}
		else
		{
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		Mesh->RegisterComponent();

		if (bPhysics)
		{
			Mesh->SetSimulatePhysics(true);
		}

		FFallingCard Card;
		Card.Mesh = Mesh;

		// Spread over the whole height at the start, so the area is already full when the menu opens.
		ResetCard(Card, FMath::FRandRange(-HalfHeight, HalfHeight));
		Cards.Add(Card);
	}
}

void ACardRainActor::ResetCard(FFallingCard& Card, float Height)
{
	const FVector Extent = Area->GetUnscaledBoxExtent();

	Card.Start = FVector(FMath::FRandRange(-Extent.X, Extent.X), FMath::FRandRange(-Extent.Y, Extent.Y), Height);
	Card.Speed = FMath::FRandRange(FallSpeed.X, FallSpeed.Y);
	Card.Spin = FMath::FRandRange(SpinSpeed.X, SpinSpeed.Y) * (FMath::RandBool() ? 1.0f : -1.0f);
	Card.SpinAxis = FMath::VRand();
	Card.SwayPhase = FMath::FRandRange(0.0f, 2.0f * PI);
	Card.Fallen = 0.0f;

	const FRotator Rotation(FMath::FRandRange(0.0f, 360.0f), FMath::FRandRange(0.0f, 360.0f), FMath::FRandRange(0.0f, 360.0f));
	Card.Mesh->SetWorldScale3D(FVector(CardScale));

	if (CardMaterials.Num() > 0)
	{
		Card.Mesh->SetMaterial(0, CardMaterials[FMath::RandRange(0, CardMaterials.Num() - 1)]);
	}

	if (bPhysics)
	{
		// Air resistance tuned so gravity settles at this card's fall speed (terminal speed = gravity / damping).
		const float Gravity = FMath::Abs(GetWorld()->GetGravityZ());
		Card.Mesh->SetLinearDamping(Gravity / FMath::Max(Card.Speed, 1.0f));

		const FTransform AreaTransform = Area->GetComponentTransform();
		Card.Mesh->SetWorldLocationAndRotation(AreaTransform.TransformPosition(Card.Start), AreaTransform.TransformRotation(Rotation.Quaternion()),
			false, nullptr, ETeleportType::TeleportPhysics);
		Card.Mesh->SetPhysicsLinearVelocity(-AreaTransform.GetUnitAxis(EAxis::Z) * Card.Speed);
		Card.Mesh->SetPhysicsAngularVelocityInDegrees(Card.SpinAxis * Card.Spin);
	}
	else
	{
		Card.Mesh->SetRelativeLocationAndRotation(Card.Start, Rotation);
	}
}

void ACardRainActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FVector Extent = Area->GetUnscaledBoxExtent();
	const float Time = GetWorld()->GetTimeSeconds();
	const FTransform AreaTransform = Area->GetComponentTransform();

	for (FFallingCard& Card : Cards)
	{
		if (bPhysics)
		{
			TickPhysicsCard(Card, AreaTransform, Extent, Time);
			continue;
		}

		Card.Fallen += Card.Speed * DeltaSeconds;

		FVector Location = Card.Start;
		Location.Z -= Card.Fallen;
		Location.Y += FMath::Sin(Time * SwaySpeed + Card.SwayPhase) * SwayAmount;

		// Out of the bottom: back in just above the top, as a new card.
		if (Location.Z < -Extent.Z)
		{
			ResetCard(Card, Extent.Z + FMath::FRandRange(0.0f, 20.0f) * CardScale);
			continue;
		}

		Card.Mesh->SetRelativeLocation(Location);
		Card.Mesh->AddLocalRotation(FQuat(Card.SpinAxis, FMath::DegreesToRadians(Card.Spin * DeltaSeconds)));
	}
}

void ACardRainActor::TickPhysicsCard(FFallingCard& Card, const FTransform& AreaTransform, const FVector& Extent, float Time)
{
	const FVector Local = AreaTransform.InverseTransformPosition(Card.Mesh->GetComponentLocation());

	// Out of the bottom (or knocked far out of the area): back in just above the top, as a new card.
	if (Local.Z < -Extent.Z || FMath::Abs(Local.X) > Extent.X * 3.0f || FMath::Abs(Local.Y) > Extent.Y * 1.5f)
	{
		ResetCard(Card, Extent.Z + FMath::FRandRange(0.0f, 20.0f) * CardScale);
		return;
	}

	// Side-to-side drift. With heavy damping, velocity follows acceleration / damping,
	// so this gives roughly SwayAmount of drift at SwaySpeed.
	const float Damping = Card.Mesh->GetLinearDamping();
	const float Sway = FMath::Sin(Time * SwaySpeed + Card.SwayPhase) * SwayAmount * SwaySpeed * Damping;
	FVector Push = AreaTransform.GetUnitAxis(EAxis::Y) * Sway;

	// Gently steer cards that got bumped out of the box's depth back towards its middle.
	if (FMath::Abs(Local.X) > Extent.X)
	{
		Push -= AreaTransform.GetUnitAxis(EAxis::X) * FMath::Sign(Local.X) * Damping * 10.0f;
	}

	Card.Mesh->AddForce(Push, NAME_None, true);
}
