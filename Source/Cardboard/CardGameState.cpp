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
	OnRep_FirstSeat();
	SetPhase(EMatchPhase::CoinFlip);

	GetWorldTimerManager().SetTimer(MulliganTimer, this, &ACardGameState::StartMulligan, CoinFlipDuration, false);
}

void ACardGameState::StartMulligan()
{
	for (int32 Seat = 0; Seat < 2; ++Seat)
	{
		if (ASeatedPawn* Pawn = FindSeatedPawn(Seat))
		{
			Pawn->DrawCards(OpeningHandSize);
		}
	}

	SetPhase(EMatchPhase::Mulligan);
}

void ACardGameState::NotifyMulliganConfirmed()
{
	ASeatedPawn* FirstPlayer = FindSeatedPawn(FirstSeat);
	ASeatedPawn* SecondPlayer = FindSeatedPawn(1 - FirstSeat);

	if (Phase != EMatchPhase::Mulligan || !FirstPlayer || !SecondPlayer
		|| !FirstPlayer->IsMulliganConfirmed() || !SecondPlayer->IsMulliganConfirmed())
	{
		return;
	}

	// Handed out after the mulligan, so it can't be thrown back.
	SecondPlayer->AddCardToHand(UCardboardSettings::GetCoinCardId());

	SetPhase(EMatchPhase::Playing);
	StartTurn(FirstSeat);
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
	GetWorldTimerManager().ClearTimer(MulliganTimer);

	// OnRep doesn't run on the server, and the listen server host needs the events too.
	OnRep_WinningSeat();
	SetPhase(EMatchPhase::Ended);
}

void ACardGameState::SetPhase(EMatchPhase NewPhase)
{
	Phase = NewPhase;

	// OnRep doesn't run on the server.
	OnRep_Phase();
}

void ACardGameState::OnRep_Phase()
{
	for (TActorIterator<ASeatedPawn> It(GetWorld()); It; ++It)
	{
		It->NotifyPhaseChanged(Phase);
	}
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

	DOREPLIFETIME(ACardGameState, Phase);
	DOREPLIFETIME(ACardGameState, FirstSeat);
	DOREPLIFETIME(ACardGameState, CurrentTurnSeat);
	DOREPLIFETIME(ACardGameState, TurnNumber);
	DOREPLIFETIME(ACardGameState, WinningSeat);
}
