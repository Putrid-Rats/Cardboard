#include "DeckPickerWidget.h"

#include "CardboardSettings.h"
#include "CardDefinition.h"
#include "CardPlayerState.h"
#include "DeckCardListWidget.h"
#include "CardboardUI.h"
#include "Components/TextBlock.h"

void UDeckPickerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (UButton* Button = FindClickableButton(PrevDeckButton))
	{
		Button->OnClicked.AddDynamic(this, &UDeckPickerWidget::HandlePrevClicked);
	}

	if (UButton* Button = FindClickableButton(NextDeckButton))
	{
		Button->OnClicked.AddDynamic(this, &UDeckPickerWidget::HandleNextClicked);
	}

	if (UButton* Button = FindClickableButton(CardListButton))
	{
		Button->OnClicked.AddDynamic(this, &UDeckPickerWidget::HandleCardListClicked);
	}
}

void UDeckPickerWidget::NativeDestruct()
{
	// The window is in the viewport, not inside the picker: close it when the lobby goes away.
	if (CardList)
	{
		CardList->RemoveFromParent();
		CardList = nullptr;
	}

	Super::NativeDestruct();
}

void UDeckPickerWidget::HandleCardListClicked()
{
	if (!CardListClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("DeckPickerWidget: set Card List Class (WBP_DeckCardList) in Class Defaults"));
		return;
	}

	if (!CardList)
	{
		CardList = CreateWidget<UDeckCardListWidget>(GetOwningPlayer(), CardListClass);
	}

	if (CardList && !CardList->IsInViewport())
	{
		CardList->AddToViewport(50);
	}

	if (CardList)
	{
		CardList->ShowDeck(GetDisplayedDeck());
	}
}

void UDeckPickerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// The player state may arrive a moment after the widget, and the pick replicates back after a click.
	const ACardPlayerState* CardPlayerState = GetCardPlayerState();

	if (CardPlayerState && PendingDeck == CardPlayerState->GetSelectedDeck())
	{
		PendingDeck = NAME_None;
	}

	if (GetDisplayedDeck() != ShownDeck)
	{
		Refresh();
	}
}

ACardPlayerState* UDeckPickerWidget::GetCardPlayerState() const
{
	return GetOwningPlayerState<ACardPlayerState>();
}

FName UDeckPickerWidget::GetDisplayedDeck() const
{
	if (!PendingDeck.IsNone())
	{
		return PendingDeck;
	}

	const ACardPlayerState* CardPlayerState = GetCardPlayerState();
	return CardPlayerState ? CardPlayerState->GetSelectedDeck() : UCardboardSettings::GetDefaultDeckId();
}

void UDeckPickerWidget::HandlePrevClicked()
{
	StepDeck(-1);
}

void UDeckPickerWidget::HandleNextClicked()
{
	StepDeck(1);
}

void UDeckPickerWidget::StepDeck(int32 Direction)
{
	ACardPlayerState* CardPlayerState = GetCardPlayerState();
	const TArray<FName> DeckIds = UCardboardSettings::GetDeckIds();

	if (!CardPlayerState || DeckIds.Num() == 0)
	{
		return;
	}

	const int32 Current = FMath::Max(DeckIds.IndexOfByKey(GetDisplayedDeck()), 0);
	const int32 Next = (Current + Direction + DeckIds.Num()) % DeckIds.Num();

	PendingDeck = DeckIds[Next];
	CardPlayerState->ServerSelectDeck(PendingDeck);
	Refresh();
}

void UDeckPickerWidget::Refresh()
{
	ShownDeck = GetDisplayedDeck();
	const FDeckDefinition* Deck = UCardboardSettings::FindDeck(ShownDeck);

	// Keep an open card list window in step with the picker.
	if (CardList && CardList->IsInViewport())
	{
		CardList->ShowDeck(ShownDeck);
	}

	if (DeckNameText)
	{
		DeckNameText->SetText(Deck
			? FText::Format(INVTEXT("{0}  ({1} cards)"), Deck->DeckName, Deck->GetCardCount())
			: INVTEXT("No decks (set DT_Decks in Project Settings > Cardboard)"));
	}

	if (DeckDescriptionText)
	{
		DeckDescriptionText->SetText(Deck ? Deck->Description : FText::GetEmpty());
	}

	if (DeckCardsText)
	{
		TArray<FString> Lines;

		if (Deck)
		{
			for (const FDeckCard& Entry : Deck->Cards)
			{
				const FCardDefinition* Card = UCardboardSettings::FindCard(Entry.CardId);
				const FString Name = Card ? Card->CardName.ToString() : Entry.CardId.ToString();
				const int32 Cost = Card ? Card->Cost : 0;

				Lines.Add(FString::Printf(TEXT("%dx %s (%d)"), Entry.Copies, *Name, Cost));
			}
		}

		// One flowing paragraph (with Auto Wrap Text on) keeps the picker short.
		DeckCardsText->SetText(FText::FromString(FString::Join(Lines, TEXT(", "))));
	}
}
