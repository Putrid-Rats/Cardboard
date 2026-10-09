#include "CardTable.h"

#include "CardActor.h"
#include "CardboardSettings.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "SeatedPawn.h"

ACardTable::ACardTable()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;

	CardClass = ACardActor::StaticClass();
}

void ACardTable::PlaceCard(int32 Seat, int32 InsertIndex, FName CardId)
{
	TArray<FBoardCard>* Row = GetRow(Seat);

	if (!HasAuthority() || !Row || Row->Num() >= MaxRowSize)
	{
		return;
	}

	FBoardCard NewCard;
	NewCard.InstanceId = NextInstanceId++;
	NewCard.CardId = CardId;

	if (const FCardDefinition* Definition = UCardboardSettings::FindCard(CardId))
	{
		NewCard.Attack = Definition->Attack;
		NewCard.Health = Definition->Health;
		NewCard.Trait = Definition->Trait;
	}

	// Can't attack the turn it's played.
	NewCard.bCanAttack = false;
	NewCard.bStealthed = NewCard.Trait == ECardTrait::Stealth;

	Row->Insert(NewCard, FMath::Clamp(InsertIndex, 0, Row->Num()));

	// OnRep doesn't run on the server, and the listen server host needs the visuals too.
	SyncBoardVisuals();
}

bool ACardTable::CanAttack(int32 Seat, int32 AttackerId, int32 TargetId) const
{
	const TArray<FBoardCard>* OwnRow = GetRow(Seat);
	const TArray<FBoardCard>* EnemyRow = GetRow(1 - Seat);

	if (!OwnRow || !EnemyRow)
	{
		return false;
	}

	const FBoardCard* Attacker = OwnRow->FindByPredicate([AttackerId](const FBoardCard& Card) { return Card.InstanceId == AttackerId; });

	if (!Attacker || !Attacker->bCanAttack || Attacker->Attack <= 0)
	{
		return false;
	}

	const bool bIgnoresTaunt = Attacker->Trait == ECardTrait::Fly;
	const bool bEnemyHasTaunt = EnemyRow->ContainsByPredicate([](const FBoardCard& Card) { return Card.IsTaunting(); });

	if (TargetId == PlayerTarget)
	{
		return bIgnoresTaunt || !bEnemyHasTaunt;
	}

	const FBoardCard* Target = EnemyRow->FindByPredicate([TargetId](const FBoardCard& Card) { return Card.InstanceId == TargetId; });

	if (!Target || Target->bStealthed)
	{
		return false;
	}

	return bIgnoresTaunt || !bEnemyHasTaunt || Target->IsTaunting();
}

void ACardTable::Attack(int32 Seat, int32 AttackerId, int32 TargetId)
{
	if (!HasAuthority() || !CanAttack(Seat, AttackerId, TargetId))
	{
		return;
	}

	TArray<FBoardCard>& OwnRow = *GetRow(Seat);
	TArray<FBoardCard>& EnemyRow = *GetRow(1 - Seat);

	FBoardCard* Attacker = OwnRow.FindByPredicate([AttackerId](const FBoardCard& Card) { return Card.InstanceId == AttackerId; });

	// Attacking uses up this turn's attack and reveals a Stealth card.
	Attacker->bCanAttack = false;
	Attacker->bStealthed = false;

	if (TargetId == PlayerTarget)
	{
		for (TActorIterator<ASeatedPawn> It(GetWorld()); It; ++It)
		{
			if (It->SeatIndex == 1 - Seat)
			{
				It->ApplyPlayerDamage(Attacker->Attack);
				break;
			}
		}
	}
	else
	{
		FBoardCard* Target = EnemyRow.FindByPredicate([TargetId](const FBoardCard& Card) { return Card.InstanceId == TargetId; });

		// Both cards hit each other.
		Target->Health -= Attacker->Attack;
		Attacker->Health -= Target->Attack;
	}

	MulticastAttackPerformed(Seat, AttackerId, TargetId, GetAttackTargetLocation(Seat, TargetId));

	auto IsDead = [](const FBoardCard& Card) { return Card.Health <= 0; };
	OwnRow.RemoveAll(IsDead);
	EnemyRow.RemoveAll(IsDead);

	SyncBoardVisuals();
}

void ACardTable::ReadyCardsForTurn(int32 Seat)
{
	TArray<FBoardCard>* Row = GetRow(Seat);

	if (!HasAuthority() || !Row)
	{
		return;
	}

	for (FBoardCard& Card : *Row)
	{
		Card.bCanAttack = true;
	}

	SyncBoardVisuals();
}

FVector ACardTable::GetAttackTargetLocation(int32 Seat, int32 TargetId) const
{
	if (TargetId != PlayerTarget)
	{
		if (const ACardActor* TargetCard = GetBoardCardActor(TargetId))
		{
			return TargetCard->GetActorLocation();
		}
	}

	// The player: the middle of their side of the table, just behind their row.
	const float TowardsSeat = RowDistanceFromCentre + ACardActor::CardHeight * 2.0f;
	const float LocalX = (1 - Seat) == 0 ? -TowardsSeat : TowardsSeat;

	return GetActorTransform().TransformPosition(FVector(LocalX, 0.0f, BoardHeight));
}

