#include "SeatedPawn.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Camera/CameraComponent.h"
#include "CardActor.h"
#include "CardTable.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"

ASeatedPawn::ASeatedPawn()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SeatRoot"));

	YawPivot = CreateDefaultSubobject<USceneComponent>(TEXT("YawPivot"));
	YawPivot->SetupAttachment(RootComponent);

	PitchPivot = CreateDefaultSubobject<USceneComponent>(TEXT("PitchPivot"));
	PitchPivot->SetupAttachment(YawPivot);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(PitchPivot);

	TableViewPoint = CreateDefaultSubobject<USceneComponent>(TEXT("TableViewPoint"));
	TableViewPoint->SetupAttachment(RootComponent);

	HandRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HandRoot"));
	HandRoot->SetupAttachment(Camera);
	HandRoot->SetRelativeLocation(HandShownOffset - FVector(0.0f, 0.0f, HandHiddenDrop));

	CardClass = ACardActor::StaticClass();
}

void ASeatedPawn::BeginPlay()
{
	Super::BeginPlay();

	// The level has one table; it's placed in the map, so it exists on the server and on clients.
	for (TActorIterator<ACardTable> It(GetWorld()); It; ++It)
	{
		Table = *It;
		break;
	}
}

void ASeatedPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsLocallyControlled())
	{
		return;
	}

	// Raw mouse movement this frame, read straight from the controller.
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		float MouseDeltaX = 0.0f;
		float MouseDeltaY = 0.0f;
		PlayerController->GetInputMouseDelta(MouseDeltaX, MouseDeltaY);

		HandleLook(FVector2D(MouseDeltaX, MouseDeltaY));
	}

	if (CameraBlendProgress < 1.0f)
	{
		CameraBlendProgress = FMath::Min(CameraBlendProgress + DeltaSeconds * TableViewBlendSpeed, 1.0f);
		UpdateCameraBlend();
	}

	const float HandTarget = bInTableView ? 1.0f : 0.0f;

	if (HandProgress != HandTarget)
	{
		HandProgress = FMath::FInterpConstantTo(HandProgress, HandTarget, DeltaSeconds, TableViewBlendSpeed);
		UpdateHandSlide();
	}

	UpdateHandInteraction();
	UpdateHandCards(DeltaSeconds, false);
}

void ASeatedPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (!EnhancedInput)
	{
		UE_LOG(LogTemp, Warning, TEXT("SeatedPawn: not an Enhanced Input component, input disabled"));
		return;
	}

	if (TableViewAction)
	{
		EnhancedInput->BindAction(TableViewAction, ETriggerEvent::Started, this, &ASeatedPawn::HandleToggleTableView);
	}
}

void ASeatedPawn::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	APlayerController* PlayerController = Cast<APlayerController>(GetController());

	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());

	if (InputSubsystem && SeatMappingContext)
	{
		InputSubsystem->AddMappingContext(SeatMappingContext, 0);
	}

	ApplyCursorMode();
	SpawnPlaceholderHand();
}

void ASeatedPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Attached actors are only detached when the parent goes away, not destroyed.
	for (ACardActor* Card : HandCards)
	{
		if (Card)
		{
			Card->Destroy();
		}
	}

	HandCards.Empty();

	Super::EndPlay(EndPlayReason);
}

void ASeatedPawn::SpawnPlaceholderHand()
{
	if (!CardClass || HandCards.Num() > 0)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 Index = 0; Index < FMath::Min(PlaceholderHandSize, MaxHandSize); ++Index)
	{
		ACardActor* Card = GetWorld()->SpawnActor<ACardActor>(CardClass, HandRoot->GetComponentTransform(), SpawnParams);

		if (!Card)
		{
			continue;
		}

		Card->AttachToComponent(HandRoot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		HandCards.Add(Card);
	}

	UpdateHandCards(0.0f, true);
	UpdateHandSlide();
}

