#pragma once

#include "CoreMinimal.h"

#include "Sound/SoundWaveProcedural.h"

#include "WpPcmSoundWave.generated.h"

UCLASS(MinimalAPI)
class UWpPcmSoundWave final : public USoundWaveProcedural
{
	GENERATED_BODY()

public:
	UWpPcmSoundWave(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void SetBuffer(const TArray<int16>& InPcm, const int32 InNumChannels, const bool bInLoop, const ESoundGroup InSoundGroup);
	void ResetCursor();

	virtual int32 GeneratePCMData(uint8* PCMData, const int32 SamplesNeeded) override;

	static constexpr int32 SampleRate = 22050;

private:
	const TArray<int16>* Pcm = nullptr;
	int32 Cursor = 0;
	bool bLoop = false;
	mutable FCriticalSection GenerateLock;
};
