#include "CardGameState.h"

#include "CardboardSettings.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "SeatedPawn.h"
#include "TimerManager.h"

void ACardGameState::TryStartMatch()
{
	if (!HasAuthority() || IsMatchStarted())
	{
		return;
	}

	ASeatedPawn* Seat0 = FindSeatedPawn(0);
	ASeatedPawn* Seat1 = FindSeatedPawn(1);

	if (!Seat0 || !Seat1)
	{
		return;
	}

	// Coin flip.
	FirstSeat = FMath::RandRange(0, 1);

	ASeatedPawn* FirstPlayer = FirstSeat == 0 ? Seat0 : Seat1;
	ASeatedPawn* SecondPlayer = FirstSeat == 0 ? Seat1 : Seat0;

	FirstPlayer->DrawCards(OpeningHandSize);
	SecondPlayer->DrawCards(OpeningHandSize);
	SecondPlayer->AddCardToHand(UCardboardSettings::GetCoinCardId());

	OnRep_FirstSeat();

	GetWorldTimerManager().SetTimer(FirstTurnTimer, this, &ACardGameState::StartFirstTurn, CoinFlipDuration, false);
}

void ACardGameState::EndTurn(int32 Seat)
{
	if (!HasAuthority() || IsMatchOver() || Seat != CurrentTurnSeat)
	{
		return;
	}

	StartTurn(1 - Seat);
}

void ACardGameState::EndMatch(int32 InWinningSeat)
{
	if (!HasAuthority() || IsMatchOver())
	{
		return;
	}

	WinningSeat = InWinningSeat;
	GetWorldTimerManager().ClearTimer(FirstTurnTimer);

	// OnRep doesn't run on the server, and the listen server host needs the events too.
	OnRep_WinningSeat();
}

void ACardGameState::StartFirstTurn()
{
	StartTurn(FirstSeat);
}

void ACardGameState::StartTurn(int32 Seat)
{
	if (IsMatchOver())
	{
		return;
	}

	CurrentTurnSeat = Seat;
	++TurnNumber;

	if (ASeatedPawn* Pawn = FindSeatedPawn(Seat))
	{
		Pawn->BeginTurn(MaxManaCap);
	}

	OnRep_CurrentTurnSeat();
}

ASeatedPawn* ACardGameState::FindSeatedPawn(int32 Seat) const
{
	for (TActorIterator<ASeatedPawn> It(GetWorld()); It; ++It)
	{
		if (It->SeatIndex == Seat && It->GetController())
		{
			return *It;
		}
	}

	return nullptr;
}

void ACardGameState::OnRep_FirstSeat()
{
	for (TActorIterator<ASeatedPawn> It(GetWorld()); It; ++It)
	{
		It->NotifyCoinFlipped(FirstSeat);
	}
}

void ACardGameState::OnRep_CurrentTurnSeat()
{
	for (TActorIterator<ASeatedPawn> It(GetWorld()); It; ++It)
	{
		It->NotifyTurnStarted(CurrentTurnSeat);
	}
}

void ACardGameState::OnRep_WinningSeat()
{
	for (TActorIterator<ASeatedPawn> It(GetWorld()); It; ++It)
	{
		It->NotifyMatchEnded(WinningSeat);
	}

	OnMatchEnded.Broadcast(WinningSeat);
}

void ACardGameState::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACardGameState, FirstSeat);
	DOREPLIFETIME(ACardGameState, CurrentTurnSeat);
	DOREPLIFETIME(ACardGameState, TurnNumber);
	DOREPLIFETIME(ACardGameState, WinningSeat);
}