void ASeatedPawn::UpdateHandInteraction()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());

	// Only once the hand is fully up in table view.
	if (!PlayerController || !bInTableView || HandProgress < 1.0f)
	{
		HoveredCard = nullptr;
		DraggedCard = nullptr;

		if (PlayerController)
		{
			UpdateBoardPreview(*PlayerController, false);
		}

		return;
	}

	FVector CursorLocal;
	const bool bCursorOnPlane = GetCursorOnHandPlane(*PlayerController, CursorLocal);

	if (DraggedCard)
	{
		if (PlayerController->IsInputKeyDown(EKeys::LeftMouseButton))
		{
			// Above the hand counts as over the table.
			const bool bOverBoard = bCursorOnPlane && CursorLocal.Z > GetHandTop();

			if (bCursorOnPlane)
			{
				DragLocation = CursorLocal;
			}

			if (bCursorOnPlane && !bOverBoard)
			{
				// Reorder: the dragged card takes the slot under the mouse, the others shift over.
				const int32 OldIndex = HandCards.IndexOfByKey(DraggedCard);
				const int32 NewIndex = GetHandSlotAt(CursorLocal.Y);

				if (OldIndex != INDEX_NONE && NewIndex != OldIndex)
				{
					HandCards.RemoveAt(OldIndex);
					HandCards.Insert(DraggedCard, NewIndex);
				}
			}

			UpdateBoardPreview(*PlayerController, bOverBoard);

			HoveredCard = DraggedCard;
			return;
		}

		// Released over the table: play it. Otherwise it glides back into its (possibly new) slot.
		if (BoardInsertIndex != INDEX_NONE)
		{
			PlayDraggedCard();
		}

		DraggedCard = nullptr;
		UpdateBoardPreview(*PlayerController, false);
	}

	HoveredCard = bCursorOnPlane ? GetHandCardAt(CursorLocal) : nullptr;

	if (HoveredCard && PlayerController->WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		DraggedCard = HoveredCard;
		DragLocation = CursorLocal;
	}
}

void ASeatedPawn::UpdateBoardPreview(const APlayerController& PlayerController, bool bOverBoard)
{
	const int32 PreviousIndex = BoardInsertIndex;
	BoardInsertIndex = INDEX_NONE;

	FVector RayOrigin;
	FVector RayDirection;

	if (bOverBoard && Table && PlayerController.DeprojectMousePositionToWorld(RayOrigin, RayDirection))
	{
		int32 InsertIndex;

		if (Table->GetInsertIndexAt(SeatIndex, RayOrigin, RayDirection, InsertIndex))
		{
			BoardInsertIndex = InsertIndex;
		}
	}

	if (Table && BoardInsertIndex != PreviousIndex)
	{
		Table->SetPlacementPreview(SeatIndex, BoardInsertIndex);
	}
}

void ASeatedPawn::PlayDraggedCard()
{
	// The hand is local for now, so the card just leaves it; the server adds the board card for everyone.
	HandCards.Remove(DraggedCard);
	DraggedCard->Destroy();

	ServerPlaceCard(BoardInsertIndex);
}

void ASeatedPawn::ServerPlaceCard_Implementation(int32 InsertIndex)
{
	if (Table)
	{
		Table->PlaceCard(SeatIndex, InsertIndex);
	}
}

float ASeatedPawn::GetHandTop() const
{
	return ACardActor::CardHeight * 0.5f + HoverRaise;
}

void ASeatedPawn::UpdateHandCards(float DeltaSeconds, bool bSnap)
{
	for (int32 Index = 0; Index < HandCards.Num(); ++Index)
	{
		ACardActor* Card = HandCards[Index];

		if (!Card)
		{
			continue;
		}

		const FTransform Target = GetCardTargetTransform(Index);

		if (bSnap)
		{
			Card->SetActorRelativeLocation(Target.GetLocation());
			Card->SetActorRelativeRotation(Target.Rotator());
			continue;
		}

		// The held card follows the mouse more tightly than the rest.
		const float Speed = Card == DraggedCard ? HandCardMoveSpeed * 2.0f : HandCardMoveSpeed;
		const USceneComponent* CardRoot = Card->GetRootComponent();

		Card->SetActorRelativeLocation(FMath::VInterpTo(CardRoot->GetRelativeLocation(), Target.GetLocation(), DeltaSeconds, Speed));
		Card->SetActorRelativeRotation(FMath::RInterpTo(CardRoot->GetRelativeRotation(), Target.Rotator(), DeltaSeconds, Speed));
	}
}

