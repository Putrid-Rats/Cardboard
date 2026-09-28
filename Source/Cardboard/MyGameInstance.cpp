// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameInstance.h"

void UMyGameInstance::Init()
{
	Super::Init();

	UE_LOG(LogTemp, Log, TEXT("MyGameInstance initialized"));
}

void UMyGameInstance::Shutdown()
{
	UE_LOG(LogTemp, Log, TEXT("MyGameInstance shutting down"));

	Super::Shutdown();
}