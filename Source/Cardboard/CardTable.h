#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CardTable.generated.h"

class ACardActor;

// One card lying on the table. Only an id for now; card data (cost, attack, health) comes later.
USTRUCT(BlueprintType)
struct FBoardCard
{
	GENERATED_BODY()

	// Unique per card on the board, so clients can tell which card moved when one is inserted.
	UPROPERTY(BlueprintReadOnly, Category = "Board")
	int32 InstanceId = 0;
};

// The table both players sit at. Holds one row of played cards per seat.
// The rows live on the server and replicate to both players; every machine
// spawns its own card visuals from them and slides them into place.
// Base class of BP_Table (seat arrows, table mesh and GetSeatTransform stay in the Blueprint).
UCLASS()
class CARDBOARD_API ACardTable : public AActor
{
	GENERATED_BODY()

public:

	ACardTable();

	UPROPERTY(EditDefaultsOnly, Category = "Board")
	TSubclassOf<ACardActor> CardClass;

	UPROPERTY(EditDefaultsOnly, Category = "Board", meta = (ClampMin = 1))
	int32 MaxRowSize = 7;

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

	// Server only. Puts a new card into the seat's row at InsertIndex (0 = leftmost from that seat).
	void PlaceCard(int32 Seat, int32 InsertIndex);

	// Where in the seat's row a card dropped along this mouse ray would go.
	// False if the ray misses the table plane or the row is full.
	bool GetInsertIndexAt(int32 Seat, const FVector& RayOrigin, const FVector& RayDirection, int32& OutInsertIndex) const;

	// Local only: opens a gap in the seat's row while a card is dragged over the table. INDEX_NONE closes it.
	void SetPlacementPreview(int32 Seat, int32 InsertIndex);

	UFUNCTION(BlueprintPure, Category = "Board")
	int32 GetRowSize(int32 Seat) const;

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

	UFUNCTION()
	void OnRep_Rows();

	TArray<FBoardCard>* GetRow(int32 Seat);
	const TArray<FBoardCard>* GetRow(int32 Seat) const;

	// Spawns visuals for new cards and removes visuals of cards that left the board.
	void SyncBoardVisuals();

	// Card position in table space, taking the placement gap into account.
	FTransform GetBoardSlotTransform(int32 Seat, int32 Index) const;

	// Position along the row as seen from the seat: negative = that player's left.
	float GetSlotOffset(int32 Slot, int32 SlotCount) const;
};
