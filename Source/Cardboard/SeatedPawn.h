#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SeatedPawn.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;

// A player sitting at the table. Doesn't move, only looks around.
// Free look: mouse turns the head (raw mouse delta), cursor hidden.
// Table view (toggled with TableViewAction): camera blends to TableViewPoint, cursor shown for cards.
// The look rotation is replicated so the other player sees the cutout turn.
UCLASS()
class CARDBOARD_API ASeatedPawn : public APawn
{
	GENERATED_BODY()

public:

	ASeatedPawn();

	// Rotates with yaw only. Attach the cardboard cutout here.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Seat")
	TObjectPtr<USceneComponent> YawPivot;

	// Child of YawPivot, rotates with pitch. Move it to eye height.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Seat")
	TObjectPtr<USceneComponent> PitchPivot;

	// Child of PitchPivot. Blends between PitchPivot and TableViewPoint.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Seat")
	TObjectPtr<UCameraComponent> Camera;

	// Where the camera goes in table view: above the seat, angled down at the table.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Seat")
	TObjectPtr<USceneComponent> TableViewPoint;

	UPROPERTY(EditDefaultsOnly, Category = "Seat|Input")
	TObjectPtr<UInputMappingContext> SeatMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Seat|Input")
	TObjectPtr<UInputAction> TableViewAction;

	UPROPERTY(EditDefaultsOnly, Category = "Seat|Input")
	float LookSensitivity = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Seat|Limits")
	float MaxYaw = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Seat|Limits")
	float MinPitch = -60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Seat|Limits")
	float MaxPitch = 40.0f;

	// How fast the camera moves between free look and table view (higher = faster).
	UPROPERTY(EditDefaultsOnly, Category = "Seat|Table View")
	float TableViewBlendSpeed = 4.0f;

	// Which seat at the table this player sits in (0 or 1). Set by the GameMode when spawning,
	// replicated once so every client can pick the right cutout.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Seat", meta = (ExposeOnSpawn = true))
	int32 SeatIndex = 0;

	// Pitch and yaw relative to the seat.
	UFUNCTION(BlueprintPure, Category = "Seat")
	FRotator GetLookRotation() const { return LookRotation; }

	UFUNCTION(BlueprintPure, Category = "Seat|Table View")
	bool IsInTableView() const { return bInTableView; }

	UFUNCTION(BlueprintCallable, Category = "Seat|Table View")
	void SetTableView(bool bEnable);

	// Local player only. Use it to show/hide the hand of cards.
	UFUNCTION(BlueprintImplementableEvent, Category = "Seat|Table View")
	void OnTableViewChanged(bool bInTableViewNow);

protected:

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;

private:

	UPROPERTY(ReplicatedUsing = OnRep_LookRotation)
	FRotator LookRotation = FRotator::ZeroRotator;

	bool bInTableView = false;

	// Camera transform when the last Space press happened; the blend starts here.
	FTransform CameraBlendStart;

	// 0 = just toggled, 1 = camera arrived at its target.
	float CameraBlendProgress = 1.0f;

	UFUNCTION()
	void OnRep_LookRotation();

	UFUNCTION(Server, Unreliable)
	void ServerSetLookRotation(FRotator NewLookRotation);

	void HandleLook(const FVector2D& MouseDelta);
	void HandleToggleTableView();
	void SetLookRotation(const FRotator& NewLookRotation);
	FRotator ClampLookRotation(const FRotator& InRotation) const;
	void ApplyLookRotation();
	void ApplyCursorMode();
	void UpdateCameraBlend();
};
