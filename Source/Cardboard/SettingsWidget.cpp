#include "SettingsWidget.h"

#include "CardboardUserSettings.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
	// Options shown in the boxes, in order. Index = engine value where it applies.
	const TArray<FString> WindowModeNames = { TEXT("Fullscreen"), TEXT("Borderless Window"), TEXT("Windowed") };
	const TArray<FString> QualityNames = { TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic"), TEXT("Cinematic") };
	const TArray<int32> FrameRates = { 30, 60, 120, 144, 165, 240, 0 };

	FString FrameRateName(int32 FrameRate)
	{
		return FrameRate == 0 ? TEXT("Unlimited") : FString::FromInt(FrameRate);
	}

	FString ResolutionName(const FIntPoint& Resolution)
	{
		return FString::Printf(TEXT("%d x %d"), Resolution.X, Resolution.Y);
	}

	void SetSlider(USlider* Slider, float Min, float Max, float Step)
	{
		if (Slider)
		{
			Slider->SetMinValue(Min);
			Slider->SetMaxValue(Max);
			Slider->SetStepSize(Step);
		}
	}

	void SetText(UTextBlock* Text, const FString& Value)
	{
		if (Text)
		{
			Text->SetText(FText::FromString(Value));
		}
	}
}

void USettingsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Fixed options and slider ranges, then the event bindings (once per widget).
	if (WindowModeBox)
	{
		for (const FString& Name : WindowModeNames)
		{
			WindowModeBox->AddOption(Name);
		}

		WindowModeBox->OnSelectionChanged.AddDynamic(this, &USettingsWidget::HandleWindowModeChanged);
	}

	if (QualityBox)
	{
		for (const FString& Name : QualityNames)
		{
			QualityBox->AddOption(Name);
		}

		QualityBox->OnSelectionChanged.AddDynamic(this, &USettingsWidget::HandleQualityChanged);
	}

	if (FrameRateBox)
	{
		for (const int32 FrameRate : FrameRates)
		{
			FrameRateBox->AddOption(FrameRateName(FrameRate));
		}

		FrameRateBox->OnSelectionChanged.AddDynamic(this, &USettingsWidget::HandleFrameRateChanged);
	}

	if (ResolutionBox)
	{
		ResolutionBox->OnSelectionChanged.AddDynamic(this, &USettingsWidget::HandleResolutionChanged);
	}

	SetSlider(ResolutionScaleSlider, 25.0f, 100.0f, 5.0f);
	SetSlider(GammaSlider, UCardboardUserSettings::MinGamma, UCardboardUserSettings::MaxGamma, 0.05f);
	SetSlider(FieldOfViewSlider, 70.0f, 110.0f, 1.0f);
	SetSlider(MasterVolumeSlider, 0.0f, 1.0f, 0.01f);
	SetSlider(MusicVolumeSlider, 0.0f, 1.0f, 0.01f);
	SetSlider(EffectsVolumeSlider, 0.0f, 1.0f, 0.01f);
	SetSlider(SensitivitySlider, 0.1f, 3.0f, 0.05f);

	if (VSyncCheck) { VSyncCheck->OnCheckStateChanged.AddDynamic(this, &USettingsWidget::HandleVSyncChanged); }
	if (ResolutionScaleSlider) { ResolutionScaleSlider->OnValueChanged.AddDynamic(this, &USettingsWidget::HandleResolutionScaleChanged); }
	if (GammaSlider) { GammaSlider->OnValueChanged.AddDynamic(this, &USettingsWidget::HandleGammaChanged); }
	if (FieldOfViewSlider) { FieldOfViewSlider->OnValueChanged.AddDynamic(this, &USettingsWidget::HandleFieldOfViewChanged); }
	if (MasterVolumeSlider) { MasterVolumeSlider->OnValueChanged.AddDynamic(this, &USettingsWidget::HandleMasterVolumeChanged); }
	if (MusicVolumeSlider) { MusicVolumeSlider->OnValueChanged.AddDynamic(this, &USettingsWidget::HandleMusicVolumeChanged); }
	if (EffectsVolumeSlider) { EffectsVolumeSlider->OnValueChanged.AddDynamic(this, &USettingsWidget::HandleEffectsVolumeChanged); }
	if (SensitivitySlider) { SensitivitySlider->OnValueChanged.AddDynamic(this, &USettingsWidget::HandleSensitivityChanged); }
	if (ApplyButton) { ApplyButton->OnClicked.AddDynamic(this, &USettingsWidget::HandleApplyClicked); }
	if (ResetButton) { ResetButton->OnClicked.AddDynamic(this, &USettingsWidget::HandleResetClicked); }
	if (BackButton) { BackButton->OnClicked.AddDynamic(this, &USettingsWidget::HandleBackClicked); }
}

void USettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshFromSettings();
}

void USettingsWidget::RefreshFromSettings()
{
	const UCardboardUserSettings* Settings = UCardboardUserSettings::GetCardboardUserSettings();

	if (!Settings)
	{
		return;
	}

	// Setting values from code fires the change events with ESelectInfo::Direct / the same values,
	// which the handlers either ignore or simply store back.
	if (ResolutionBox)
	{
		Resolutions.Reset();
		UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions);

		const FIntPoint Current = Settings->GetScreenResolution();
		Resolutions.AddUnique(Current);
		Resolutions.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.X != B.X ? A.X > B.X : A.Y > B.Y; });

		ResolutionBox->ClearOptions();

		for (const FIntPoint& Resolution : Resolutions)
		{
			ResolutionBox->AddOption(ResolutionName(Resolution));
		}

		ResolutionBox->SetSelectedOption(ResolutionName(Current));
	}

	if (WindowModeBox)
	{
		WindowModeBox->SetSelectedIndex(static_cast<int32>(Settings->GetFullscreenMode()));
	}

	if (QualityBox)
	{
		// -1 = custom mix of levels: leave the box empty.
		QualityBox->SetSelectedIndex(Settings->GetOverallScalabilityLevel());
	}

	if (FrameRateBox)
	{
		const int32 Limit = FMath::RoundToInt(Settings->GetFrameRateLimit());
		FrameRateBox->SetSelectedOption(FrameRateName(FrameRates.Contains(Limit) ? Limit : 0));
	}

	if (VSyncCheck)
	{
		VSyncCheck->SetIsChecked(Settings->IsVSyncEnabled());
	}

	if (ResolutionScaleSlider)
	{
		float ScaleNormalized, ScaleValue, MinScale, MaxScale;
		Settings->GetResolutionScaleInformationEx(ScaleNormalized, ScaleValue, MinScale, MaxScale);
		ResolutionScaleSlider->SetValue(ScaleValue);
	}

	if (GammaSlider) { GammaSlider->SetValue(Settings->GetGamma()); }
	if (FieldOfViewSlider) { FieldOfViewSlider->SetValue(Settings->GetFieldOfView()); }
	if (MasterVolumeSlider) { MasterVolumeSlider->SetValue(Settings->GetMasterVolume()); }
	if (MusicVolumeSlider) { MusicVolumeSlider->SetValue(Settings->GetMusicVolume()); }
	if (EffectsVolumeSlider) { EffectsVolumeSlider->SetValue(Settings->GetEffectsVolume()); }
	if (SensitivitySlider) { SensitivitySlider->SetValue(Settings->GetMouseSensitivity()); }

	UpdateValueTexts();
}

void USettingsWidget::UpdateValueTexts()
{
	const UCardboardUserSettings* Settings = UCardboardUserSettings::GetCardboardUserSettings();

	if (!Settings)
	{
		return;
	}

	float ScaleNormalized, ScaleValue, MinScale, MaxScale;
	Settings->GetResolutionScaleInformationEx(ScaleNormalized, ScaleValue, MinScale, MaxScale);

	SetText(ResolutionScaleText, FString::Printf(TEXT("%d%%"), FMath::RoundToInt(ScaleValue)));
	SetText(GammaText, FString::Printf(TEXT("%.2f"), Settings->GetGamma()));
	SetText(FieldOfViewText, FString::Printf(TEXT("%d°"), FMath::RoundToInt(Settings->GetFieldOfView())));
	SetText(MasterVolumeText, FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Settings->GetMasterVolume() * 100.0f)));
	SetText(MusicVolumeText, FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Settings->GetMusicVolume() * 100.0f)));
	SetText(EffectsVolumeText, FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Settings->GetEffectsVolume() * 100.0f)));
	SetText(SensitivityText, FString::Printf(TEXT("%.2fx"), Settings->GetMouseSensitivity()));
}

void USettingsWidget::HandleResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	UCardboardUserSettings* Settings = UCardboardUserSettings::GetCardboardUserSettings();
	const int32 Index = ResolutionBox->GetSelectedIndex();

	if (Settings && SelectionType != ESelectInfo::Direct && Resolutions.IsValidIndex(Index))
	{
		Settings->SetScreenResolution(Resolutions[Index]);
	}
}

