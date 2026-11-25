#include "MatchHistorySave.h"

#include "Kismet/GameplayStatics.h"

const FString UMatchHistorySave::SlotName = TEXT("MatchHistory");

UMatchHistorySave* UMatchHistorySave::LoadHistory()
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		if (UMatchHistorySave* Loaded = Cast<UMatchHistorySave>(UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
		{
			return Loaded;
		}
	}

	return Cast<UMatchHistorySave>(UGameplayStatics::CreateSaveGameObject(UMatchHistorySave::StaticClass()));
}

void UMatchHistorySave::AddMatch(const FMatchRecord& Record)
{
	if (UMatchHistorySave* History = LoadHistory())
	{
		History->Matches.Add(Record);
		UGameplayStatics::SaveGameToSlot(History, SlotName, 0);
	}
}

int32 UMatchHistorySave::GetWins() const
{
	int32 Wins = 0;

	for (const FMatchRecord& Match : Matches)
	{
		Wins += Match.bWon ? 1 : 0;
	}

	return Wins;
}
