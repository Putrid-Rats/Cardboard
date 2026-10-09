#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CardGameState.generated.h"

class ASeatedPawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchEnded, int32, WinningSeat);

UENUM(BlueprintType)
enum class EMatchPhase : uint8
{
	WaitingForPlayers,
	CoinFlip,
	Mulligan,
	Playing,
	Ended
};

// Match state shared by both players on L_Gameplay: phase, coin flip, whose turn it is, who won.
// The match flow runs on the server:
// both players seated -> coin flip -> after CoinFlipDuration: opening hands, mulligan
// -> both confirmed: the second player gets The Coin, the first turn starts
// -> players end turns in alternation -> a player's health reaches 0.
// Set as the GameState Class in GM_Gameplay_TCG.
UCLASS()
class CARDBOARD_API ACardGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

	// How long the coin is shown before the mulligan starts, in seconds.
	UPROPERTY(EditDefaultsOnly, Category = "Match")
	float CoinFlipDuration = 3.0f;

	// Cards each player starts with (the second player also gets The Coin).
	UPROPERTY(EditDefaultsOnly, Category = "Match", meta = (ClampMin = 0))
	int32 OpeningHandSize = 3;

	// Max mana grows by 1 per turn up to this.
	UPROPERTY(EditDefaultsOnly, Category = "Match", meta = (ClampMin = 1))
	int32 MaxManaCap = 10;

	// Fires on every machine when the match is decided.
	UPROPERTY(BlueprintAssignable, Category = "Match")
	FOnMatchEnded OnMatchEnded;

	UFUNCTION(BlueprintPure, Category = "Match")
	EMatchPhase GetMatchPhase() const { return Phase; }

	UFUNCTION(BlueprintPure, Category = "Match")
	bool IsMatchStarted() const { return FirstSeat != INDEX_NONE; }

	UFUNCTION(BlueprintPure, Category = "Match")
	bool IsMatchOver() const { return WinningSeat != INDEX_NONE; }

	// Seat that won the coin flip, or -1 before the flip.
	UFUNCTION(BlueprintPure, Category = "Match")
	int32 GetFirstSeat() const { return FirstSeat; }

	// Seat whose turn it is, or -1 before the first turn.
	UFUNCTION(BlueprintPure, Category = "Match")
	int32 GetCurrentTurnSeat() const { return CurrentTurnSeat; }

	// Counts both players' turns, starting at 1.
	UFUNCTION(BlueprintPure, Category = "Match")
	int32 GetTurnNumber() const { return TurnNumber; }

	// Seat of the winner, or -1 while the match is still going.
	UFUNCTION(BlueprintPure, Category = "Match")
	int32 GetWinningSeat() const { return WinningSeat; }

	// Server only. Starts the match once both seats have a player; does nothing otherwise.
	void TryStartMatch();

	// Server only, called by a pawn after its mulligan. Starts the first turn once both players are done.
	void NotifyMulliganConfirmed();

	// Server only. Ignored unless it's that seat's turn.
	void EndTurn(int32 Seat);

	// Server only. Ignored once a winner is set.
	void EndMatch(int32 InWinningSeat);

protected:

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;

private:

	UPROPERTY(ReplicatedUsing = OnRep_Phase)
	EMatchPhase Phase = EMatchPhase::WaitingForPlayers;

	UPROPERTY(ReplicatedUsing = OnRep_FirstSeat)
	int32 FirstSeat = INDEX_NONE;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentTurnSeat)
	int32 CurrentTurnSeat = INDEX_NONE;

	UPROPERTY(Replicated)
	int32 TurnNumber = 0;

	UPROPERTY(ReplicatedUsing = OnRep_WinningSeat)
	int32 WinningSeat = INDEX_NONE;

	FTimerHandle MulliganTimer;

	UFUNCTION()
	void OnRep_Phase();

	UFUNCTION()
	void OnRep_FirstSeat();

	UFUNCTION()
	void OnRep_CurrentTurnSeat();

	UFUNCTION()
	void OnRep_WinningSeat();

	void SetPhase(EMatchPhase NewPhase);
	void StartMulligan();
	void StartTurn(int32 Seat);
	ASeatedPawn* FindSeatedPawn(int32 Seat) const;
};
