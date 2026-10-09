#include "SeatedPawn.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Camera/CameraComponent.h"
#include "CardActor.h"
#include "CardboardSettings.h"
#include "CardDefinition.h"
#include "CardGameState.h"
#include "Components/TextRenderComponent.h"
#include "CardTable.h"
#include "Engine/DataTable.h"
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

	// Top-left corner of the view, in front of the camera. Owner-only so the opponent never sees it.
	StatusText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StatusText"));
	StatusText->SetupAttachment(Camera);
	StatusText->SetRelativeLocation(FVector(30.0f, -26.0f, 15.0f));
	StatusText->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	StatusText->SetHorizontalAlignment(EHTA_Left);
	StatusText->SetVerticalAlignment(EVRTA_TextTop);
	StatusText->SetWorldSize(1.0f);
	StatusText->SetTextRenderColor(FColor::White);
	StatusText->SetOnlyOwnerSee(true);
	StatusText->SetCastShadow(false);
	StatusText->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

	if (HasAuthority())
	{
		Health = MaxHealth;
		OnRep_Health(0);
	}
}

void ASeatedPawn::ApplyPlayerDamage(int32 Amount)
{
	const ACardGameState* CardGameState = GetWorld()->GetGameState<ACardGameState>();

	if (!HasAuthority() || Amount <= 0 || Health <= 0 || (CardGameState && CardGameState->IsMatchOver()))
	{
		return;
	}

	const int32 OldHealth = Health;
	Health = FMath::Max(Health - Amount, 0);
	OnRep_Health(OldHealth);

	if (Health == 0)
	{
		if (ACardGameState* GameState = GetWorld()->GetGameState<ACardGameState>())
		{
			GameState->EndMatch(1 - SeatIndex);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("SeatedPawn: no CardGameState, set it as GameState Class in GM_Gameplay_TCG"));
		}
	}
}

void ASeatedPawn::OnRep_Health(int32 OldHealth)
{
	OnHealthChanged(Health, OldHealth);
}

void ASeatedPawn::NotifyMatchEnded(int32 WinningSeat)
{
	OnMatchEnded(SeatIndex == WinningSeat);
}

void ASeatedPawn::DebugDamage(int32 Amount)
{
	ServerDebugDamage(Amount);
}

void ASeatedPawn::ServerDebugDamage_Implementation(int32 Amount)
{
	ApplyPlayerDamage(Amount);
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

	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (bInTableView && PlayerController->WasInputKeyJustPressed(EKeys::E))
		{
			if (IsMulliganPhase())
			{
				ConfirmMulligan();
			}
			else
			{
				EndTurn();
			}
		}
	}

	UpdateStatusText();
}

void ASeatedPawn::UpdateStatusText()
{
	const ACardGameState* CardGameState = GetWorld()->GetGameState<ACardGameState>();
	FString Status;

	if (!CardGameState || !CardGameState->IsMatchStarted())
	{
		Status = TEXT("Waiting for the other player...");
	}
	else if (CardGameState->IsMatchOver())
	{
		Status = CardGameState->GetWinningSeat() == SeatIndex ? TEXT("You won!") : TEXT("You lost");
	}
	else if (CardGameState->GetMatchPhase() == EMatchPhase::CoinFlip)
	{
		Status = CardGameState->GetFirstSeat() == SeatIndex ? TEXT("Coin: you go first") : TEXT("Coin: you go second");
	}
	else if (CardGameState->GetMatchPhase() == EMatchPhase::Mulligan)
	{
		Status = bMulliganConfirmed
			? TEXT("Mulligan: waiting for the opponent...")
			: TEXT("Mulligan: click cards to replace, E = confirm");
	}
	else
	{
		Status = IsMyTurn() ? TEXT("Your turn (E = end turn)") : TEXT("Opponent's turn");
	}

	Status += FString::Printf(TEXT("\nMana %d/%d   Deck %d   Health %d"), Mana, MaxMana, DeckCount, Health);
	StatusText->SetText(FText::FromString(Status));
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

void ASeatedPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Server: the player just sat down. Shuffle their deck once; the GameState deals when both are seated.
	if (!bDeckBuilt)
	{
		bDeckBuilt = true;
		BuildDeck();
	}

	if (ACardGameState* CardGameState = GetWorld()->GetGameState<ACardGameState>())
	{
		CardGameState->TryStartMatch();
	}
}

