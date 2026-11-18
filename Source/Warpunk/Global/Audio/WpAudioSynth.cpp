#include "WpAudioSynth.h"

#include "Warpunk/Global/Audio/WpPcmSoundWave.h"

namespace
{
	constexpr float TwoPi = 2.0f * PI;

	struct FRng
	{
		uint32 State = 1;

		float Next()
		{
			State = State * 1664525u + 1013904223u;
			return (static_cast<float>(State >> 8) / 16777216.0f) * 2.0f - 1.0f;
		}
	};

	float MidiHz(const int32 Midi)
	{
		return 440.0f * FMath::Pow(2.0f, (static_cast<float>(Midi) - 69.0f) / 12.0f);
	}

	TArray<int16> ToPcm(const TArray<float>& Samples, const float TargetPeak)
	{
		float Peak = 0.001f;
		for (const float Sample : Samples)
		{
			Peak = FMath::Max(Peak, FMath::Abs(Sample));
		}

		const float Gain = TargetPeak / Peak;
		TArray<int16> Pcm;
		Pcm.SetNumUninitialized(Samples.Num());
		for (int32 Index = 0; Index < Samples.Num(); ++Index)
		{
			const float Clamped = FMath::Clamp(Samples[Index] * Gain, -1.0f, 1.0f);
			Pcm[Index] = static_cast<int16>(Clamped * 32767.0f);
		}

		return Pcm;
	}

	float AttackDecay(const float Time, const float Attack, const float Decay)
	{
		if (Time < Attack)
		{
			return Time / FMath::Max(Attack, 0.0001f);
		}

		return FMath::Exp(-(Time - Attack) * Decay);
	}

	struct FScore
	{
		float Bpm = 90.0f;
		int32 RootMidi = 45;
		float Kick = 0.2f;
		float Hat = 0.0f;
		float Bass = 0.3f;
		float Pad = 0.14f;
		float Lead = 0.12f;
		int32 LeadSteps[16] = {};
		uint32 Seed = 1;
	};

