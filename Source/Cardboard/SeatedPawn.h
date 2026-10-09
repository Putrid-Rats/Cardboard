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
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
enum class EMatchPhase : uint8;

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

// What a player is doing with their hand, sent to the opponent so they can mirror it with card backs.
// Indices are hand slots left to right; card identities are never included.
USTRUCT()
struct FHandPose
{
	GENERATED_BODY()

	UPROPERTY()
	int32 HoveredIndex = INDEX_NONE;

	UPROPERTY()
	int32 DraggedIndex = INDEX_NONE;

	// Bit per slot: marked for replacing in the mulligan.
	UPROPERTY()
	int32 MarkedMask = 0;

	// Where the dragged card is, in hand space (X = sideways, Y = up).
	UPROPERTY()
	FVector2D DragLocation = FVector2D::ZeroVector;

	// The hand is raised (table view) rather than put away.
	UPROPERTY()
	bool bHandUp = false;

	bool Equals(const FHandPose& Other) const
	{
		return HoveredIndex == Other.HoveredIndex && DraggedIndex == Other.DraggedIndex
			&& MarkedMask == Other.MarkedMask && bHandUp == Other.bHandUp
			&& DragLocation.Equals(Other.DragLocation, 0.2f);
	}
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

	// Where the other player sees this player's cards (backs), in front of the cutout.
	// Child of YawPivot, so the cards turn with the player. Scale it to make the held cards bigger.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Seat|Hand")
	TObjectPtr<USceneComponent> HeldHandRoot;

	// On the table to the player's left. Child of the seat, so it stays put when the player looks around.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Seat|Table")
	TObjectPtr<USceneComponent> DeckRoot;

	// The deck pile: grows/shrinks with the number of cards left, hidden when empty. Both players see it.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Seat|Table")
	TObjectPtr<UStaticMeshComponent> DeckMesh;

	// On the table to the player's right. Attach the bottle here in BP_SeatedPawn.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Seat|Table")
	TObjectPtr<USceneComponent> BottlePoint;

	// Placeholder attack arrow: a dotted arc from the attacking card to the mouse, drawn in world space.
	// Orange when the target can be attacked, grey when it can't.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Seat|Table")
	TObjectPtr<UInstancedStaticMeshComponent> TargetArrow;

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

	// Temporary turn/mana/deck/health readout in the corner of this player's view (only they see it).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Seat|Hand")
	TObjectPtr<UTextRenderComponent> StatusText;

	// Server only. Moves cards from the top of the deck into the hand.
	// With a full hand the drawn card is discarded; with an empty deck nothing happens (yet).
	void DrawCards(int32 Count);

	// Server only. Puts a specific card into the hand (e.g. The Coin). Discarded if the hand is full.
	void AddCardToHand(FName CardId);

	// Server only, called by ACardGameState: +1 max mana (up to ManaCap), refill, draw a card.
	void BeginTurn(int32 ManaCap);

	UFUNCTION(BlueprintPure, Category = "Seat|Turn")
	int32 GetMana() const { return Mana; }

	UFUNCTION(BlueprintPure, Category = "Seat|Turn")
	int32 GetMaxMana() const { return MaxMana; }

	UFUNCTION(BlueprintPure, Category = "Seat|Turn")
	bool IsMyTurn() const;

	// Local player: ends this player's turn (also the E key in table view). Ignored if it isn't their turn.
	UFUNCTION(BlueprintCallable, Category = "Seat|Turn")
	void EndTurn();

	// Every machine, on both pawns: the coin was flipped. Show the coin here.
	UFUNCTION(BlueprintImplementableEvent, Category = "Seat|Turn")
	void OnCoinFlipped(bool bThisPlayerGoesFirst);

	// Every machine, on both pawns: a new turn started.
	UFUNCTION(BlueprintImplementableEvent, Category = "Seat|Turn")
	void OnTurnStarted(bool bThisPlayersTurn);

	// Local player, during the mulligan: replaces the marked cards (also the E key).
	// Marked cards go back into the deck after the replacements are drawn.
	UFUNCTION(BlueprintCallable, Category = "Seat|Turn")
	void ConfirmMulligan();

	UFUNCTION(BlueprintPure, Category = "Seat|Turn")
	bool IsMulliganConfirmed() const { return bMulliganConfirmed; }

	// Server only: replace these hand cards and mark this player's mulligan as done.
	// Also used with an empty list when the mulligan time runs out.
	void ApplyMulligan(const TArray<int32>& ReplaceInstanceIds);

	// Every machine, on both pawns: the opening hands are dealt and the mulligan begins.
	UFUNCTION(BlueprintImplementableEvent, Category = "Seat|Turn")
	void OnMulliganStarted();

	// Called by ACardGameState.
	void NotifyCoinFlipped(int32 FirstSeat);
	void NotifyTurnStarted(int32 TurnSeat);
	void NotifyPhaseChanged(EMatchPhase NewPhase);

	// Cards left in the deck. Visible to both players.
	UFUNCTION(BlueprintPure, Category = "Seat|Hand")
	int32 GetDeckCount() const { return DeckCount; }

	// Every machine, on this player's pawn: the deck got smaller or bigger (draw, mulligan).
	UFUNCTION(BlueprintImplementableEvent, Category = "Seat|Hand")
	void OnDeckCountChanged(int32 NewDeckCount);