void ACardTable::MulticastAttackPerformed_Implementation(int32 Seat, int32 AttackerId, int32 TargetId, FVector_NetQuantize TargetLocation)
{
	// Lunge the attacker most of the way towards its target and back.
	LungeCardId = AttackerId;
	LungeTargetLocation = TargetLocation;
	LungeElapsed = 0.0f;

	OnAttackPerformed(Seat, AttackerId, TargetId);
}

bool ACardTable::FindBoardCard(int32 InstanceId, int32& OutSeat, FBoardCard& OutCard) const
{
	for (int32 Seat = 0; Seat < 2; ++Seat)
	{
		if (const FBoardCard* Card = GetRow(Seat)->FindByPredicate([InstanceId](const FBoardCard& Entry) { return Entry.InstanceId == InstanceId; }))
		{
			OutSeat = Seat;
			OutCard = *Card;
			return true;
		}
	}

	return false;
}

bool ACardTable::GetTablePoint(const FVector& RayOrigin, const FVector& RayDirection, FVector& OutWorldPoint) const
{
	// Intersect the mouse ray with the table surface.
	const FVector PlaneOrigin = GetActorTransform().TransformPosition(FVector(0.0f, 0.0f, BoardHeight));
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

	OutWorldPoint = RayOrigin + RayDirection * Distance;
	return true;
}

bool ACardTable::IsInPlayerArea(int32 Seat, const FVector& WorldPoint) const
{
	// Seat 0 sits on the -X side, seat 1 on the +X side. Behind the row = further out than the row's back edge.
	const FVector LocalPoint = GetActorTransform().InverseTransformPosition(WorldPoint);
	const float TowardsSeat = Seat == 0 ? -LocalPoint.X : LocalPoint.X;

	return TowardsSeat > RowDistanceFromCentre + ACardActor::CardHeight * 0.5f;
}

bool ACardTable::GetInsertIndexAt(int32 Seat, const FVector& RayOrigin, const FVector& RayDirection, int32& OutInsertIndex) const
{
	const TArray<FBoardCard>* Row = GetRow(Seat);
	FVector WorldPoint;

	if (!Row || Row->Num() >= MaxRowSize || !GetTablePoint(RayOrigin, RayDirection, WorldPoint))
	{
		return false;
	}

	const FVector LocalHit = GetActorTransform().InverseTransformPosition(WorldPoint);

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

ACardActor* ACardTable::GetBoardCardActor(int32 InstanceId) const
{
	return BoardCardActors.FindRef(InstanceId);
}

void ACardTable::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FTransform& TableTransform = GetActorTransform();
	const float Time = GetWorld()->GetTimeSeconds();

	if (LungeCardId != INDEX_NONE)
	{
		LungeElapsed += DeltaSeconds;

		if (LungeElapsed >= LungeDuration)
		{
			LungeCardId = INDEX_NONE;
		}
	}

	// Slide every card towards its slot, so inserted cards push the others aside smoothly.
	for (int32 Seat = 0; Seat < 2; ++Seat)
	{
		const TArray<FBoardCard>* Row = GetRow(Seat);

		for (int32 Index = 0; Index < Row->Num(); ++Index)
		{
			const FBoardCard& BoardCard = (*Row)[Index];
			ACardActor* Card = BoardCardActors.FindRef(BoardCard.InstanceId);

			if (!Card)
			{
				continue;
			}

			FTransform Target = GetBoardSlotTransform(Seat, Index) * TableTransform;

			// Fly: hover a little above the table, bobbing out of step with the other flyers.
			if (BoardCard.Trait == ECardTrait::Fly)
			{
				const float Bob = FMath::Sin(Time * 2.0f + BoardCard.InstanceId) * 0.3f;
				Target.AddToTranslation(GetActorUpVector() * (FlyHeight + Bob));
			}

			if (BoardCard.InstanceId == LungeCardId)
			{
				// Out and back along a sine: 0 -> 0.8 of the way -> 0.
				const float Alpha = FMath::Sin(PI * LungeElapsed / LungeDuration) * 0.8f;
				Card->SetActorLocationAndRotation(FMath::Lerp(Target.GetLocation(), LungeTargetLocation, Alpha), Target.Rotator());
				continue;
			}

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
			const FBoardCard& BoardCard = (*Row)[Index];
			OnBoard.Add(BoardCard.InstanceId);

			ACardActor* Card = BoardCardActors.FindRef(BoardCard.InstanceId);

			if (!Card && CardClass)
			{
				// New card: appear a little above its slot and settle down onto the table.
				FTransform SpawnTransform = GetBoardSlotTransform(Seat, Index) * GetActorTransform();
				SpawnTransform.AddToTranslation(GetActorUpVector() * 10.0f);

				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = this;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

				Card = GetWorld()->SpawnActor<ACardActor>(CardClass, SpawnTransform, SpawnParams);

				if (Card)
				{
					Card->InstanceId = BoardCard.InstanceId;
					Card->SetCard(BoardCard.CardId);
					BoardCardActors.Add(BoardCard.InstanceId, Card);
				}
			}

			if (Card)
			{
				Card->SetBoardState(BoardCard.Attack, BoardCard.Health, BoardCard.Trait, BoardCard.bStealthed, BoardCard.bCanAttack);
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
