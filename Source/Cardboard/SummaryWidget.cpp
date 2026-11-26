#include "SummaryWidget.h"

#include "CardGameState.h"
#include "CardboardUI.h"
#include "Components/TextBlock.h"
#include "MatchHistorySave.h"
#include "SeatedPawn.h"

namespace
{
	void SetSummaryText(UTextBlock* Text, const FString& Value)
	{
		if (Text)
		{
			Text->SetText(FText::FromString(Value));
		}
	}
}

void USummaryWidget::Init(ASeatedPawn* InOwnerPawn, int32 InWins, int32 InLosses)
{
	OwnerPawn = InOwnerPawn;
	Wins = InWins;
	Losses = InLosses;
	Refresh();
}

void USummaryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Plain Buttons or WBP_Button both work (see CardboardUI.h).
	if (UButton* Button = FindClickableButton(RematchButton))
	{
		Button->OnClicked.AddDynamic(this, &USummaryWidget::HandleRematchClicked);
	}

	if (UButton* Button = FindClickableButton(BackToLobbyButton))
	{
		Button->OnClicked.AddDynamic(this, &USummaryWidget::HandleBackToLobbyClicked);
	}
}

void USummaryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	Refresh();
}

void USummaryWidget::Refresh()
{
	ASeatedPawn* Me = OwnerPawn.Get();

	if (!Me)
	{
		return;
	}

	const ASeatedPawn* Opponent = Me->GetOpponentPawn();
	const ACardGameState* CardGameState = GetWorld() ? GetWorld()->GetGameState<ACardGameState>() : nullptr;

	if (CardGameState)
	{
		SetSummaryText(ResultText, CardGameState->GetWinningSeat() == Me->SeatIndex ? TEXT("VICTORY") : TEXT("DEFEAT"));

		const int32 Seconds = FMath::RoundToInt(CardGameState->GetMatchDuration());
		SetSummaryText(MatchInfoText, FString::Printf(TEXT("%d turns  ·  %d:%02d"), CardGameState->GetTurnNumber(), Seconds / 60, Seconds % 60));
	}

	SetSummaryText(YourNameText, Me->GetPlayerDisplayName());
	SetSummaryText(YourStatsText, FormatStats(Me->MatchStats, Me->GetHealth()));

	if (Opponent)
	{
		SetSummaryText(OpponentNameText, Opponent->GetPlayerDisplayName());
		SetSummaryText(OpponentStatsText, FormatStats(Opponent->MatchStats, Opponent->GetHealth()));
	}

	SetSummaryText(HistoryText, FString::Printf(TEXT("Your record: %d %s, %d %s"),
		Wins, Wins == 1 ? TEXT("win") : TEXT("wins"),
		Losses, Losses == 1 ? TEXT("loss") : TEXT("losses")));

	const bool bOpponentReady = Opponent && Opponent->IsRematchReady();

	if (Me->IsRematchReady())
	{
		SetSummaryText(RematchStatusText, TEXT("Waiting for the opponent..."));
	}
	else
	{
		SetSummaryText(RematchStatusText, bOpponentReady ? TEXT("Your opponent wants a rematch!") : TEXT(""));
	}

	if (RematchButton)
	{
		RematchButton->SetIsEnabled(!Me->IsRematchReady());
	}
}

FString USummaryWidget::FormatStats(const FPlayerMatchStats& Stats, int32 HealthLeft)
{
	return FString::Printf(
		TEXT("Health left: %d\nDamage to opponent: %d\nDamage to cards: %d\nEnemy cards destroyed: %d\nCards lost: %d\nCards played: %d\nMana spent: %d\nTurns taken: %d"),
		HealthLeft, Stats.DamageToOpponent, Stats.DamageToCards, Stats.CardsDestroyed,
		Stats.CardsLost, Stats.CardsPlayed, Stats.ManaSpent, Stats.TurnsTaken);
}

void USummaryWidget::HandleRematchClicked()
{
	if (ASeatedPawn* Me = OwnerPawn.Get())
	{
		Me->RequestRematch();
	}
}

void USummaryWidget::HandleBackToLobbyClicked()
{
	if (ASeatedPawn* Me = OwnerPawn.Get())
	{
		Me->RequestReturnToLobby();
	}
}