	TArray<int16> RenderSoundtrack(const FScore& Score)
	{
		struct FChord
		{
			int32 Root = 0;
			bool bMinor = true;
		};

		const FChord Chords[4] =
		{
			{0, true},
			{-4, false},
			{3, false},
			{-2, false},
		};

		const int32 SampleRate = UWpPcmSoundWave::SampleRate;
		const int32 SamplesPerBeat = FMath::Max(1, FMath::RoundToInt(60.0f / Score.Bpm * static_cast<float>(SampleRate)));
		const int32 SamplesPerEighth = FMath::Max(1, SamplesPerBeat / 2);
		const int32 SamplesPerBar = SamplesPerBeat * 4;
		const int32 Frames = SamplesPerBar * 16;
		const int32 Fade = FMath::RoundToInt(0.04f * static_cast<float>(SampleRate));
		const int32 TotalFrames = Frames + Fade;

		TArray<float> Stereo;
		Stereo.SetNumZeroed(TotalFrames * 2);

		float KickEnv = 0.0f;
		float KickPhase = 0.0f;
		float HatEnv = 0.0f;
		float HatLp = 0.0f;
		float BassPhase = 0.0f;
		float PadPhaseL[3] = {};
		float PadPhaseR[3] = {};
		float LeadPhase = 0.0f;
		FRng Rng;
		Rng.State = Score.Seed;

		for (int32 Frame = 0; Frame < TotalFrames; ++Frame)
		{
			const int32 Bar = Frame / SamplesPerBar;
			const int32 BeatInBar = (Frame / SamplesPerBeat) % 4;
			const int32 FrameInBeat = Frame % SamplesPerBeat;
			const int32 Eighth = Frame / SamplesPerEighth;
			const int32 Step = Eighth % 16;
			const int32 FrameInStep = Frame % SamplesPerEighth;
			const bool bBeatStart = FrameInBeat == 0;
			const bool bEighthStart = (Frame % SamplesPerEighth) == 0;
			const FChord& Chord = Chords[(Bar / 2) % 4];
			const int32 Root = Score.RootMidi + Chord.Root;

			const bool bKickOnBeat = (Score.Hat > 0.12f) || (BeatInBar == 0) || ((Score.Hat > 0.01f) && (BeatInBar == 2));
			if (bBeatStart && bKickOnBeat && (Score.Kick > 0.0f))
			{
				KickEnv = 1.0f;
				KickPhase = 0.0f;
			}
			if (bEighthStart && (Score.Hat > 0.0f))
			{
				HatEnv = ((Eighth % 2) == 0) ? 0.55f : 1.0f;
			}

			KickEnv *= FMath::Exp(-26.0f / static_cast<float>(SampleRate));
			HatEnv *= FMath::Exp(-78.0f / static_cast<float>(SampleRate));

			const float KickFreq = 46.0f + 150.0f * KickEnv;
			KickPhase = FMath::Fmod(KickPhase + KickFreq / static_cast<float>(SampleRate), 1.0f);
			const float KickSample = FMath::Sin(KickPhase * TwoPi) * KickEnv * Score.Kick;

			HatLp += (Rng.Next() - HatLp) * 0.22f;
			const float HatSample = HatLp * HatEnv * Score.Hat;

			const float BassHz = MidiHz(Root - 12);
			BassPhase = FMath::Fmod(BassPhase + BassHz / static_cast<float>(SampleRate), 1.0f);
			float BassEnv = 1.0f;
			if (Score.Hat > 0.12f)
			{
				BassEnv = AttackDecay(static_cast<float>(Frame % SamplesPerEighth) / static_cast<float>(SampleRate), 0.004f, 8.0f);
			}
			else if (Score.Hat > 0.01f)
			{
				BassEnv = AttackDecay(static_cast<float>(FrameInBeat) / static_cast<float>(SampleRate), 0.006f, 3.5f);
			}
			const float BassSample = FMath::Sin(BassPhase * TwoPi) * BassEnv * Score.Bass;

			const int32 Third = Chord.bMinor ? 3 : 4;
			const int32 Tones[3] = {Root, Root + Third, Root + 7};
			float PadL = 0.0f;
			float PadR = 0.0f;
			for (int32 ToneIndex = 0; ToneIndex < 3; ++ToneIndex)
			{
				int32 Midi = Tones[ToneIndex];
				while (Midi < 52)
				{
					Midi += 12;
				}
				while (Midi > 76)
				{
					Midi -= 12;
				}

				const float Hz = MidiHz(Midi);
				PadPhaseL[ToneIndex] = FMath::Fmod(PadPhaseL[ToneIndex] + Hz / static_cast<float>(SampleRate), 1.0f);
				PadPhaseR[ToneIndex] = FMath::Fmod(PadPhaseR[ToneIndex] + Hz * 1.004f / static_cast<float>(SampleRate), 1.0f);
				PadL += FMath::Sin(PadPhaseL[ToneIndex] * TwoPi);
				PadR += FMath::Sin(PadPhaseR[ToneIndex] * TwoPi);
			}
			const float PadEnv = 0.75f + 0.25f * FMath::Sin(TwoPi * static_cast<float>(Frame) / static_cast<float>(SampleRate) * 0.08f);
			PadL *= Score.Pad * PadEnv;
			PadR *= Score.Pad * PadEnv;

			float LeadSample = 0.0f;
			if (Score.LeadSteps[Step] >= 0)
			{
				const float StepTime = static_cast<float>(FrameInStep) / static_cast<float>(SampleRate);
				const float LeadHz = MidiHz(Root + Score.LeadSteps[Step] + 12);
				LeadPhase = FMath::Fmod(LeadPhase + LeadHz / static_cast<float>(SampleRate), 1.0f);
				LeadSample = FMath::Sin(LeadPhase * TwoPi) * AttackDecay(StepTime, 0.007f, 5.5f) * Score.Lead;
			}

			const float Duck = 1.0f - 0.62f * KickEnv;
			const float Pan = ((Step % 2) == 0) ? 0.28f : -0.28f;
			const float Left = KickSample + HatSample + BassSample * Duck + PadL * Duck + LeadSample * (0.75f + Pan);
			const float Right = KickSample + HatSample + BassSample * Duck + PadR * Duck + LeadSample * (0.75f - Pan);
			Stereo[Frame * 2] = Left;
			Stereo[Frame * 2 + 1] = Right;
		}

		for (int32 Index = 0; Index < Fade; ++Index)
		{
			const float Blend = static_cast<float>(Index) / static_cast<float>(Fade);
			const int32 Head = Index * 2;
			const int32 Tail = (Frames + Index) * 2;
			Stereo[Head] = Stereo[Head] * Blend + Stereo[Tail] * (1.0f - Blend);
			Stereo[Head + 1] = Stereo[Head + 1] * Blend + Stereo[Tail + 1] * (1.0f - Blend);
		}
		Stereo.SetNum(Frames * 2);

		return ToPcm(Stereo, 0.82f);
	}

