#include "CardboardUserSettings.h"

#include "CardboardSettings.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

UCardboardUserSettings* UCardboardUserSettings::GetCardboardUserSettings()
{
	return GEngine ? Cast<UCardboardUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

void UCardboardUserSettings::ApplyAudioAndGamma(const UObject* WorldContextObject)
{
	if (GEngine)
	{
		GEngine->DisplayGamma = Gamma;
	}

	const UCardboardSettings* ProjectSettings = GetDefault<UCardboardSettings>();
	USoundMix* VolumeMix = ProjectSettings->VolumeSoundMix.LoadSynchronous();

	if (!VolumeMix || !WorldContextObject)
	{
		return;
	}

	// Each class gets its own volume. Music and Effects are children of Master in the sound class
	// hierarchy, so the engine multiplies Master into them by itself.
	auto SetClassVolume = [WorldContextObject, VolumeMix](const TSoftObjectPtr<USoundClass>& SoundClassPtr, float Volume)
	{
		if (USoundClass* SoundClass = SoundClassPtr.LoadSynchronous())
		{
			UGameplayStatics::SetSoundMixClassOverride(WorldContextObject, VolumeMix, SoundClass, Volume, 1.0f, 0.0f, false);
		}
	};

	SetClassVolume(ProjectSettings->MasterSoundClass, MasterVolume);
	SetClassVolume(ProjectSettings->MusicSoundClass, MusicVolume);
	SetClassVolume(ProjectSettings->EffectsSoundClass, EffectsVolume);

	UGameplayStatics::PushSoundMixModifier(WorldContextObject, VolumeMix);
}

void UCardboardUserSettings::SetToDefaults()
{
	Super::SetToDefaults();

	MasterVolume = 1.0f;
	MusicVolume = 1.0f;
	EffectsVolume = 1.0f;
	Gamma = 2.2f;
	MouseSensitivity = 1.0f;
	FieldOfView = 90.0f;
}

void UCardboardUserSettings::ApplyNonResolutionSettings()
{
	Super::ApplyNonResolutionSettings();

	if (GEngine)
	{
		GEngine->DisplayGamma = Gamma;
	}
}
