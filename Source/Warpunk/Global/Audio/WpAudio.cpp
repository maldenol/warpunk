#include "WpAudio.h"

#include "Warpunk/Global/Audio/WpAudioSynth.h"
#include "Warpunk/Global/Audio/WpPcmSoundWave.h"

#include "Components/AudioComponent.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundConcurrency.h"

namespace
{
	constexpr int32 SoundtrackCount = 4;
	constexpr int32 UiCount = 4;
	constexpr int32 SfxCount = 14;

	int32 SfxPoolSize(const EWpSfx Sound)
	{
		switch (Sound)
		{
		case EWpSfx::Projectile:
		case EWpSfx::Impact:
			return 12;
		case EWpSfx::Damage:
			return 8;
		case EWpSfx::HitScan:
			return 6;
		default:
			return 4;
		}
	}

	float UiVolume(const EWpUiSound Sound)
	{
		switch (Sound)
		{
		case EWpUiSound::Hover:
			return 0.1f;
		case EWpUiSound::Select:
			return 0.42f;
		case EWpUiSound::Deselect:
			return 0.32f;
		case EWpUiSound::Press:
		default:
			return 0.36f;
		}
	}

	float SfxVolume(const EWpSfx Sound)
	{
		switch (Sound)
		{
		case EWpSfx::Projectile:
			return 0.55f;
		case EWpSfx::Impact:
			return 0.78f;
		case EWpSfx::HitScan:
			return 0.42f;
		case EWpSfx::Damage:
			return 0.34f;
		case EWpSfx::Destruction:
			return 0.9f;
		case EWpSfx::DestructionBuilding:
			return 1.0f;
		case EWpSfx::Production:
			return 0.62f;
		case EWpSfx::ProductionComplete:
			return 0.7f;
		case EWpSfx::Move:
			return 0.55f;
		case EWpSfx::Attack:
			return 0.62f;
		case EWpSfx::Stop:
			return 0.5f;
		case EWpSfx::Assist:
			return 0.46f;
		case EWpSfx::Reclaim:
			return 0.5f;
		case EWpSfx::SelfDestruct:
		default:
			return 0.66f;
		}
	}

	float SfxPitch(const EWpSfx Sound)
	{
		switch (Sound)
		{
		case EWpSfx::Projectile:
		case EWpSfx::Impact:
		case EWpSfx::HitScan:
		case EWpSfx::Damage:
			return FMath::FRandRange(0.94f, 1.06f);
		default:
			return 1.0f;
		}
	}
}

static_assert(static_cast<int32>(EWpSoundtrack::Game) == SoundtrackCount - 1, "Soundtrack voice table is out of date.");
static_assert(static_cast<int32>(EWpUiSound::Deselect) == UiCount - 1, "UI voice table is out of date.");
static_assert(static_cast<int32>(EWpSfx::SelfDestruct) == SfxCount - 1, "SFX voice table is out of date.");

void UWpAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	BuildSounds();
	FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UWpAudioSubsystem::OnPostWorldInitialization);

	if (UWorld* const World = GetWorld())
	{
		OnPostWorldInitialization(World, UWorld::InitializationValues{});
	}
}

void UWpAudioSubsystem::Deinitialize()
{
	FWorldDelegates::OnPostWorldInitialization.RemoveAll(this);

	if (MusicComponent)
	{
		MusicComponent->Stop();
		MusicComponent = nullptr;
	}
	if (FadingMusicComponent)
	{
		FadingMusicComponent->Stop();
		FadingMusicComponent = nullptr;
	}

	Super::Deinitialize();
}

UWpAudioSubsystem* UWpAudioSubsystem::Get(const UObject* const WorldContext)
{
	if (!WorldContext)
	{
		return nullptr;
	}

	const UWorld* const World = WorldContext->GetWorld();
	if (!World || (World->GetNetMode() == NM_DedicatedServer))
	{
		return nullptr;
	}

	UGameInstance* const GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<UWpAudioSubsystem>();
}

