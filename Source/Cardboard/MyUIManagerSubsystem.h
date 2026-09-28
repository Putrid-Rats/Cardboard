#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "MyUIManagerSubsystem.generated.h"

UCLASS()
class CARDBOARD_API UMyUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void InitializeUI(
		APlayerController* PlayerController,
		TSubclassOf<UUserWidget> InMenuRootClass
	);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowSessionMenu();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowLobby();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowSettings();

private:

	UPROPERTY()
	TObjectPtr<UUserWidget> MenuRoot;

	UPROPERTY()
	TObjectPtr<APlayerController> CachedPlayerController;
};