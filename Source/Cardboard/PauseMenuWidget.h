#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

class ASeatedPawn;
class USettingsWidget;
class UWidget;

// Base class of WBP_PauseMenu: the in-match menu opened with Esc (or P).
// Every widget is optional - name it exactly as below and it gets wired up.
// The buttons can be plain Buttons or WBP_Button. The match keeps running behind it (it's multiplayer).
UCLASS(Abstract)
class CARDBOARD_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// Called by the pawn right after creating the widget.
	void Init(ASeatedPawn* InOwnerPawn);

	// Esc while the settings panel is open goes back to the menu instead of closing it.
	// Returns true if it did.
	bool CloseSettingsPanel();

protected:

	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ResumeButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SettingsButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> LeaveButton;

	// The box holding the buttons, hidden while the settings panel is shown.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> MenuPanel;

	// A WBP_Settings placed in this widget, hidden by default (set its Visibility to Collapsed).
	// Its Back button returns to the pause menu.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USettingsWidget> SettingsPanel;

private:

	TWeakObjectPtr<ASeatedPawn> OwnerPawn;

	UFUNCTION()
	void HandleResumeClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleLeaveClicked();

	UFUNCTION()
	void HandleSettingsClosed();
};