void UWpAudioSubsystem::ArmButton(const UObject* const WorldContext, UButton* const Button)
{
	UWpAudioSubsystem* const Audio = Get(WorldContext);
	if (!Audio || !Button)
	{
		return;
	}

	Button->OnPressed.AddUniqueDynamic(Audio, &UWpAudioSubsystem::OnUiPressed);
	Button->OnHovered.AddUniqueDynamic(Audio, &UWpAudioSubsystem::OnUiHovered);
}

void UWpAudioSubsystem::ArmCheckBox(const UObject* const WorldContext, UCheckBox* const CheckBox)
{
	UWpAudioSubsystem* const Audio = Get(WorldContext);
	if (!Audio || !CheckBox)
	{
		return;
	}

	CheckBox->OnCheckStateChanged.AddUniqueDynamic(Audio, &UWpAudioSubsystem::OnUiCheckChanged);
}

void UWpAudioSubsystem::PlayUi(const UObject* const WorldContext, const EWpUiSound Sound)
{
	UWpAudioSubsystem* const Audio = Get(WorldContext);
	if (!Audio)
	{
		return;
	}

	const int32 SoundIndex = static_cast<int32>(Sound);
	const FWpVoiceRange& Range = Audio->UiVoices[SoundIndex];
	const int32 VoiceIndex = Audio->Checkout(Range.First, Range.Num, Audio->UiPcm[SoundIndex], 1, SOUNDGROUP_UI);
	if (VoiceIndex == INDEX_NONE)
	{
		return;
	}

	Audio->PlayVoice(WorldContext, VoiceIndex, nullptr, UiVolume(Sound), 1.0f);
}

void UWpAudioSubsystem::PlaySfx(const UObject* const WorldContext, const EWpSfx Sound, const FVector& Location)
{
	UWpAudioSubsystem* const Audio = Get(WorldContext);
	if (!Audio)
	{
		return;
	}

	const int32 SoundIndex = static_cast<int32>(Sound);
	const FWpVoiceRange& Range = Audio->SfxVoices[SoundIndex];
	const float Pitch = SfxPitch(Sound);
	const int32 VoiceIndex = Audio->Checkout(Range.First, Range.Num, Audio->SfxPcm[SoundIndex], 1, SOUNDGROUP_Effects);
	if (VoiceIndex == INDEX_NONE)
	{
		return;
	}

	Audio->PlayVoice(WorldContext, VoiceIndex, &Location, SfxVolume(Sound), Pitch);
}

void UWpAudioSubsystem::OnUiPressed()
{
	PlayUi(this, EWpUiSound::Press);
}

void UWpAudioSubsystem::OnUiHovered()
{
	PlayUi(this, EWpUiSound::Hover);
}

void UWpAudioSubsystem::OnUiCheckChanged(bool bIsChecked)
{
	static_cast<void>(bIsChecked);
	PlayUi(this, EWpUiSound::Press);
}

void UWpAudioSubsystem::OnPostWorldInitialization(UWorld* const World, const UWorld::InitializationValues InitializationValues)
{
	static_cast<void>(InitializationValues);

	if (!CanPlayInWorld(World))
	{
		return;
	}

	PendingWorld = World;
	if (bStartQueued)
	{
		return;
	}

	bStartQueued = true;
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UWpAudioSubsystem::StartPendingSoundtrack));
}

bool UWpAudioSubsystem::StartPendingSoundtrack(const float DeltaTime)
{
	static_cast<void>(DeltaTime);
	bStartQueued = false;

	if (UWorld* const World = PendingWorld.Get())
	{
		StartSoundtrack(World);
	}

	return false;
}

