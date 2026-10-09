#include "CardActor.h"

#include "CardboardSettings.h"
#include "CardDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
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

	// Only mouse traces (Visibility) hit cards.
	CardMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CardMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	CardMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// Layout on the front face (horizontal: + is right, vertical: + is up, in cm from the centre).
	const float Right = CardWidth * 0.5f - 0.8f;
	const float Top = CardHeight * 0.5f - 0.8f;

	CostText = CreateCardText(TEXT("CostText"), -Right, Top, 1.0f, FColor(20, 60, 200));
	NameText = CreateCardText(TEXT("NameText"), 0.0f, Top - 1.5f, 0.6f, FColor::Black);
	TraitText = CreateCardText(TEXT("TraitText"), 0.0f, 0.0f, 0.5f, FColor(90, 40, 130));
	AttackText = CreateCardText(TEXT("AttackText"), -Right, -Top, 1.0f, FColor(200, 120, 0));
	HealthText = CreateCardText(TEXT("HealthText"), Right, -Top, 1.0f, FColor(200, 20, 20));
}

UTextRenderComponent* ACardActor::CreateCardText(FName Name, float Horizontal, float Vertical, float Size, FColor Color)
{
	UTextRenderComponent* Text = CreateDefaultSubobject<UTextRenderComponent>(Name);
	Text->SetupAttachment(RootComponent);

	// Just in front of the -X face, turned around so it reads correctly from that side.
	Text->SetRelativeLocation(FVector(-CardThickness * 0.5f - 0.02f, Horizontal, Vertical));
	Text->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	Text->SetHorizontalAlignment(EHTA_Center);
	Text->SetVerticalAlignment(EVRTA_TextCenter);
	Text->SetWorldSize(Size);
	Text->SetTextRenderColor(Color);
	Text->SetCastShadow(false);
	Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	return Text;
}

void ACardActor::SetCard(FName InCardId)
{
	CardId = InCardId;

	if (const FCardDefinition* Definition = UCardboardSettings::FindCard(CardId))
	{
		NameText->SetText(Definition->CardName);
		CostText->SetText(FText::AsNumber(Definition->Cost));
		AttackText->SetText(FText::AsNumber(Definition->Attack));
		HealthText->SetText(FText::AsNumber(Definition->Health));

		TraitText->SetText(Definition->Trait == ECardTrait::None
			? FText::GetEmpty()
			: UEnum::GetDisplayValueAsText(Definition->Trait));
		return;
	}

	const FText Unknown = FText::FromString(TEXT("?"));

	NameText->SetText(FText::FromName(CardId));
	TraitText->SetText(FText::GetEmpty());
	CostText->SetText(Unknown);
	AttackText->SetText(Unknown);
	HealthText->SetText(Unknown);
}
