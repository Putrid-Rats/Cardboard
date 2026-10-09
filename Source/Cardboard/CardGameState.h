#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CardGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchEnded, int32, WinningSeat);

// Match state shared by both players on L_Gameplay: who won (later also whose turn it is).
// Set as the GameState Class in GM_Gameplay_TCG.
UCLASS()
class CARDBOARD_API ACardGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

	// Fires on every machine when the match is decided.
	UPROPERTY(BlueprintAssignable, Category = "Match")
	FOnMatchEnded OnMatchEnded;

	UFUNCTION(BlueprintPure, Category = "Match")
	bool IsMatchOver() const { return WinningSeat != INDEX_NONE; }

	// Seat of the winner, or -1 while the match is still going.
	UFUNCTION(BlueprintPure, Category = "Match")
	int32 GetWinningSeat() const { return WinningSeat; }

	// Server only. Ignored once a winner is set.
	void EndMatch(int32 InWinningSeat);

protected:

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;

private:

	UPROPERTY(ReplicatedUsing = OnRep_WinningSeat)
	int32 WinningSeat = INDEX_NONE;

	UFUNCTION()
	void OnRep_WinningSeat();
};
