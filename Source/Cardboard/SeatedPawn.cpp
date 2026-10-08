#include "SeatedPawn.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Camera/CameraComponent.h"
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
	ApplyLookRotation();
}

FRotator ASeatedPawn::ClampLookRotation(const FRotator& InRotation) const
{
	return FRotator(
		FMath::Clamp(InRotation.Pitch, MinPitch, MaxPitch),
		FMath::Clamp(InRotation.Yaw, -MaxYaw, MaxYaw),
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
