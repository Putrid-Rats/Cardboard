// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MyGameInstance.generated.h"

/**
 *
 */
UCLASS()
class CARDBOARD_API UMyGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

virtual void Init() override;
virtual void Shutdown() override;

protected:

virtual void OnStart() override;

private:

// Re-applies the saved volumes and gamma, so they hold after every map change.
void HandlePostLoadMap(UWorld* LoadedWorld);
};
