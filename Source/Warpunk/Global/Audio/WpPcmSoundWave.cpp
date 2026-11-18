#include "WpPcmSoundWave.h"

UWpPcmSoundWave::UWpPcmSoundWave(const FObjectInitializer& ObjectInitializer) : USoundWaveProcedural(ObjectInitializer)
{
	NumBufferUnderrunSamples = 1;
	NumSamplesToGeneratePerCallback = 2048;
	VirtualizationMode = EVirtualizationMode::Disabled;
	SoundGroup = SOUNDGROUP_Effects;
	SetSampleRate(SampleRate);
	NumChannels = 1;
	Duration = 0.1f;
	bLooping = false;
}

void UWpPcmSoundWave::SetBuffer(const TArray<int16>& InPcm, const int32 InNumChannels, const bool bInLoop, const ESoundGroup InSoundGroup)
{
	FScopeLock Lock(&GenerateLock);

	Pcm = &InPcm;
	bLoop = bInLoop;
	bLooping = bInLoop;
	NumChannels = InNumChannels;
	SoundGroup = InSoundGroup;
	Cursor = 0;

	const int32 FrameCount = (InNumChannels > 0) ? (InPcm.Num() / InNumChannels) : 0;
	Duration = static_cast<float>(FrameCount) / static_cast<float>(SampleRate);
}

void UWpPcmSoundWave::ResetCursor()
{
	FScopeLock Lock(&GenerateLock);
	Cursor = 0;
}

int32 UWpPcmSoundWave::GeneratePCMData(uint8* const PCMData, const int32 SamplesNeeded)
{
	if (!PCMData || (SamplesNeeded <= 0))
	{
		return 0;
	}

	FScopeLock Lock(&GenerateLock);
	const int32 BytesNeeded = SamplesNeeded * static_cast<int32>(sizeof(int16));
	if (!Pcm || (Pcm->Num() == 0))
	{
		FMemory::Memzero(PCMData, static_cast<SIZE_T>(BytesNeeded));
		return BytesNeeded;
	}

	int16* const Dest = reinterpret_cast<int16*>(PCMData);
	const int16* const Source = Pcm->GetData();
	const int32 SourceNum = Pcm->Num();

	for (int32 Index = 0; Index < SamplesNeeded; ++Index)
	{
		if (Cursor >= SourceNum)
		{
			if (!bLoop)
			{
				FMemory::Memzero(Dest + Index, static_cast<SIZE_T>(SamplesNeeded - Index) * sizeof(int16));
				break;
			}

			Cursor = 0;
		}

		Dest[Index] = Source[Cursor];
		++Cursor;
	}

	return BytesNeeded;
}
