#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Global/Audio/WpAudioTypes.h"

namespace WpAudioSynth
{
	TArray<int16> MakeSoundtrack(const EWpSoundtrack Soundtrack);
	TArray<int16> MakeUi(const EWpUiSound Sound);
	TArray<int16> MakeSfx(const EWpSfx Sound);
}