void ASeatedPawn::BeginTurn(int32 ManaCap)
{
	MaxMana = FMath::Min(MaxMana + 1, ManaCap);
	Mana = MaxMana;
	DrawCards(1);
}

bool ASeatedPawn::IsMyTurn() const
{
	const ACardGameState* CardGameState = GetWorld()->GetGameState<ACardGameState>();
	return CardGameState && !CardGameState->IsMatchOver() && CardGameState->GetCurrentTurnSeat() == SeatIndex;
}

void ASeatedPawn::EndTurn()
{
	if (IsMyTurn())
	{
		ServerEndTurn();
	}
}

void ASeatedPawn::ServerEndTurn_Implementation()
{
	if (ACardGameState* CardGameState = GetWorld()->GetGameState<ACardGameState>())
	{
		CardGameState->EndTurn(SeatIndex);
	}
}

void ASeatedPawn::NotifyPhaseChanged(EMatchPhase NewPhase)
{
	if (NewPhase != EMatchPhase::Mulligan)
	{
		MulliganMarked.Reset();
		return;
	}

	// Show the opening hand straight away.
	if (IsLocallyControlled())
	{
		SetTableView(true);
	}

	OnMulliganStarted();
}

bool ASeatedPawn::IsMulliganPhase() const
{
	const ACardGameState* CardGameState = GetWorld()->GetGameState<ACardGameState>();
	return CardGameState && CardGameState->GetMatchPhase() == EMatchPhase::Mulligan && !bMulliganConfirmed;
}

void ASeatedPawn::ConfirmMulligan()
{
	if (IsMulliganPhase())
	{
		ServerConfirmMulligan(MulliganMarked.Array());
	}
}

void ASeatedPawn::ServerConfirmMulligan_Implementation(const TArray<int32>& ReplaceInstanceIds)
{
	if (!IsMulliganPhase())
	{
		return;
	}

	TArray<FName> Returned;

	for (const int32 InstanceId : ReplaceInstanceIds)
	{
		const int32 HandIndex = Hand.IndexOfByPredicate([InstanceId](const FHandCard& HandCard)
		{
			return HandCard.InstanceId == InstanceId;
		});

		if (HandIndex != INDEX_NONE)
		{
			Returned.Add(Hand[HandIndex].CardId);
			Hand.RemoveAt(HandIndex);
		}
	}

	// Draw the replacements first, so the thrown-back cards can't come straight back.
	DrawCards(Returned.Num());

	for (const FName CardId : Returned)
	{
		Deck.Insert(CardId, FMath::RandRange(0, Deck.Num()));
	}

	DeckCount = Deck.Num();
	SyncHandVisuals();

	bMulliganConfirmed = true;

	if (ACardGameState* CardGameState = GetWorld()->GetGameState<ACardGameState>())
	{
		CardGameState->NotifyMulliganConfirmed();
	}
}

void ASeatedPawn::NotifyCoinFlipped(int32 FirstSeat)
{
	OnCoinFlipped(SeatIndex == FirstSeat);
}

void ASeatedPawn::NotifyTurnStarted(int32 TurnSeat)
{
	OnTurnStarted(SeatIndex == TurnSeat);
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

	for (ACardActor* Card : PendingPlayCards)
	{
		if (Card)
		{
			Card->Destroy();
		}
	}

	HandCards.Empty();
	PendingPlayCards.Empty();

	Super::EndPlay(EndPlayReason);
}

void ASeatedPawn::BuildDeck()
{
	Deck.Reset();

	// For now the deck is every card in the sheet once, except The Coin.
	if (const UDataTable* CardTable = UCardboardSettings::GetCardDataTable())
	{
		Deck = CardTable->GetRowNames();
		Deck.Remove(UCardboardSettings::GetCoinCardId());
	}

	// Fisher-Yates shuffle.
	for (int32 Index = Deck.Num() - 1; Index > 0; --Index)
	{
		Deck.Swap(Index, FMath::RandRange(0, Index));
	}

	DeckCount = Deck.Num();
}