	TArray<int16> RenderMono(const TFunctionRef<float(float, FRng&)>& SampleAt, const float Seconds, const uint32 Seed)
	{
		const int32 SampleRate = UWpPcmSoundWave::SampleRate;
		const int32 FrameCount = FMath::Max(1, FMath::RoundToInt(Seconds * static_cast<float>(SampleRate)));
		TArray<float> Samples;
		Samples.SetNumUninitialized(FrameCount);
		FRng Rng;
		Rng.State = Seed;
		for (int32 Frame = 0; Frame < FrameCount; ++Frame)
		{
			const float Time = static_cast<float>(Frame) / static_cast<float>(SampleRate);
			Samples[Frame] = SampleAt(Time, Rng);
		}

		return ToPcm(Samples, 0.9f);
	}
}

TArray<int16> WpAudioSynth::MakeSoundtrack(const EWpSoundtrack Soundtrack)
{
	FScore Score;
	if (Soundtrack == EWpSoundtrack::Menu)
	{
		Score = {74.0f, 45, 0.16f, 0.0f, 0.26f, 0.18f, 0.11f, {0, -1, 7, -1, 3, -1, 10, -1, 7, -1, 3, -1, 12, -1, 0, -1}, 11u};
	}
	else if (Soundtrack == EWpSoundtrack::Game)
	{
		Score = {124.0f, 40, 0.5f, 0.16f, 0.34f, 0.1f, 0.2f, {0, 3, 6, 7, 3, 6, 10, 12, 15, 12, 10, 7, 6, 3, 1, 0}, 47u};
	}
	else
	{
		Score = {96.0f, 38, 0.32f, 0.07f, 0.3f, 0.15f, 0.15f, {0, 3, 7, 3, 10, 7, 12, 7, 3, 7, 10, 7, 12, 10, 7, 3}, 29u};
	}

	return RenderSoundtrack(Score);
}

