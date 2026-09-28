#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MenuRootWidget.generated.h"

UCLASS()
class CARDBOARD_API UMenuRootWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowSessionMenu();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowLobby();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowSettings();

protected:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUserWidget> MainMenu;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUserWidget> SessionMenu;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUserWidget> Lobby;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUserWidget> Settings;

private:

	void SetOnlyVisible(UUserWidget* WidgetToShow);
};