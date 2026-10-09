#include "CardTable.h"

#include "CardActor.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

ACardTable::ACardTable()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;

	CardClass = ACardActor::StaticClass();
}

void ACardTable::PlaceCard(int32 Seat, int32 InsertIndex)
{
	TArray<FBoardCard>* Row = GetRow(Seat);

	if (!HasAuthority() || !Row || Row->Num() >= MaxRowSize)
	{
		return;
	}

	FBoardCard NewCard;
	NewCard.InstanceId = NextInstanceId++;

	Row->Insert(NewCard, FMath::Clamp(InsertIndex, 0, Row->Num()));

	// OnRep doesn't run on the server, and the listen server host needs the visuals too.
	SyncBoardVisuals();
}

bool ACardTable::GetInsertIndexAt(int32 Seat, const FVector& RayOrigin, const FVector& RayDirection, int32& OutInsertIndex) const
{
	const TArray<FBoardCard>* Row = GetRow(Seat);

	if (!Row || Row->Num() >= MaxRowSize)
	{
		return false;
	}

	// Intersect the mouse ray with the table surface.
	const FTransform& TableTransform = GetActorTransform();
	const FVector PlaneOrigin = TableTransform.TransformPosition(FVector(0.0f, 0.0f, BoardHeight));
	const FVector PlaneNormal = GetActorUpVector();
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

	const FVector LocalHit = TableTransform.InverseTransformPosition(RayOrigin + RayDirection * Distance);

	// Seat 0 looks along +X, so its right is +Y. Seat 1 sits opposite, so its right is -Y.
	const float HitOffset = Seat == 0 ? LocalHit.Y : -LocalHit.Y;

	// The new card goes after every card whose centre is left of the mouse.
	OutInsertIndex = 0;

	for (int32 Index = 0; Index < Row->Num(); ++Index)
	{
		if (GetSlotOffset(Index, Row->Num()) < HitOffset)
		{
			OutInsertIndex = Index + 1;
		}
	}

	return true;
}

void ACardTable::SetPlacementPreview(int32 Seat, int32 InsertIndex)
{
	PreviewSeat = InsertIndex == INDEX_NONE ? INDEX_NONE : Seat;
	PreviewIndex = InsertIndex;
}

int32 ACardTable::GetRowSize(int32 Seat) const
{
	const TArray<FBoardCard>* Row = GetRow(Seat);
	return Row ? Row->Num() : 0;
}

void ACardTable::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FTransform& TableTransform = GetActorTransform();

	// Slide every card towards its slot, so inserted cards push the others aside smoothly.
	for (int32 Seat = 0; Seat < 2; ++Seat)
	{
		const TArray<FBoardCard>* Row = GetRow(Seat);

		for (int32 Index = 0; Index < Row->Num(); ++Index)
		{
			ACardActor* Card = BoardCardActors.FindRef((*Row)[Index].InstanceId);

			if (!Card)
			{
				continue;
			}

			const FTransform Target = GetBoardSlotTransform(Seat, Index) * TableTransform;

			Card->SetActorLocationAndRotation(
				FMath::VInterpTo(Card->GetActorLocation(), Target.GetLocation(), DeltaSeconds, BoardCardMoveSpeed),
				FMath::RInterpTo(Card->GetActorRotation(), Target.Rotator(), DeltaSeconds, BoardCardMoveSpeed)
			);
		}
	}
}

void ACardTable::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (const TPair<int32, TObjectPtr<ACardActor>>& Pair : BoardCardActors)
	{
		if (Pair.Value)
		{
			Pair.Value->Destroy();
		}
	}

	BoardCardActors.Empty();

	Super::EndPlay(EndPlayReason);
}

void ACardTable::OnRep_Rows()
{
	SyncBoardVisuals();
}

TArray<FBoardCard>* ACardTable::GetRow(int32 Seat)
{
	return Seat == 0 ? &RowSeat0 : Seat == 1 ? &RowSeat1 : nullptr;
}

const TArray<FBoardCard>* ACardTable::GetRow(int32 Seat) const
{
	return Seat == 0 ? &RowSeat0 : Seat == 1 ? &RowSeat1 : nullptr;
}

void ACardTable::SyncBoardVisuals()
{
	TSet<int32> OnBoard;

	for (int32 Seat = 0; Seat < 2; ++Seat)
	{
		const TArray<FBoardCard>* Row = GetRow(Seat);

		for (int32 Index = 0; Index < Row->Num(); ++Index)
		{
			const int32 InstanceId = (*Row)[Index].InstanceId;
			OnBoard.Add(InstanceId);

			if (BoardCardActors.Contains(InstanceId) || !CardClass)
			{
				continue;
			}

			// New card: appear a little above its slot and settle down onto the table.
			FTransform SpawnTransform = GetBoardSlotTransform(Seat, Index) * GetActorTransform();
			SpawnTransform.AddToTranslation(GetActorUpVector() * 10.0f);

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			if (ACardActor* Card = GetWorld()->SpawnActor<ACardActor>(CardClass, SpawnTransform, SpawnParams))
			{
				BoardCardActors.Add(InstanceId, Card);
			}
		}
	}

	for (auto It = BoardCardActors.CreateIterator(); It; ++It)
	{
		if (!OnBoard.Contains(It.Key()))
		{
			if (It.Value())
			{
				It.Value()->Destroy();
			}

			It.RemoveCurrent();
		}
	}
}

FTransform ACardTable::GetBoardSlotTransform(int32 Seat, int32 Index) const
{
	int32 Slot = Index;
	int32 SlotCount = GetRowSize(Seat);

	// While a card is dragged over this row, leave an empty slot where it would land.
	if (Seat == PreviewSeat && PreviewIndex != INDEX_NONE)
	{
		Slot = Index < PreviewIndex ? Index : Index + 1;
		++SlotCount;
	}

	const float Offset = GetSlotOffset(Slot, SlotCount);

	// Seat 0 is on the -X side and looks along +X; seat 1 is mirrored.
	const float RowX = Seat == 0 ? -RowDistanceFromCentre : RowDistanceFromCentre;
	const float RowY = Seat == 0 ? Offset : -Offset;

	// Lying flat, face up, with the top of the card pointing away from its owner.
	const FRotator Rotation(-90.0f, Seat == 0 ? 0.0f : 180.0f, 0.0f);

	return FTransform(Rotation, FVector(RowX, RowY, BoardHeight));
}

float ACardTable::GetSlotOffset(int32 Slot, int32 SlotCount) const
{
	// Centred: one card sits in the middle, more cards spread out evenly on both sides.
	return (Slot - (SlotCount - 1) * 0.5f) * BoardCardSpacing;
}

void ACardTable::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACardTable, RowSeat0);
	DOREPLIFETIME(ACardTable, RowSeat1);
}