TArray<int16> WpAudioSynth::MakeUi(const EWpUiSound Sound)
{
	switch (Sound)
	{
	case EWpUiSound::Hover:
		return RenderMono([](const float Time, FRng&)
		{
			const float Hz = FMath::Lerp(1700.0f, 1280.0f, FMath::Clamp(Time / 0.045f, 0.0f, 1.0f));
			return FMath::Sin(TwoPi * Hz * Time) * AttackDecay(Time, 0.002f, 46.0f);
		}, 0.05f, 3u);
	case EWpUiSound::Select:
		return RenderMono([](const float Time, FRng&)
		{
			const float Hz = (Time < 0.07f) ? 523.0f : 784.0f;
			const float Local = (Time < 0.07f) ? Time : (Time - 0.07f);
			return FMath::Sin(TwoPi * Hz * Time) * AttackDecay(Local, 0.003f, 18.0f);
		}, 0.16f, 5u);
	case EWpUiSound::Deselect:
		return RenderMono([](const float Time, FRng&)
		{
			const float Hz = (Time < 0.06f) ? 740.0f : 392.0f;
			const float Local = (Time < 0.06f) ? Time : (Time - 0.06f);
			return FMath::Sin(TwoPi * Hz * Time) * AttackDecay(Local, 0.002f, 16.0f);
		}, 0.14f, 6u);
	case EWpUiSound::Press:
	default:
		return RenderMono([](const float Time, FRng& Rng)
		{
			const float Tick = Rng.Next() * AttackDecay(Time, 0.0002f, 160.0f);
			const float Hz = FMath::Lerp(3200.0f, 2100.0f, FMath::Clamp(Time / 0.02f, 0.0f, 1.0f));
			const float Tone = FMath::Sin(TwoPi * Hz * Time) * AttackDecay(Time, 0.0003f, 95.0f);
			const float Over = FMath::Sin(TwoPi * Hz * 1.7f * Time) * AttackDecay(Time, 0.0002f, 130.0f);
			return Tick * 0.18f + Tone * 0.7f + Over * 0.22f;
		}, 0.035f, 4u);
	}
}

