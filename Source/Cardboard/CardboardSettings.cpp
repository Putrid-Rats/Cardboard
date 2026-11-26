#include "CardboardSettings.h"

#include "CardDefinition.h"
#include "Engine/DataTable.h"

UDataTable* UCardboardSettings::GetCardDataTable()
{
	return GetDefault<UCardboardSettings>()->CardDataTable.LoadSynchronous();
}

FName UCardboardSettings::GetCoinCardId()
{
	return GetDefault<UCardboardSettings>()->CoinCardId;
}

const FCardDefinition* UCardboardSettings::FindCard(FName CardId)
{
	const UDataTable* CardTable = GetCardDataTable();

	if (!CardTable || CardId.IsNone())
	{
		return nullptr;
	}

	return CardTable->FindRow<FCardDefinition>(CardId, TEXT("FindCard"), false);
}

UDataTable* UCardboardSettings::GetDeckDataTable()
{
	return GetDefault<UCardboardSettings>()->DeckDataTable.LoadSynchronous();
}

const FDeckDefinition* UCardboardSettings::FindDeck(FName DeckId)
{
	const UDataTable* DeckTable = GetDeckDataTable();

	if (!DeckTable || DeckId.IsNone())
	{
		return nullptr;
	}

	return DeckTable->FindRow<FDeckDefinition>(DeckId, TEXT("FindDeck"), false);
}

TArray<FName> UCardboardSettings::GetDeckIds()
{
	const UDataTable* DeckTable = GetDeckDataTable();
	return DeckTable ? DeckTable->GetRowNames() : TArray<FName>();
}

FName UCardboardSettings::GetDefaultDeckId()
{
	const TArray<FName> DeckIds = GetDeckIds();
	return DeckIds.Num() > 0 ? DeckIds[0] : NAME_None;
}