FTransform ASeatedPawn::GetCardTargetTransform(int32 Index) const
{
	const ACardActor* Card = HandCards[Index];

	// Negative left of the middle card, positive right of it.
	const float FromMiddle = Index - (HandCards.Num() - 1) * 0.5f;
	const float SlotY = FromMiddle * HandCardSpacing;

	// Held and hovered cards face the screen straight on, in front of the rest (negative X = towards the camera).
	// A card closer to the camera looks further from the screen centre, so pull it in by the same ratio
	// to keep it lined up with its slot (hover) or exactly under the mouse (drag).
	if (Card == DraggedCard)
	{
		const float Perspective = GetPerspectiveScale(DragForward);
		return FTransform(FRotator::ZeroRotator, FVector(-DragForward, DragLocation.Y * Perspective, DragLocation.Z * Perspective));
	}

	if (Card == HoveredCard)
	{
		return FTransform(FRotator::ZeroRotator, FVector(-HoverForward, SlotY * GetPerspectiveScale(HoverForward), HoverRaise));
	}

	// Resting in the fan. Each card a bit closer to the camera than the one on its left, so overlaps don't flicker.
	const FVector Location(
		-Index * ACardActor::CardThickness * 1.5f,
		SlotY,
		-FMath::Square(FromMiddle) * HandArcDrop
	);

	return FTransform(FRotator(0.0f, 0.0f, FromMiddle * HandFanAngle), Location);
}

bool ASeatedPawn::GetCursorOnHandPlane(const APlayerController& PlayerController, FVector& OutLocal) const
{
	FVector RayOrigin;
	FVector RayDirection;

	if (!PlayerController.DeprojectMousePositionToWorld(RayOrigin, RayDirection))
	{
		return false;
	}

	// The hand lies on a flat plane through HandRoot, facing the camera.
	const FVector PlaneOrigin = HandRoot->GetComponentLocation();
	const FVector PlaneNormal = HandRoot->GetForwardVector();
	const float Facing = FVector::DotProduct(RayDirection, PlaneNormal);

	if (FMath::IsNearlyZero(Facing))
	{
		return false;
	}

	const float Distance = FVector::DotProduct(PlaneOrigin - RayOrigin, PlaneNormal) / Facing;

	if (Distance <= 0.0f)
	{
		return false;
	}

	OutLocal = HandRoot->GetComponentTransform().InverseTransformPosition(RayOrigin + RayDirection * Distance);
	return true;
}

float ASeatedPawn::GetPerspectiveScale(float TowardsCamera) const
{
	// HandRoot sits HandShownOffset.X in front of the camera; something TowardsCamera closer
	// appears bigger and further out by HandDistance / (HandDistance - TowardsCamera).
	const float HandDistance = HandShownOffset.X;
	return HandDistance > TowardsCamera ? (HandDistance - TowardsCamera) / HandDistance : 1.0f;
}

int32 ASeatedPawn::GetHandSlotAt(float LocalY) const
{
	const float MiddleIndex = (HandCards.Num() - 1) * 0.5f;
	return FMath::Clamp(FMath::RoundToInt(LocalY / HandCardSpacing + MiddleIndex), 0, HandCards.Num() - 1);
}

ACardActor* ASeatedPawn::GetHandCardAt(const FVector& Local) const
{
	if (HandCards.Num() == 0)
	{
		return nullptr;
	}

	const float MiddleIndex = (HandCards.Num() - 1) * 0.5f;
	const float HalfWidth = MiddleIndex * HandCardSpacing + ACardActor::CardWidth * 0.5f;
	const float Bottom = -ACardActor::CardHeight * 0.5f - FMath::Square(MiddleIndex) * HandArcDrop;

	// A raised card reaches higher, so keep it hovered while the mouse is on its upper part.
	const float Top = HoveredCard ? GetHandTop() : ACardActor::CardHeight * 0.5f;

	if (FMath::Abs(Local.Y) > HalfWidth || Local.Z < Bottom || Local.Z > Top)
	{
		return nullptr;
	}

	return HandCards[GetHandSlotAt(Local.Y)];
}

void ASeatedPawn::UpdateHandSlide()
{
	const float SmoothAlpha = FMath::SmoothStep(0.0f, 1.0f, HandProgress);

	HandRoot->SetRelativeLocation(HandShownOffset - FVector(0.0f, 0.0f, HandHiddenDrop * (1.0f - SmoothAlpha)));

	// Fully down: hide the cards, so they don't float in view when looking down in free look.
	HandRoot->SetHiddenInGame(HandProgress <= 0.0f, true);
}