TArray<int16> WpAudioSynth::MakeSfx(const EWpSfx Sound)
{
	switch (Sound)
	{
	case EWpSfx::Projectile:
		return RenderMono([](const float Time, FRng& Rng)
		{
			const float Thump = FMath::Sin(TwoPi * 78.0f * Time) * AttackDecay(Time, 0.002f, 22.0f);
			const float Noise = Rng.Next() * AttackDecay(Time, 0.001f, 28.0f);
			return Thump + Noise * 0.45f;
		}, 0.16f, 13u);
	case EWpSfx::Impact:
		return RenderMono([](const float Time, FRng& Rng)
		{
			const float Boom = FMath::Sin(TwoPi * (52.0f + 40.0f * FMath::Exp(-Time * 18.0f)) * Time) * AttackDecay(Time, 0.001f, 8.0f);
			const float Noise = Rng.Next() * AttackDecay(Time, 0.001f, 11.0f);
			return Boom * 0.85f + Noise * 0.7f;
		}, 0.38f, 17u);
	case EWpSfx::HitScan:
		return RenderMono([](const float Time, FRng& Rng)
		{
			return Rng.Next() * AttackDecay(Time, 0.0003f, 70.0f) + FMath::Sin(TwoPi * 1400.0f * Time) * AttackDecay(Time, 0.0005f, 55.0f) * 0.25f;
		}, 0.06f, 19u);
	case EWpSfx::Damage:
		return RenderMono([](const float Time, FRng& Rng)
		{
			return FMath::Sin(TwoPi * 150.0f * Time) * AttackDecay(Time, 0.001f, 30.0f) * 0.7f + Rng.Next() * AttackDecay(Time, 0.001f, 40.0f) * 0.4f;
		}, 0.09f, 23u);
	case EWpSfx::Destruction:
		return RenderMono([](const float Time, FRng& Rng)
		{
			const float Boom = FMath::Sin(TwoPi * 48.0f * Time) * AttackDecay(Time, 0.002f, 5.5f);
			const float Crack = Rng.Next() * AttackDecay(Time, 0.001f, 7.0f);
			return Boom + Crack * 0.85f;
		}, 0.55f, 31u);
	case EWpSfx::DestructionBuilding:
		return RenderMono([](const float Time, FRng& Rng)
		{
			const float Boom = FMath::Sin(TwoPi * 36.0f * Time) * AttackDecay(Time, 0.004f, 3.2f);
			const float Mid = FMath::Sin(TwoPi * 90.0f * Time) * AttackDecay(Time, 0.002f, 6.0f);
			const float Debris = Rng.Next() * AttackDecay(FMath::Max(0.0f, Time - 0.05f), 0.002f, 4.5f);
			return Boom + Mid * 0.45f + Debris * 0.9f;
		}, 0.85f, 37u);
	case EWpSfx::Production:
		return RenderMono([](const float Time, FRng&)
		{
			float Sum = 0.0f;
			for (int32 Pulse = 0; Pulse < 3; ++Pulse)
			{
				const float Local = Time - static_cast<float>(Pulse) * 0.07f;
				if (Local >= 0.0f)
				{
					const float Hz = 280.0f + static_cast<float>(Pulse) * 70.0f;
					Sum += FMath::Sin(TwoPi * Hz * Time) * AttackDecay(Local, 0.004f, 22.0f);
				}
			}
			return Sum;
		}, 0.28f, 41u);
	case EWpSfx::ProductionComplete:
		return RenderMono([](const float Time, FRng&)
		{
			const int32 Notes[3] = {72, 76, 79};
			float Sum = 0.0f;
			for (int32 Note = 0; Note < 3; ++Note)
			{
				const float Local = Time - static_cast<float>(Note) * 0.045f;
				if (Local >= 0.0f)
				{
					Sum += FMath::Sin(TwoPi * MidiHz(Notes[Note]) * Time) * AttackDecay(Local, 0.004f, 6.0f);
				}
			}
			return Sum;
		}, 0.48f, 43u);
	case EWpSfx::Move:
		return RenderMono([](const float Time, FRng&)
		{
			const float Hz = FMath::Lerp(320.0f, 720.0f, FMath::Clamp(Time / 0.12f, 0.0f, 1.0f));
			return FMath::Sin(TwoPi * Hz * Time) * AttackDecay(Time, 0.004f, 12.0f);
		}, 0.14f, 53u);
	case EWpSfx::Attack:
		return RenderMono([](const float Time, FRng& Rng)
		{
			const float Tone = FMath::Sin(TwoPi * 196.0f * Time) * AttackDecay(Time, 0.003f, 10.0f);
			return Tone + Rng.Next() * AttackDecay(Time, 0.001f, 16.0f) * 0.35f;
		}, 0.16f, 59u);
	case EWpSfx::Stop:
		return RenderMono([](const float Time, FRng&)
		{
			return FMath::Sin(TwoPi * 110.0f * Time) * AttackDecay(Time, 0.001f, 24.0f);
		}, 0.1f, 61u);
	case EWpSfx::Assist:
		return RenderMono([](const float Time, FRng&)
		{
			const float Hz = (Time < 0.08f) ? 494.0f : 659.0f;
			const float Local = (Time < 0.08f) ? Time : (Time - 0.08f);
			return FMath::Sin(TwoPi * Hz * Time) * AttackDecay(Local, 0.004f, 14.0f) * 0.8f;
		}, 0.18f, 67u);
	case EWpSfx::Reclaim:
		return RenderMono([](const float Time, FRng&)
		{
			const float Hz = FMath::Lerp(640.0f, 220.0f, FMath::Clamp(Time / 0.18f, 0.0f, 1.0f));
			return FMath::Sin(TwoPi * Hz * Time) * AttackDecay(Time, 0.003f, 8.0f);
		}, 0.2f, 71u);
	case EWpSfx::SelfDestruct:
	default:
		return RenderMono([](const float Time, FRng&)
		{
			float Sum = 0.0f;
			for (int32 Beep = 0; Beep < 2; ++Beep)
			{
				const float Local = Time - static_cast<float>(Beep) * 0.16f;
				if (Local >= 0.0f)
				{
					Sum += FMath::Sin(TwoPi * 880.0f * Time) * AttackDecay(Local, 0.002f, 18.0f);
				}
			}
			return Sum;
		}, 0.36f, 73u);
	}
}