void UWpAudioSubsystem::BuildSounds()
{
	SfxAttenuation = NewObject<USoundAttenuation>(this);
	SfxAttenuation->Attenuation.bAttenuate = true;
	SfxAttenuation->Attenuation.bSpatialize = true;
	SfxAttenuation->Attenuation.DistanceAlgorithm = EAttenuationDistanceModel::Linear;
	SfxAttenuation->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
	SfxAttenuation->Attenuation.AttenuationShapeExtents = FVector(1500.0f, 0.0f, 0.0f);
	SfxAttenuation->Attenuation.FalloffDistance = 20000.0f;

	SfxConcurrency = NewObject<USoundConcurrency>(this);
	SfxConcurrency->Concurrency.MaxCount = 32;
	SfxConcurrency->Concurrency.ResolutionRule = EMaxConcurrentResolutionRule::StopFarthestThenPreventNew;

	SoundtrackWaves.SetNum(SoundtrackCount);
	for (int32 Index = static_cast<int32>(EWpSoundtrack::Menu); Index < SoundtrackCount; ++Index)
	{
		const EWpSoundtrack Soundtrack = static_cast<EWpSoundtrack>(Index);
		SoundtrackPcm[Index] = WpAudioSynth::MakeSoundtrack(Soundtrack);

		UWpPcmSoundWave* const Wave = NewObject<UWpPcmSoundWave>(this);
		Wave->SetBuffer(SoundtrackPcm[Index], 2, true, SOUNDGROUP_Music);
		SoundtrackWaves[Index] = Wave;
	}

	int32 VoiceCursor = 0;
	for (int32 Index = 0; Index < UiCount; ++Index)
	{
		UiPcm[Index] = WpAudioSynth::MakeUi(static_cast<EWpUiSound>(Index));
		UiVoices[Index].First = VoiceCursor;
		UiVoices[Index].Num = 4;
		VoiceCursor += UiVoices[Index].Num;
	}
	for (int32 Index = 0; Index < SfxCount; ++Index)
	{
		SfxPcm[Index] = WpAudioSynth::MakeSfx(static_cast<EWpSfx>(Index));
		SfxVoices[Index].First = VoiceCursor;
		SfxVoices[Index].Num = SfxPoolSize(static_cast<EWpSfx>(Index));
		VoiceCursor += SfxVoices[Index].Num;
	}

	Voices.Reserve(VoiceCursor);
	bVoiceBusy.Init(false, VoiceCursor);
	for (int32 Index = 0; Index < VoiceCursor; ++Index)
	{
		Voices.Add(NewObject<UWpPcmSoundWave>(this));
	}
}

void UWpAudioSubsystem::StartSoundtrack(UWorld* const World)
{
	if (!CanPlayInWorld(World))
	{
		return;
	}

	const EWpSoundtrack Desired = SoundtrackForWorld(World);
	if (Desired == EWpSoundtrack::None)
	{
		return;
	}

	const int32 SoundtrackIndex = static_cast<int32>(Desired);
	UWpPcmSoundWave* const Wave = SoundtrackWaves[SoundtrackIndex];
	if (!Wave)
	{
		return;
	}

	// LobbyMap and TransitionMap share one soundtrack. While it is already playing, leave it
	// running so the lobby -> game travel does not restart or replace it.
	if ((Desired == CurrentSoundtrack) && MusicComponent && MusicComponent->IsPlaying())
	{
		return;
	}

	const bool bContinue = Desired == CurrentSoundtrack;
	if (!bContinue)
	{
		Wave->ResetCursor();
		if (FadingMusicComponent)
		{
			FadingMusicComponent->Stop();
			FadingMusicComponent = nullptr;
		}
		if (MusicComponent)
		{
			MusicComponent->FadeOut(SoundtrackFadeSeconds, 0.0f);
			FadingMusicComponent = MusicComponent;
			MusicComponent = nullptr;
		}
	}

	if (!MusicComponent)
	{
		MusicComponent = UGameplayStatics::CreateSound2D(World, Wave, MusicVolume, 1.0f, 0.0f, nullptr, true, false);
		if (!MusicComponent)
		{
			return;
		}
	}
	else if (MusicComponent->GetSound() != Wave)
	{
		MusicComponent->SetSound(Wave);
	}

	CurrentSoundtrack = Desired;
	if (bContinue)
	{
		MusicComponent->SetVolumeMultiplier(MusicVolume);
		if (!MusicComponent->IsPlaying())
		{
			MusicComponent->Play();
		}
		return;
	}

	MusicComponent->FadeIn(SoundtrackFadeSeconds, MusicVolume);
}

