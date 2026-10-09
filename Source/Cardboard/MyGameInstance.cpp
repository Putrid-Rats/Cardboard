// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameInstance.h"

#include "CardboardUserSettings.h"
#include "UObject/UObjectGlobals.h"

void UMyGameInstance::Init()
{
	Super::Init();

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UMyGameInstance::HandlePostLoadMap);

	UE_LOG(LogTemp, Log, TEXT("MyGameInstance initialized"));
}

void UMyGameInstance::Shutdown()
{
	UE_LOG(LogTemp, Log, TEXT("MyGameInstance shutting down"));

	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	Super::Shutdown();
}

void UMyGameInstance::OnStart()
{
	Super::OnStart();

	HandlePostLoadMap(GetWorld());
}

void UMyGameInstance::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (UCardboardUserSettings* Settings = UCardboardUserSettings::GetCardboardUserSettings())
	{
		Settings->ApplyAudioAndGamma(LoadedWorld);
	}
}