void ASeatedPawn::HandleLook(const FVector2D& MouseDelta)
{
	const FVector2D LookInput = MouseDelta * LookSensitivity;

	if (bInTableView || LookInput.IsNearlyZero())
	{
		return;
	}

	// Raw mouse Y is positive when moving the mouse up, which is a positive (upward) pitch.
	FRotator NewLookRotation = LookRotation;
	NewLookRotation.Yaw += LookInput.X;
	NewLookRotation.Pitch += LookInput.Y;

	SetLookRotation(NewLookRotation);
}

void ASeatedPawn::HandleToggleTableView()
{
	SetTableView(!bInTableView);
}

void ASeatedPawn::SetTableView(bool bEnable)
{
	if (!IsLocallyControlled() || bInTableView == bEnable)
	{
		return;
	}

	bInTableView = bEnable;

	// Start the glide from wherever the camera is right now (also mid-glide).
	CameraBlendStart = Camera->GetComponentTransform();
	CameraBlendProgress = 0.0f;

	// Face the table, so the other player sees this cutout looking at the cards.
	if (bInTableView)
	{
		SetLookRotation(FRotator::ZeroRotator);
	}

	ApplyCursorMode();
	OnTableViewChanged(bInTableView);
}

void ASeatedPawn::ApplyCursorMode()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());

	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	PlayerController->SetShowMouseCursor(bInTableView);

	if (bInTableView)
	{
		UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PlayerController, nullptr, EMouseLockMode::DoNotLock, false);
	}
	else
	{
		UWidgetBlueprintLibrary::SetInputMode_GameOnly(PlayerController);
	}
}

void ASeatedPawn::UpdateCameraBlend()
{
	// Free look target follows the head (PitchPivot), so mouse movement during the glide is respected.
	const FTransform Target = bInTableView
		? TableViewPoint->GetComponentTransform()
		: PitchPivot->GetComponentTransform();

	const float SmoothAlpha = FMath::SmoothStep(0.0f, 1.0f, CameraBlendProgress);

	const FVector Location = FMath::Lerp(CameraBlendStart.GetLocation(), Target.GetLocation(), SmoothAlpha);
	const FQuat Rotation = FQuat::Slerp(CameraBlendStart.GetRotation(), Target.GetRotation(), SmoothAlpha);

	Camera->SetWorldLocationAndRotation(Location, Rotation);

	// Back in free look: re-centre the camera on the head so it follows the mouse again.
	if (!bInTableView && CameraBlendProgress >= 1.0f)
	{
		Camera->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	}
}

void ASeatedPawn::SetLookRotation(const FRotator& NewLookRotation)
{
	LookRotation = ClampLookRotation(NewLookRotation);
	ApplyLookRotation();

	if (!HasAuthority())
	{
		ServerSetLookRotation(LookRotation);
	}
}

void ASeatedPawn::ServerSetLookRotation_Implementation(FRotator NewLookRotation)
{
	LookRotation = ClampLookRotation(NewLookRotation);
	ApplyLookRotation();
}

void ASeatedPawn::OnRep_LookRotation()
{
	LookRotation = ClampLookRotation(LookRotation);
	ApplyLookRotation();
}

FRotator ASeatedPawn::ClampLookRotation(const FRotator& InRotation) const
{
	// Rotators sent over the network arrive as 0..360 (-10 becomes 350), so bring them back
	// to -180..180 first, otherwise looking left/down would clamp to the far right/up limit.
	return FRotator(
		FMath::Clamp(FRotator::NormalizeAxis(InRotation.Pitch), MinPitch, MaxPitch),
		FMath::Clamp(FRotator::NormalizeAxis(InRotation.Yaw), -MaxYaw, MaxYaw),
		0.0f
	);
}

void ASeatedPawn::ApplyLookRotation()
{
	YawPivot->SetRelativeRotation(FRotator(0.0f, LookRotation.Yaw, 0.0f));
	PitchPivot->SetRelativeRotation(FRotator(LookRotation.Pitch, 0.0f, 0.0f));
}

void ASeatedPawn::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// The owner already applied its own rotation locally.
	DOREPLIFETIME_CONDITION(ASeatedPawn, LookRotation, COND_SkipOwner);

	// Arrives with the spawn, so it's already set when BeginPlay runs on clients.
	DOREPLIFETIME_CONDITION(ASeatedPawn, SeatIndex, COND_InitialOnly);
}
