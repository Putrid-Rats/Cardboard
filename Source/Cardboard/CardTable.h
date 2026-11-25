#pragma once

#include "CoreMinimal.h"
#include "CardDefinition.h"
#include "GameFramework/Actor.h"
#include "CardTable.generated.h"

class ACardActor;

// One card lying on the table, with its current combat state.
USTRUCT(BlueprintType)
struct FBoardCard
{
	GENERATED_BODY()

	// Unique per card on the board, so clients can tell which card moved when one is inserted.
	UPROPERTY(BlueprintReadOnly, Category = "Board")
	int32 InstanceId = 0;

	// Row name in DT_Cards.
	UPROPERTY(BlueprintReadOnly, Category = "Board")
	FName CardId;

	UPROPERTY(BlueprintReadOnly, Category = "Board")
	int32 Attack = 0;

	// Current health; the card is removed when it reaches 0.
	UPROPERTY(BlueprintReadOnly, Category = "Board")
	int32 Health = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Board")
	ECardTrait Trait = ECardTrait::None;

	// False the turn it's played and after it has attacked; true again at its owner's next turn.
	UPROPERTY(BlueprintReadOnly, Category = "Board")
	bool bCanAttack = false;

	// Stealth cards start hidden from targeting and lose it when they attack.
	UPROPERTY(BlueprintReadOnly, Category = "Board")
	bool bStealthed = false;

	bool IsTaunting() const { return Trait == ECardTrait::Taunt && !bStealthed; }
};

// The table both players sit at. Holds one row of played cards per seat, and runs combat between them.
// The rows live on the server and replicate to both players; every machine
// spawns its own card visuals from them and slides them into place.
// Base class of BP_Table (seat arrows, table mesh and GetSeatTransform stay in the Blueprint).
//
// Combat rules (server):
// - a card can't attack the turn it's played, and attacks once per turn
// - attacking a card: both deal their Attack to each other; attacking the opponent damages their health
// - Taunt: while the opponent has a Taunt card, only Taunt cards can be attacked
// - Fly: ignores Taunt
// - Stealth: can't be targeted until it attacks
UCLASS()
class CARDBOARD_API ACardTable : public AActor
{
	GENERATED_BODY()

public:

	ACardTable();

	// Target ID meaning "the opponent player" instead of a card.
	static constexpr int32 PlayerTarget = INDEX_NONE;

	UPROPERTY(EditDefaultsOnly, Category = "Board")
	TSubclassOf<ACardActor> CardClass;

	UPROPERTY(EditDefaultsOnly, Category = "Board", meta = (ClampMin = 1))
	int32 MaxRowSize = 6;

	// Height of a card lying on the table, relative to the table actor (just above the table top).
	UPROPERTY(EditDefaultsOnly, Category = "Board")
	float BoardHeight = 77.6f;

	// How far each row sits from the middle of the table, towards its own seat.
	UPROPERTY(EditDefaultsOnly, Category = "Board")
	float RowDistanceFromCentre = 12.0f;

	// Distance between card centres in a row.
	UPROPERTY(EditDefaultsOnly, Category = "Board")
	float BoardCardSpacing = 8.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Board")
	float BoardCardMoveSpeed = 10.0f;

	// Fly cards hover this high above the table, bobbing gently.
	UPROPERTY(EditDefaultsOnly, Category = "Board")
	float FlyHeight = 2.0f;

	// Server only. Puts a new card into the seat's row at InsertIndex (0 = leftmost from that seat).
	void PlaceCard(int32 Seat, int32 InsertIndex, FName CardId);

	// Server only. The seat's card AttackerId attacks TargetId (a card in the other row, or PlayerTarget).
	// Ignored if the rules don't allow it.
	void Attack(int32 Seat, int32 AttackerId, int32 TargetId);

	// Server only, at the start of the seat's turn: all its cards may attack again.
	void ReadyCardsForTurn(int32 Seat);

	// Server only: removes every card from both rows (rematch).
	void ClearBoard();

	// Whether the seat's card AttackerId may attack TargetId right now (ignores whose turn it is).
	bool CanAttack(int32 Seat, int32 AttackerId, int32 TargetId) const;

	// Seat and data of a board card, by InstanceId. False if it's not on the table.
	bool FindBoardCard(int32 InstanceId, int32& OutSeat, FBoardCard& OutCard) const;

	// Where in the seat's row a card dropped along this mouse ray would go.
	// False if the ray misses the table plane or the row is full.
	bool GetInsertIndexAt(int32 Seat, const FVector& RayOrigin, const FVector& RayDirection, int32& OutInsertIndex) const;

	// Where the mouse ray hits the table top, in world space. False if it misses the plane.
	bool GetTablePoint(const FVector& RayOrigin, const FVector& RayDirection, FVector& OutWorldPoint) const;

	// Whether a table point lies on the seat's side, behind its row (the area that stands for the player).
	bool IsInPlayerArea(int32 Seat, const FVector& WorldPoint) const;

	// Local only: opens a gap in the seat's row while a card is dragged over the table. INDEX_NONE closes it.
	void SetPlacementPreview(int32 Seat, int32 InsertIndex);

	UFUNCTION(BlueprintPure, Category = "Board")
	int32 GetRowSize(int32 Seat) const;

	// This machine's visual for a board card, or null.
	ACardActor* GetBoardCardActor(int32 InstanceId) const;

	// Every machine: an attack just happened (TargetId -1 = the player). Hook for hit effects and sounds.
	UFUNCTION(BlueprintImplementableEvent, Category = "Board")
	void OnAttackPerformed(int32 Seat, int32 AttackerId, int32 TargetId);

protected:

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;

private:

	UPROPERTY(ReplicatedUsing = OnRep_Rows)
	TArray<FBoardCard> RowSeat0;

	UPROPERTY(ReplicatedUsing = OnRep_Rows)
	TArray<FBoardCard> RowSeat1;

	int32 NextInstanceId = 1;

	// This machine's card actors, by InstanceId.
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<ACardActor>> BoardCardActors;

	int32 PreviewSeat = INDEX_NONE;
	int32 PreviewIndex = INDEX_NONE;

	// Attack animation: the attacker darts towards its target and back.
	int32 LungeCardId = INDEX_NONE;
	FVector LungeTargetLocation = FVector::ZeroVector;
	float LungeElapsed = 0.0f;
	float LungeDuration = 0.4f;

	// Tells every machine to play the attack animation.
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastAttackPerformed(int32 Seat, int32 AttackerId, int32 TargetId, FVector_NetQuantize TargetLocation);

	// Where an attack on TargetId should aim (the card, or the player's side of the table).
	FVector GetAttackTargetLocation(int32 Seat, int32 TargetId) const;

	UFUNCTION()
	void OnRep_Rows();

	TArray<FBoardCard>* GetRow(int32 Seat);
	const TArray<FBoardCard>* GetRow(int32 Seat) const;

	// Spawns visuals for new cards, updates stats and traits, removes visuals of cards that left the board.
	void SyncBoardVisuals();

	// Card position in table space, taking the placement gap into account.
	FTransform GetBoardSlotTransform(int32 Seat, int32 Index) const;

	// Position along the row as seen from the seat: negative = that player's left.
	float GetSlotOffset(int32 Slot, int32 SlotCount) const;
};
