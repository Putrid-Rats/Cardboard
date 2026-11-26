#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SummaryWidget.generated.h"

class ASeatedPawn;
class UWidget;
class UTextBlock;
struct FPlayerMatchStats;

// Base class of WBP_MatchSummary: the end-of-match screen with results and statistics.
// The pawn creates it when the match ends. Every widget is optional - name it exactly as below
// and it gets filled in / wired up. The texts refresh every frame, so late-arriving stats still show.
UCLASS(Abstract)
class CARDBOARD_API USummaryWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// Called by the pawn right after creating the widget.
	void Init(ASeatedPawn* InOwnerPawn, int32 InWins, int32 InLosses);

protected:

	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// "VICTORY" / "DEFEAT"
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ResultText;

	// "12 turns · 5:32"
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MatchInfoText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> YourNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OpponentNameText;

	// Multi-line stat lists, one per player.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> YourStatsText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OpponentStatsText;

	// "Your record: 3 wins, 2 losses" (from the local match history).
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HistoryText;

	// "Waiting for the opponent..." / "Opponent wants a rematch!"
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RematchStatusText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> RematchButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> BackToLobbyButton;

private:

	TWeakObjectPtr<ASeatedPawn> OwnerPawn;
	int32 Wins = 0;
	int32 Losses = 0;

	UFUNCTION()
	void HandleRematchClicked();

	UFUNCTION()
	void HandleBackToLobbyClicked();

	void Refresh();

	static FString FormatStats(const FPlayerMatchStats& Stats, int32 HealthLeft);
};
