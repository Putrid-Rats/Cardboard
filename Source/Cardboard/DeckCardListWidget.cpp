#include "DeckCardListWidget.h"

#include "CardboardSettings.h"
#include "CardDefinition.h"
#include "CardboardUI.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"

void UDeckCardRowWidget::SetCard(FName CardId, int32 Copies)
{
	const FCardDefinition* Card = UCardboardSettings::FindCard(CardId);

	auto SetText = [](UTextBlock* Text, const FText& Value)
	{
		if (Text)
		{
			Text->SetText(Value);
		}
	};

	const FText Unknown = INVTEXT("?");

	SetText(CopiesText, FText::Format(INVTEXT("{0}x"), Copies));
	SetText(NameText, Card ? Card->CardName : FText::FromName(CardId));
	SetText(CostText, Card ? FText::AsNumber(Card->Cost) : Unknown);
	SetText(AttackText, Card ? FText::AsNumber(Card->Attack) : Unknown);
	SetText(HealthText, Card ? FText::AsNumber(Card->Health) : Unknown);
	SetText(RulesText, Card ? Card->GetRulesText() : FText::GetEmpty());
}

void UDeckCardListWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (UButton* Button = FindClickableButton(CloseButton))
	{
		Button->OnClicked.AddDynamic(this, &UDeckCardListWidget::HandleCloseClicked);
	}
}

void UDeckCardListWidget::ShowDeck(FName DeckId)
{
	const FDeckDefinition* Deck = UCardboardSettings::FindDeck(DeckId);

	if (DeckNameText)
	{
		DeckNameText->SetText(Deck
			? FText::Format(INVTEXT("{0}  ({1} cards)"), Deck->DeckName, Deck->GetCardCount())
			: FText::FromName(DeckId));
	}

	if (!RowsBox)
	{
		return;
	}

	RowsBox->ClearChildren();

	if (!Deck || !RowClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("DeckCardListWidget: %s"), RowClass ? TEXT("unknown deck") : TEXT("set Row Class (WBP_DeckCardRow) in Class Defaults"));
		return;
	}

	// Cheapest first, like a mana curve.
	TArray<FDeckCard> Entries = Deck->Cards;
	Entries.StableSort([](const FDeckCard& A, const FDeckCard& B)
	{
		const FCardDefinition* CardA = UCardboardSettings::FindCard(A.CardId);
		const FCardDefinition* CardB = UCardboardSettings::FindCard(B.CardId);
		return (CardA ? CardA->Cost : 0) < (CardB ? CardB->Cost : 0);
	});

	for (const FDeckCard& Entry : Entries)
	{
		if (UDeckCardRowWidget* Row = CreateWidget<UDeckCardRowWidget>(this, RowClass))
		{
			Row->SetCard(Entry.CardId, Entry.Copies);
			RowsBox->AddChild(Row);
		}
	}
}

void UDeckCardListWidget::HandleCloseClicked()
{
	RemoveFromParent();
}
