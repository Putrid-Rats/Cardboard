#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SeatedPawn.generated.h"

class ACardActor;
class ACardTable;
class APlayerController;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;

// One card in a player's hand.
USTRUCT(BlueprintType)
struct FHandCard
{
	GENERATED_BODY()

	// Unique within this player's hand, so plays name an exact card even with duplicates.
	UPROPERTY(BlueprintReadOnly, Category = "Hand")
	int32 InstanceId = 0;

	// Row name in DT_Cards.
	UPROPERTY(BlueprintReadOnly, Category = "Hand")
	FName CardId;
};

// A player sitting at the table. Doesn't move, only looks around.
// Free look: mouse turns the head (raw mouse delta), cursor hidden.
// Table view (toggled with TableViewAction): camera blends to TableViewPoint, cursor shown for cards,
// and the hand of cards slides up from the bottom of the screen. Hovering a card brings it forward,
// holding the left mouse button drags it to reorder the hand, and releasing it above the hand
// asks the server to play it onto this seat's row on the table.
// The look rotation is replicated so the other player sees the cutout turn.
// Deck and hand live on the server. The deck never leaves it; the hand replicates to the owner only,
// and each play is checked against it. Card visuals are spawned locally from the replicated hand.
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

	// Child of Camera, so the hand stays fixed on screen and always slides in from the bottom,
	// even while the camera is still gliding. Cards are attached here and laid out in a fan.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Seat|Hand")
	TObjectPtr<USceneComponent> HandRoot;

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

	UPROPERTY(EditDefaultsOnly, Category = "Seat|Hand")
	TSubclassOf<ACardActor> CardClass;

	// Most cards a hand can hold.
	static constexpr int32 MaxHandSize = 6;

	// Cards dealt from the shuffled deck when the player sits down.
	UPROPERTY(EditDefaultsOnly, Category = "Seat|Hand", meta = (ClampMin = 0, ClampMax = 6))
	int32 OpeningHandSize = 5;

	// Server only. Moves cards from the top of the deck into the hand.
	// With a full hand the drawn card is discarded; with an empty deck nothing happens (yet).
	void DrawCards(int32 Count);

	// Cards left in the deck. Visible to both players.
	UFUNCTION(BlueprintPure, Category = "Seat|Hand")
	int32 GetDeckCount() const { return DeckCount; }

	// Testing only: type DebugDrawCard in the console (`) to draw one card.
	UFUNCTION(Exec)
	void DebugDrawCard();

	// HandRoot position relative to the camera when the hand is up (X forward, Z up, in cm).
	UPROPERTY(EditDefaultsOnly, Category = "Seat|Hand")
	FVector HandShownOffset = FVector(25.0f, 0.0f, -10.0f);

	// How far below the shown position the hand waits while hidden.
	UPROPERTY(EditDefaultsOnly, Category = "Seat|Hand")
	float HandHiddenDrop = 15.0f;

	// Distance between card centres. Less than the card width, so neighbours overlap.
	UPROPERTY(EditDefaultsOnly, Category = "Seat|Hand")
	float HandCardSpacing = 5.0f;

	// Tilt per card away from the centre, in degrees. Flip the sign if the fan leans the wrong way.
	UPROPERTY(EditDefaultsOnly, Category = "Seat|Hand")
	float HandFanAngle = 5.0f;

	// How far the outer cards drop below the middle ones (per card from the centre, squared).
	UPROPERTY(EditDefaultsOnly, Category = "Seat|Hand")
	float HandArcDrop = 0.4f;

	// How quickly cards move to their place in the hand (higher = snappier).
	UPROPERTY(EditDefaultsOnly, Category = "Seat|Hand")
	float HandCardMoveSpeed = 15.0f;

	// Hovered card: how far it comes towards the camera (bigger on screen) and how far it rises.
	UPROPERTY(EditDefaultsOnly, Category = "Seat|Hand")
	float HoverForward = 8.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Seat|Hand")
	float HoverRaise = 5.5f;

	// Dragged card: how far in front of the rest of the hand it floats.
	UPROPERTY(EditDefaultsOnly, Category = "Seat|Hand")
	float DragForward = 4.0f;

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

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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

	// Server only: card IDs still to draw, the last one is the top.
	TArray<FName> Deck;

	bool bDealtOpeningHand = false;

	UPROPERTY(Replicated)
	int32 DeckCount = 0;

	// The real hand. Replicates to the owning player only, so the opponent never learns it.
	UPROPERTY(ReplicatedUsing = OnRep_Hand)
	TArray<FHandCard> Hand;

	int32 NextHandInstanceId = 1;

	// Visuals of Hand, spawned on the owning player's machine, in the player's own order (left to right).
	UPROPERTY(Transient)
	TArray<TObjectPtr<ACardActor>> HandCards;

	// Played cards waiting for the server: hidden, and brought back if the server refuses the play.
	UPROPERTY(Transient)
	TArray<TObjectPtr<ACardActor>> PendingPlayCards;

	// 0 = hand hidden below the screen, 1 = hand fully up.
	float HandProgress = 0.0f;

	// Card under the mouse (picked by hand slot, so it doesn't flicker as the card moves).
	UPROPERTY(Transient)
	TObjectPtr<ACardActor> HoveredCard;

	// Card held with the left mouse button. Moving it sideways reorders the hand.
	UPROPERTY(Transient)
	TObjectPtr<ACardActor> DraggedCard;

	// Mouse position on the hand's plane, in HandRoot space, while dragging.
	FVector DragLocation = FVector::ZeroVector;

	// Where the dragged card would land in this seat's row, or INDEX_NONE while it's over the hand.
	int32 BoardInsertIndex = INDEX_NONE;

	UPROPERTY(Transient)
	TObjectPtr<ACardTable> Table;

	// Play the hand card with this InstanceId into this seat's row at InsertIndex.
	UFUNCTION(Server, Reliable)
	void ServerPlayCard(int32 HandInstanceId, int32 InsertIndex);

	// The server refused a play (card not in hand, row full): put the card back into the hand.
	UFUNCTION(Client, Reliable)
	void ClientRejectPlay(int32 HandInstanceId);

	UFUNCTION(Server, Reliable)
	void ServerDebugDrawCard();

	UFUNCTION()
	void OnRep_Hand();

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
	void BuildDeck();

	// Spawns visuals for new hand cards and removes visuals of cards that left the hand.
	void SyncHandVisuals();
	void UpdateHandSlide();
	void UpdateHandInteraction();
	void UpdateBoardPreview(const APlayerController& PlayerController, bool bOverBoard);
	void PlayDraggedCard();
	float GetHandTop() const;
	void UpdateHandCards(float DeltaSeconds, bool bSnap);
	FTransform GetCardTargetTransform(int32 Index) const;
	bool GetCursorOnHandPlane(const APlayerController& PlayerController, FVector& OutLocal) const;
	float GetPerspectiveScale(float TowardsCamera) const;
	int32 GetHandSlotAt(float LocalY) const;
	ACardActor* GetHandCardAt(const FVector& Local) const;
};