bool UWpAudioSubsystem::CanPlayInWorld(const UWorld* const World) const
{
	if (!World || !World->IsGameWorld())
	{
		return false;
	}
	if (World->GetGameInstance() != GetGameInstance())
	{
		return false;
	}
	if (World->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	return true;
}

EWpSoundtrack UWpAudioSubsystem::SoundtrackForWorld(const UWorld* const World) const
{
	const FString MapName = World->GetMapName();
	if (MapName.Contains(TEXT("Transition")))
	{
		return EWpSoundtrack::Lobby;
	}
	if (MapName.Contains(TEXT("Menu")))
	{
		return EWpSoundtrack::Menu;
	}
	if (MapName.Contains(TEXT("Game")))
	{
		return EWpSoundtrack::Game;
	}

	return EWpSoundtrack::Lobby;
}

int32 UWpAudioSubsystem::Checkout(const int32 FirstVoice, const int32 VoiceNum, const TArray<int16>& Pcm, const int32 NumChannels, const ESoundGroup SoundGroup)
{
	for (int32 Offset = 0; Offset < VoiceNum; ++Offset)
	{
		const int32 VoiceIndex = FirstVoice + Offset;
		if (bVoiceBusy[VoiceIndex])
		{
			continue;
		}

		bVoiceBusy[VoiceIndex] = true;
		Voices[VoiceIndex]->SetBuffer(Pcm, NumChannels, false, SoundGroup);
		return VoiceIndex;
	}

	return INDEX_NONE;
}

void UWpAudioSubsystem::PlayVoice(const UObject* const WorldContext, const int32 VoiceIndex, const FVector* const Location, const float Volume, const float Pitch)
{
	UWpPcmSoundWave* const Wave = Voices[VoiceIndex];
	UAudioComponent* Component = nullptr;
	if (Location)
	{
		Component = UGameplayStatics::SpawnSoundAtLocation(WorldContext, Wave, *Location, FRotator::ZeroRotator, Volume, Pitch, 0.0f, SfxAttenuation, SfxConcurrency, true);
	}
	else
	{
		Component = UGameplayStatics::SpawnSound2D(WorldContext, Wave, Volume, Pitch, 0.0f, nullptr, false, true);
	}

	if (!Component)
	{
		bVoiceBusy[VoiceIndex] = false;
		return;
	}

	VoiceByComponent.Add(Component, VoiceIndex);
	Component->OnAudioFinishedNative.AddUObject(this, &UWpAudioSubsystem::OnVoiceFinished);

	UWorld* const World = WorldContext->GetWorld();
	if (!World)
	{
		return;
	}

	const float StopAfter = FMath::Max(Wave->GetDuration(), 0.05f) / FMath::Max(Pitch, 0.1f) + 0.05f;
	TWeakObjectPtr<UWpAudioSubsystem> WeakThis = this;
	TWeakObjectPtr<UAudioComponent> WeakComponent = Component;
	FTimerHandle TimerHandle;
	World->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateLambda([WeakThis, WeakComponent]()
	{
		UAudioComponent* const AudioComponent = WeakComponent.Get();
		if (AudioComponent)
		{
			AudioComponent->Stop();
		}
		if (UWpAudioSubsystem* const Audio = WeakThis.Get())
		{
			Audio->ReleaseVoice(AudioComponent);
		}
	}), StopAfter, false);
}

void UWpAudioSubsystem::OnVoiceFinished(UAudioComponent* const Component)
{
	ReleaseVoice(Component);
}

void UWpAudioSubsystem::ReleaseVoice(UAudioComponent* const Component)
{
	if (!Component)
	{
		return;
	}

	if (const int32* const VoiceIndex = VoiceByComponent.Find(Component))
	{
		bVoiceBusy[*VoiceIndex] = false;
		VoiceByComponent.Remove(Component);
	}
}
