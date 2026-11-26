#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DeckPickerWidget.generated.h"

class ACardPlayerState;
class UDeckCardListWidget;
class UWidget;
class UTextBlock;

// Base class of WBP_DeckPicker: cycles through the decks in DT_Decks for the local player.
// Put it in WBP_Lobby. Every widget is optional - name it exactly as below and it gets filled in / wired up.
// The buttons can be plain Buttons or WBP_Button (its own click sound keeps working).
UCLASS(Abstract)
class CARDBOARD_API UDeckPickerWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// Window opened by CardListButton (WBP_DeckCardList).
	UPROPERTY(EditAnywhere, Category = "Deck")
	TSubclassOf<UDeckCardListWidget> CardListClass;

protected:

	virtual void NativeDestruct() override;

	// Opens the card list window for the shown deck.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CardListButton;

	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> PrevDeckButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> NextDeckButton;

	// "Swarm"
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DeckNameText;

	// The deck's description from DT_Decks.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DeckDescriptionText;

	// "2x Ant (1), 2x Rat (1), ..." as one paragraph: turn on Auto Wrap Text and let its slot Fill.
	// Optional now that the Card List window exists.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DeckCardsText;

private:

	// Shown right after a click, before the server's answer replicates back.
	FName PendingDeck;

	// Deck shown last frame, so the texts only rebuild when it changes.
	FName ShownDeck;

	UFUNCTION()
	void HandlePrevClicked();

	UFUNCTION()
	void HandleNextClicked();

	UFUNCTION()
	void HandleCardListClicked();

	// The open card list window, if any.
	UPROPERTY()
	TObjectPtr<UDeckCardListWidget> CardList;

	ACardPlayerState* GetCardPlayerState() const;
	FName GetDisplayedDeck() const;
	void StepDeck(int32 Direction);
	void Refresh();
};
