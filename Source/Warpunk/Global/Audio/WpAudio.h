#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Global/Audio/WpAudioTypes.h"

#include "Subsystems/GameInstanceSubsystem.h"

#include "WpAudio.generated.h"

class UAudioComponent;
class UButton;
class UCheckBox;
class USoundAttenuation;
class USoundConcurrency;
class UWpPcmSoundWave;

UCLASS(MinimalAPI)
class UWpAudioSubsystem final : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static UWpAudioSubsystem* Get(const UObject* const WorldContext);

	static void ArmButton(const UObject* const WorldContext, UButton* const Button);
	static void ArmCheckBox(const UObject* const WorldContext, UCheckBox* const CheckBox);
	static void PlayUi(const UObject* const WorldContext, const EWpUiSound Sound);
	static void PlaySfx(const UObject* const WorldContext, const EWpSfx Sound, const FVector& Location);

private:
	UFUNCTION()
	void OnUiPressed();
	UFUNCTION()
	void OnUiHovered();
	UFUNCTION()
	void OnUiCheckChanged(bool bIsChecked);

	void OnPostWorldInitialization(UWorld* World, const UWorld::InitializationValues InitializationValues);
	bool StartPendingSoundtrack(const float DeltaTime);

	void BuildSounds();
	void StartSoundtrack(UWorld* const World);
	bool CanPlayInWorld(const UWorld* const World) const;
	EWpSoundtrack SoundtrackForWorld(const UWorld* const World) const;

	int32 Checkout(const int32 FirstVoice, const int32 VoiceNum, const TArray<int16>& Pcm, const int32 NumChannels, const ESoundGroup SoundGroup);
	void PlayVoice(const UObject* const WorldContext, const int32 VoiceIndex, const FVector* const Location, const float Volume, const float Pitch);
	void OnVoiceFinished(UAudioComponent* const Component);
	void ReleaseVoice(UAudioComponent* const Component);

	struct FWpVoiceRange
	{
		int32 First = 0;
		int32 Num = 0;
	};

	TArray<int16> SoundtrackPcm[4] = {};
	TArray<int16> UiPcm[4] = {};
	TArray<int16> SfxPcm[14] = {};

	UPROPERTY()
	TArray<TObjectPtr<UWpPcmSoundWave>> SoundtrackWaves = {};
	UPROPERTY()
	TArray<TObjectPtr<UWpPcmSoundWave>> Voices = {};
	TArray<bool> bVoiceBusy = {};
	TMap<UAudioComponent*, int32> VoiceByComponent = {};

	FWpVoiceRange UiVoices[4] = {};
	FWpVoiceRange SfxVoices[14] = {};

	UPROPERTY()
	TObjectPtr<UAudioComponent> MusicComponent = nullptr;
	UPROPERTY()
	TObjectPtr<UAudioComponent> FadingMusicComponent = nullptr;
	UPROPERTY()
	TObjectPtr<USoundAttenuation> SfxAttenuation = nullptr;
	UPROPERTY()
	TObjectPtr<USoundConcurrency> SfxConcurrency = nullptr;

	EWpSoundtrack CurrentSoundtrack = EWpSoundtrack::None;
	TWeakObjectPtr<UWorld> PendingWorld = nullptr;
	bool bStartQueued = false;

	static constexpr float MusicVolume = 0.48f;
	static constexpr float SoundtrackFadeSeconds = 1.5f;
};
