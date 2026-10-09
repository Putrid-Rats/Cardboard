#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "SettingsWidget.generated.h"

class UButton;
class UCheckBox;
class UComboBoxString;
class USlider;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSettingsClosed);

// Base class of WBP_Settings: all the logic, the Blueprint only lays out the widgets.
// Every widget is optional - name it exactly as below and it gets wired up, leave it out and it's skipped.
// Volume, gamma, field of view and sensitivity preview straight away; the other video options take effect on Apply.
// Apply saves everything. Back without Apply throws the changes away. Reset restores the defaults (unsaved).
UCLASS(Abstract)
class CARDBOARD_API USettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// Back was clicked. The menu holding this widget decides what to show next.
	UPROPERTY(BlueprintAssignable, Category = "Settings")
	FOnSettingsClosed OnClosed;

	// Loads the saved values into the widgets. Called on construct; call it again when re-showing the panel.
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void RefreshFromSettings();

protected:

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	// Video
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> ResolutionBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> WindowModeBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> QualityBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> FrameRateBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> VSyncCheck;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> ResolutionScaleSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> GammaSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> FieldOfViewSlider;

	// Audio
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> MasterVolumeSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> MusicVolumeSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> EffectsVolumeSlider;

	// Controls
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> SensitivitySlider;

	// Value readouts next to the sliders (e.g. "80%", "2.2", "90°").
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ResolutionScaleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GammaText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FieldOfViewText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MasterVolumeText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MusicVolumeText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EffectsVolumeText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SensitivityText;

	// Buttons
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ApplyButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ResetButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackButton;

private:

	// Fullscreen resolutions in the same order as ResolutionBox's options.
	TArray<FIntPoint> Resolutions;

	UFUNCTION()
	void HandleResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleWindowModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleFrameRateChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleVSyncChanged(bool bIsChecked);

	UFUNCTION()
	void HandleResolutionScaleChanged(float Value);

	UFUNCTION()
	void HandleGammaChanged(float Value);

	UFUNCTION()
	void HandleFieldOfViewChanged(float Value);

	UFUNCTION()
	void HandleMasterVolumeChanged(float Value);

	UFUNCTION()
	void HandleMusicVolumeChanged(float Value);

	UFUNCTION()
	void HandleEffectsVolumeChanged(float Value);

	UFUNCTION()
	void HandleSensitivityChanged(float Value);

	UFUNCTION()
	void HandleApplyClicked();

	UFUNCTION()
	void HandleResetClicked();

	UFUNCTION()
	void HandleBackClicked();

	void UpdateValueTexts();
};
