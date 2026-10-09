#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "CardboardUserSettings.generated.h"

// The player's settings, saved to Saved/Config/<Platform>/GameUserSettings.ini.
// The engine part (resolution, window mode, quality, V-Sync, frame limit, resolution scale)
// comes from UGameUserSettings; this adds volumes, gamma, mouse sensitivity and field of view.
// Registered in DefaultEngine.ini: [/Script/Engine.Engine] GameUserSettingsClassName.
UCLASS(Config = GameUserSettings, ConfigDoNotCheckDefaults)
class CARDBOARD_API UCardboardUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = "Settings")
	static UCardboardUserSettings* GetCardboardUserSettings();

	// Volumes are 0..1.
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMasterVolume(float Volume) { MasterVolume = FMath::Clamp(Volume, 0.0f, 1.0f); }

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetMasterVolume() const { return MasterVolume; }

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMusicVolume(float Volume) { MusicVolume = FMath::Clamp(Volume, 0.0f, 1.0f); }

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetMusicVolume() const { return MusicVolume; }

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetEffectsVolume(float Volume) { EffectsVolume = FMath::Clamp(Volume, 0.0f, 1.0f); }

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetEffectsVolume() const { return EffectsVolume; }

	// Display gamma, 1.5..3.0 (engine default 2.2). Higher = brighter.
	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetGamma(float InGamma) { Gamma = FMath::Clamp(InGamma, MinGamma, MaxGamma); }

	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	float GetGamma() const { return Gamma; }

	// Multiplies the pawn's LookSensitivity, 0.1..3.
	UFUNCTION(BlueprintCallable, Category = "Settings|Controls")
	void SetMouseSensitivity(float Sensitivity) { MouseSensitivity = FMath::Clamp(Sensitivity, 0.1f, 3.0f); }

	UFUNCTION(BlueprintPure, Category = "Settings|Controls")
	float GetMouseSensitivity() const { return MouseSensitivity; }

	// Horizontal field of view in degrees, 70..110.
	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetFieldOfView(float Degrees) { FieldOfView = FMath::Clamp(Degrees, 70.0f, 110.0f); }

	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	float GetFieldOfView() const { return FieldOfView; }

	// Pushes the volumes into the sound mix (set up in Project Settings → Game → Cardboard) and the gamma
	// into the engine. Called on every map load; call it after changing those values to hear/see them.
	UFUNCTION(BlueprintCallable, Category = "Settings", meta = (WorldContext = "WorldContextObject"))
	void ApplyAudioAndGamma(const UObject* WorldContextObject);

	virtual void SetToDefaults() override;
	virtual void ApplyNonResolutionSettings() override;

	static constexpr float MinGamma = 1.5f;
	static constexpr float MaxGamma = 3.0f;

private:

	UPROPERTY(Config)
	float MasterVolume = 1.0f;

	UPROPERTY(Config)
	float MusicVolume = 1.0f;

	UPROPERTY(Config)
	float EffectsVolume = 1.0f;

	UPROPERTY(Config)
	float Gamma = 2.2f;

	UPROPERTY(Config)
	float MouseSensitivity = 1.0f;

	UPROPERTY(Config)
	float FieldOfView = 90.0f;
};