	// The pawn in the other seat, or null if nobody sits there. E.g. to make the opponent's cutout drink.
	UFUNCTION(BlueprintPure, Category = "Seat")
	ASeatedPawn* GetOpponentPawn() const;

	// Testing only: type DebugDrawCard in the console (`) to draw one card.
	UFUNCTION(Exec)
	void DebugDrawCard();

	UPROPERTY(EditDefaultsOnly, Category = "Seat|Health", meta = (ClampMin = 1))
	int32 MaxHealth = 15;

	UFUNCTION(BlueprintPure, Category = "Seat|Health")
	int32 GetHealth() const { return Health; }

	// 1 = full bottle, 0 = empty. Drive the drink level with this.
	UFUNCTION(BlueprintPure, Category = "Seat|Health")
	float GetHealthFraction() const { return MaxHealth > 0 ? static_cast<float>(Health) / MaxHealth : 0.0f; }

	// Server only. Reaching 0 ends the match with the opponent as the winner.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Seat|Health")
	void ApplyPlayerDamage(int32 Amount);

	// Every machine, on this player's pawn: health went from OldHealth to NewHealth
	// (the opponent takes a sip of this player's bottle). The first update after spawning has OldHealth 0.
	UFUNCTION(BlueprintImplementableEvent, Category = "Seat|Health")
	void OnHealthChanged(int32 NewHealth, int32 OldHealth);

	// Every machine, on both pawns: the match is decided. bThisPlayerWon is about this pawn's player
	// (the winner drinks the rest of the opponent's bottle and then finishes their own).
	UFUNCTION(BlueprintImplementableEvent, Category = "Seat|Health")
	void OnMatchEnded(bool bThisPlayerWon);

	// Called by ACardGameState when a winner is set.
	void NotifyMatchEnded(int32 WinningSeat);

	// Testing only: type DebugDamage 3 in the console to take 3 damage yourself.
	UFUNCTION(Exec)
	void DebugDamage(int32 Amount);

	// Testing only: type DebugMana 10 in the console to set your mana (and max mana) to 10.
	UFUNCTION(Exec)
	void DebugMana(int32 Amount);

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

	// Opponent's view: how far the held cards drop when this player puts the hand away (free look).
	UPROPERTY(EditDefaultsOnly, Category = "Seat|Hand")
	float HeldHandLowerDistance = 15.0f;

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

	bool bDeckBuilt = false;

	UPROPERTY(ReplicatedUsing = OnRep_DeckCount)
	int32 DeckCount = 0;

	UFUNCTION()
	void OnRep_DeckCount();

	void SetDeckCount(int32 NewDeckCount);

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

	// Attack targeting: the board card being aimed with (held left mouse button), or INDEX_NONE.
	int32 AttackingCardId = INDEX_NONE;

	// What's under the mouse while aiming: a board card, or ACardTable::PlayerTarget.
	int32 AttackTargetId = INDEX_NONE;
	bool bAttackTargetValid = false;

	UFUNCTION(Server, Reliable)
	void ServerAttack(int32 AttackerId, int32 TargetId);

	// Starts aiming if the mouse is on one of this player's table cards that may attack.
	bool TryBeginAttack(APlayerController& PlayerController);
	void UpdateAttackTargeting(APlayerController& PlayerController);
	void EndAttackTargeting();
	void DrawTargetArrow(const FVector& Start, const FVector& End, bool bValid);

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

	UFUNCTION(Server, Reliable)
	void ServerDebugDamage(int32 Amount);

	UFUNCTION(Server, Reliable)
	void ServerDebugMana(int32 Amount);

	UFUNCTION(Server, Reliable)
	void ServerEndTurn();

	UFUNCTION(Server, Reliable)
	void ServerConfirmMulligan(const TArray<int32>& ReplaceInstanceIds);

	// Replicated to everyone, so the other player can see "waiting for opponent".
	UPROPERTY(Replicated)
	bool bMulliganConfirmed = false;

	// Local: hand cards (InstanceId) the player clicked to replace.
	TSet<int32> MulliganMarked;

	// Number of cards in hand, public (the opponent sees that many backs).
	UPROPERTY(Replicated)
	int32 HandCount = 0;

	// Owner -> server -> opponent.
	UPROPERTY(Replicated)
	FHandPose HandPose;

	FHandPose LastSentPose;
	float PoseSendTimer = 0.0f;

	UFUNCTION(Server, Unreliable)
	void ServerSetHandPose(const FHandPose& NewPose);

	// Opponent's machine: card backs mirroring this player's hand.
	UPROPERTY(Transient)
	TArray<TObjectPtr<ACardActor>> HeldCards;

	void SendHandPose(float DeltaSeconds);
	void UpdateHeldHand(float DeltaSeconds);
	FTransform GetHeldCardTransform(int32 Index, int32 Count) const;

	bool IsMulliganPhase() const;

	// Mana left this turn (The Coin can push it above MaxMana).
	UPROPERTY(Replicated)
	int32 Mana = 0;

	UPROPERTY(Replicated)
	int32 MaxMana = 0;

	void UpdateStatusText();

	UPROPERTY(ReplicatedUsing = OnRep_Health)
	int32 Health = 0;

	UFUNCTION()
	void OnRep_Health(int32 OldHealth);

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