void ASeatedPawn::DrawCards(int32 Count)
{
	if (!HasAuthority())
	{
		return;
	}

	for (int32 Drawn = 0; Drawn < Count && Deck.Num() > 0; ++Drawn)
	{
		AddCardToHand(Deck.Pop());
	}

	DeckCount = Deck.Num();
}

void ASeatedPawn::AddCardToHand(FName CardId)
{
	// Full hand: the card is lost, like in Hearthstone.
	if (!HasAuthority() || Hand.Num() >= MaxHandSize)
	{
		return;
	}

	FHandCard NewCard;
	NewCard.InstanceId = NextHandInstanceId++;
	NewCard.CardId = CardId;
	Hand.Add(NewCard);

	// OnRep doesn't run on the server, and the listen server host needs its visuals too.
	SyncHandVisuals();
}

void ASeatedPawn::DebugDrawCard()
{
	ServerDebugDrawCard();
}

void ASeatedPawn::ServerDebugDrawCard_Implementation()
{
	DrawCards(1);
}

void ASeatedPawn::OnRep_Hand()
{
	SyncHandVisuals();
}

void ASeatedPawn::SyncHandVisuals()
{
	// Only the owning player shows their hand. On the server that means the host's own pawn.
	if (HasAuthority() && !IsLocallyControlled())
	{
		return;
	}

	TSet<int32> InHand;

	for (const FHandCard& HandCard : Hand)
	{
		InHand.Add(HandCard.InstanceId);
	}

	// Cards that left the hand: played (the server accepted) or discarded.
	for (int32 Index = HandCards.Num() - 1; Index >= 0; --Index)
	{
		ACardActor* Card = HandCards[Index];

		if (Card && InHand.Contains(Card->InstanceId))
		{
			continue;
		}

		if (Card == HoveredCard)
		{
			HoveredCard = nullptr;
		}

		if (Card == DraggedCard)
		{
			DraggedCard = nullptr;
		}

		if (Card)
		{
			Card->Destroy();
		}

		HandCards.RemoveAt(Index);
	}

	for (int32 Index = PendingPlayCards.Num() - 1; Index >= 0; --Index)
	{
		ACardActor* Card = PendingPlayCards[Index];

		if (!Card || !InHand.Contains(Card->InstanceId))
		{
			if (Card)
			{
				Card->Destroy();
			}

			PendingPlayCards.RemoveAt(Index);
		}
	}

	// New cards: added on the right, rising from below the hand.
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (const FHandCard& HandCard : Hand)
	{
		auto HasInstance = [&HandCard](const ACardActor* Card) { return Card && Card->InstanceId == HandCard.InstanceId; };

		if (!CardClass || HandCards.ContainsByPredicate(HasInstance) || PendingPlayCards.ContainsByPredicate(HasInstance))
		{
			continue;
		}

		ACardActor* Card = GetWorld()->SpawnActor<ACardActor>(CardClass, HandRoot->GetComponentTransform(), SpawnParams);

		if (!Card)
		{
			continue;
		}

		Card->InstanceId = HandCard.InstanceId;
		Card->SetCard(HandCard.CardId);
		Card->AttachToComponent(HandRoot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Card->SetActorRelativeLocation(FVector(0.0f, 0.0f, -HandHiddenDrop));
		HandCards.Add(Card);
	}

	// Applies the hand's hidden/shown state to the new cards too.
	UpdateHandSlide();
}

void ASeatedPawn::ClientRejectPlay_Implementation(int32 HandInstanceId)
{
	for (int32 Index = 0; Index < PendingPlayCards.Num(); ++Index)
	{
		ACardActor* Card = PendingPlayCards[Index];

		if (Card && Card->InstanceId == HandInstanceId)
		{
			PendingPlayCards.RemoveAt(Index);
			Card->SetActorHiddenInGame(false);
			HandCards.Add(Card);
			return;
		}
	}
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

	if (!HoveredCard || !PlayerController->WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		return;
	}

	// Mulligan: a click marks or unmarks the card for replacing instead of picking it up.
	if (IsMulliganPhase())
	{
		if (MulliganMarked.Contains(HoveredCard->InstanceId))
		{
			MulliganMarked.Remove(HoveredCard->InstanceId);
		}
		else
		{
			MulliganMarked.Add(HoveredCard->InstanceId);
		}

		return;
	}

	DraggedCard = HoveredCard;
	DragLocation = CursorLocal;
}

void ASeatedPawn::UpdateBoardPreview(const APlayerController& PlayerController, bool bOverBoard)
{
	const int32 PreviousIndex = BoardInsertIndex;
	BoardInsertIndex = INDEX_NONE;

	FVector RayOrigin;
	FVector RayDirection;

	// The Coin never goes on the table: anywhere above the hand plays it, without opening a gap.
	const bool bDraggingCoin = DraggedCard && DraggedCard->GetCardId() == UCardboardSettings::GetCoinCardId();

	if (bOverBoard && bDraggingCoin)
	{
		BoardInsertIndex = 0;
	}
	else if (bOverBoard && Table && PlayerController.DeprojectMousePositionToWorld(RayOrigin, RayDirection))
	{
		int32 InsertIndex;

		if (Table->GetInsertIndexAt(SeatIndex, RayOrigin, RayDirection, InsertIndex))
		{
			BoardInsertIndex = InsertIndex;
		}
	}

	if (Table && BoardInsertIndex != PreviousIndex)
	{
		Table->SetPlacementPreview(SeatIndex, bDraggingCoin ? INDEX_NONE : BoardInsertIndex);
	}
}

void ASeatedPawn::PlayDraggedCard()
{
	// Hide it right away so the play feels instant. The replicated hand removes it for good,
	// or ClientRejectPlay brings it back.
	HandCards.Remove(DraggedCard);
	DraggedCard->SetActorHiddenInGame(true);
	PendingPlayCards.Add(DraggedCard);

	ServerPlayCard(DraggedCard->InstanceId, BoardInsertIndex);
}

void ASeatedPawn::ServerPlayCard_Implementation(int32 HandInstanceId, int32 InsertIndex)
{
	const int32 HandIndex = Hand.IndexOfByPredicate([HandInstanceId](const FHandCard& HandCard)
	{
		return HandCard.InstanceId == HandInstanceId;
	});

	if (HandIndex == INDEX_NONE || !Table || !IsMyTurn())
	{
		ClientRejectPlay(HandInstanceId);
		return;
	}

	const FName CardId = Hand[HandIndex].CardId;
	const bool bIsCoin = CardId == UCardboardSettings::GetCoinCardId();
	const FCardDefinition* Definition = UCardboardSettings::FindCard(CardId);
	const int32 Cost = Definition ? Definition->Cost : 0;

	if (Mana < Cost || (!bIsCoin && Table->GetRowSize(SeatIndex) >= Table->MaxRowSize))
	{
		ClientRejectPlay(HandInstanceId);
		return;
	}

	Mana -= Cost;
	Hand.RemoveAt(HandIndex);

	// The Coin is spent for +1 mana this turn; every other card goes on the table.
	if (bIsCoin)
	{
		++Mana;
	}
	else
	{
		Table->PlaceCard(SeatIndex, InsertIndex, CardId);
	}

	SyncHandVisuals();
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
	FVector Location(
		-Index * ACardActor::CardThickness * 1.5f,
		SlotY,
		-FMath::Square(FromMiddle) * HandArcDrop
	);

	FRotator Rotation(0.0f, 0.0f, FromMiddle * HandFanAngle);

	// Marked in the mulligan: turned face-down and slightly lowered.
	if (MulliganMarked.Contains(Card->InstanceId))
	{
		Rotation.Yaw = 180.0f;
		Location.Z -= 2.0f;
	}

	return FTransform(Rotation, Location);
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

	// Only the owner may know their hand; the deck size is public.
	DOREPLIFETIME_CONDITION(ASeatedPawn, Hand, COND_OwnerOnly);
	DOREPLIFETIME(ASeatedPawn, DeckCount);
	DOREPLIFETIME(ASeatedPawn, Health);
	DOREPLIFETIME(ASeatedPawn, Mana);
	DOREPLIFETIME(ASeatedPawn, MaxMana);
	DOREPLIFETIME(ASeatedPawn, bMulliganConfirmed);
}
