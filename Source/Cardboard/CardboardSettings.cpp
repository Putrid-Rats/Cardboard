#include "CardboardSettings.h"

#include "CardDefinition.h"
#include "Engine/DataTable.h"

UDataTable* UCardboardSettings::GetCardDataTable()
{
	return GetDefault<UCardboardSettings>()->CardDataTable.LoadSynchronous();
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
