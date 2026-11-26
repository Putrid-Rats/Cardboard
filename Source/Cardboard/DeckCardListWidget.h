#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DeckCardListWidget.generated.h"

class UWidget;
class UPanelWidget;
class UTextBlock;

// Base class of WBP_DeckCardRow: one card in the deck list window.
// Every widget is optional - name it exactly as below and it gets filled in.
UCLASS(Abstract)
class CARDBOARD_API UDeckCardRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	void SetCard(FName CardId, int32 Copies);

protected:

	// "3x"
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CopiesText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CostText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AttackText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthText;

	// Trait and ability, e.g. "Taunt, Foe draws 2". Empty for plain cards.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RulesText;
};

// Base class of WBP_DeckCardList: a window listing every card in a deck, cheapest first.
// Opened by the Card List button of WBP_DeckPicker. Every widget is optional except RowsBox.
// CloseButton can be a plain Button or WBP_Button.
UCLASS(Abstract)
class CARDBOARD_API UDeckCardListWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// Row Blueprint to create for each card (WBP_DeckCardRow).
	UPROPERTY(EditAnywhere, Category = "Deck")
	TSubclassOf<UDeckCardRowWidget> RowClass;

	// Fills the window with this deck's cards.
	void ShowDeck(FName DeckId);

protected:

	virtual void NativeOnInitialized() override;

	// "Bury Them (22 cards)"
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DeckNameText;

	// The rows go in here. A Vertical Box inside a Scroll Box works well.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> RowsBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CloseButton;

private:

	UFUNCTION()
	void HandleCloseClicked();
};
