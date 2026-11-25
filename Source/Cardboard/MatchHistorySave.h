#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MatchHistorySave.generated.h"

// What one player did during a match. Counted on the server, replicated to both players.
USTRUCT(BlueprintType)
struct FPlayerMatchStats
{
	GENERATED_BODY()

	// Cards put on the table (The Coin doesn't count).
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Stats")
	int32 CardsPlayed = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Stats")
	int32 ManaSpent = 0;

	// Damage this player's cards dealt to the opponent's health.
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Stats")
	int32 DamageToOpponent = 0;

	// Damage this player's cards dealt to enemy cards.
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Stats")
	int32 DamageToCards = 0;

	// Enemy cards destroyed in this player's attacks.
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Stats")
	int32 CardsDestroyed = 0;

	// This player's own cards that were destroyed.
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Stats")
	int32 CardsLost = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Stats")
	int32 TurnsTaken = 0;
};

// One finished match, as remembered by this computer.
USTRUCT(BlueprintType)
struct FMatchRecord
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "History")
	FDateTime PlayedAt;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "History")
	bool bWon = false;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "History")
	FString OpponentName;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "History")
	int32 Turns = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "History")
	float DurationSeconds = 0.0f;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "History")
	FPlayerMatchStats MyStats;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "History")
	FPlayerMatchStats OpponentStats;
};

// Local match history, saved in Saved/SaveGames/MatchHistory.sav on each player's own computer.
UCLASS()
class CARDBOARD_API UMatchHistorySave : public USaveGame
{
	GENERATED_BODY()

public:

	static const FString SlotName;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "History")
	TArray<FMatchRecord> Matches;

	// The saved history, or an empty new one if nothing was saved yet.
	UFUNCTION(BlueprintCallable, Category = "History")
	static UMatchHistorySave* LoadHistory();

	// Appends a match and writes the file.
	UFUNCTION(BlueprintCallable, Category = "History")
	static void AddMatch(const FMatchRecord& Record);

	UFUNCTION(BlueprintPure, Category = "History")
	int32 GetWins() const;

	UFUNCTION(BlueprintPure, Category = "History")
	int32 GetLosses() const { return Matches.Num() - GetWins(); }
};