void USettingsWidget::HandleWindowModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	UCardboardUserSettings* Settings = UCardboardUserSettings::GetCardboardUserSettings();
	const int32 Index = WindowModeBox->GetSelectedIndex();

	if (Settings && SelectionType != ESelectInfo::Direct && Index != INDEX_NONE)
	{
		// The options are in EWindowMode order: Fullscreen, WindowedFullscreen, Windowed.
		Settings->SetFullscreenMode(static_cast<EWindowMode::Type>(Index));
	}
}

void USettingsWidget::HandleQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	UCardboardUserSettings* Settings = UCardboardUserSettings::GetCardboardUserSettings();
	const int32 Index = QualityBox->GetSelectedIndex();

	if (Settings && SelectionType != ESelectInfo::Direct && Index != INDEX_NONE)
	{
		Settings->SetOverallScalabilityLevel(Index);
	}
}

void USettingsWidget::HandleFrameRateChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	UCardboardUserSettings* Settings = UCardboardUserSettings::GetCardboardUserSettings();
	const int32 Index = FrameRateBox->GetSelectedIndex();

	if (Settings && SelectionType != ESelectInfo::Direct && FrameRates.IsValidIndex(Index))
	{
		Settings->SetFrameRateLimit(FrameRates[Index]);
	}
}

void USettingsWidget::HandleVSyncChanged(bool bIsChecked)
{
	if (UCardboardUserSettings* Settings = UCardboardUserSettings::GetCardboardUserSettings())
	{
		Settings->SetVSyncEnabled(bIsChecked);
	}
}

void USettingsWidget::HandleResolutionScaleChanged(float Value)
{
	if (UCardboardUserSettings* Settings = UCardboardUserSettings::GetCardboardUserSettings())
	{
		Settings->SetResolutionScaleValueEx(Value);
		UpdateValueTexts();
	}
}

void USettingsWidget::HandleGammaChanged(float Value)
{
	if (UCardboardUserSettings* Settings = UCardboardUserSettings::GetCardboardUserSettings())
	{
		Settings->SetGamma(Value);
		Settings->ApplyAudioAndGamma(this);
		UpdateValueTexts();
	}
}

void USettingsWidget::HandleFieldOfViewChanged(float Value)
{
	if (UCardboardUserSettings* Settings = UCardboardUserSettings::GetCardboardUserSettings())
	{
		// The pawn reads it every frame, so this previews straight away.
		Settings->SetFieldOfView(Value);
		UpdateValueTexts();
	}
}

void USettingsWidget::HandleMasterVolumeChanged(float Value)
{
	if (UCardboardUserSettings* Settings = UCardboardUserSettings::GetCardboardUserSettings())
	{
		Settings->SetMasterVolume(Value);
		Settings->ApplyAudioAndGamma(this);
		UpdateValueTexts();
	}
}

void USettingsWidget::HandleMusicVolumeChanged(float Value)
{
	if (UCardboardUserSettings* Settings = UCardboardUserSettings::GetCardboardUserSettings())
	{
		Settings->SetMusicVolume(Value);
		Settings->ApplyAudioAndGamma(this);
		UpdateValueTexts();
	}
}

void USettingsWidget::HandleEffectsVolumeChanged(float Value)
{
	if (UCardboardUserSettings* Settings = UCardboardUserSettings::GetCardboardUserSettings())
	{
		Settings->SetEffectsVolume(Value);
		Settings->ApplyAudioAndGamma(this);
		UpdateValueTexts();
	}
}

void USettingsWidget::HandleSensitivityChanged(float Value)
{
	if (UCardboardUserSettings* Settings = UCardboardUserSettings::GetCardboardUserSettings())
	{
		Settings->SetMouseSensitivity(Value);
		UpdateValueTexts();
	}
}

void USettingsWidget::HandleApplyClicked()
{
	if (UCardboardUserSettings* Settings = UCardboardUserSettings::GetCardboardUserSettings())
	{
		// Applies resolution, window mode, quality, V-Sync, frame limit and resolution scale, then saves.
		Settings->ApplySettings(false);
		Settings->ApplyAudioAndGamma(this);
		RefreshFromSettings();
	}
}

void USettingsWidget::HandleResetClicked()
{
	if (UCardboardUserSettings* Settings = UCardboardUserSettings::GetCardboardUserSettings())
	{
		Settings->SetToDefaults();
		Settings->ApplyAudioAndGamma(this);
		RefreshFromSettings();
	}
}

void USettingsWidget::HandleBackClicked()
{
	if (UCardboardUserSettings* Settings = UCardboardUserSettings::GetCardboardUserSettings())
	{
		// Throw away anything not applied: reload what's saved and undo the previews.
		Settings->LoadSettings(true);
		Settings->ApplyAudioAndGamma(this);
		RefreshFromSettings();
	}

	OnClosed.Broadcast();
}
