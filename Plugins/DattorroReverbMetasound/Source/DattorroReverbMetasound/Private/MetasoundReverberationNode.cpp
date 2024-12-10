// Copyright Epic Games, Inc. All Rights Reserved.

#include "Internationalization/Text.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundEnumRegistrationMacro.h"
#include "MetasoundParamHelper.h"
#include "MetasoundPrimitives.h"
#include "MetasoundStandardNodesNames.h"
#include "MetasoundDataTypeRegistrationMacro.h"
#include "MetasoundAudioBuffer.h"
#include "MetasoundFacade.h"
#include "DSP/Dsp.h"
#include "DSP/Filter.h"
#include "DSP/AllPassFilter.h"
#include "DSP/AllPassFractionalDelay.h"
#include "DSP/DynamicDelayAPF.h"
#include "DSP/VoiceProcessing.h"

#define LOCTEXT_NAMESPACE "MetasoundStandardNodesReverberation"

namespace Metasound
{
	namespace Reverberate
	{
		// METASOUND_PARAM: Variable Name - Node Name - Node Description.
		// -------------------- Input --------------------
		METASOUND_PARAM(InParamAudioInput, "In", "Audio input.")
		//PreDelay - 
		METASOUND_PARAM(InParamPreDelay, "PreDelayTime", "Controls how long the pre delay is.") // Clamped between Min and Max PreDelay
		// Pre Low Pass Filter - 
		METASOUND_PARAM(InParamPreLPF, "Pre Low Pass Filter", "Controls intensity of pre low pass filter.") // Clamp Between 0 and 1.
		METASOUND_PARAM(InParamLowPassCutOff, "Low Pass CutOff", "Cut off frequency for low pass filter") // Clamp Between 0 and 1.
		// Pre Diffusion - All Pass
		METASOUND_PARAM(InParamAllPassCutOff, "All Pass Cutoff", ".")
		METASOUND_PARAM(InParamPreDiffuse_1, "Input Diffusion 1", ".") // Clamp Between 0 and 1.
		METASOUND_PARAM(InParamPreDiffuse_2, "Input Diffusion 2", ".") // Clamp Between 0 and 1.
			
		// -------------------- Feedback Tail --------------------
		// Decay Rate
		METASOUND_PARAM(InParamDecayRate, "Decay Rate", ".") // clamp between 0 and 1

		// Decay Diffusion
		METASOUND_PARAM(InParamDecayDiffusion_1, "Decay Diffusion 1", "Delay Value for all pass filter 1.") // clamp between 0 and 1
		METASOUND_PARAM(InParamDecayDiffusion_2, "Decay Diffusion 2", "Delay Value for all pass filter 2.") // clamp between 0 and 1

		// Damping 
		METASOUND_PARAM(InParamDelayDamping, "Delay Damping", ".") // clamp between 0 and 1

		// Excursion rate - 
		METASOUND_PARAM(InParamExcursionRate, "Excursion Rate", ".") // clamp between 0 - 16?
		// and depth -
		METASOUND_PARAM(InParamExcursionDepth, "Excursion Depth", ".") // clamp between 0 - 16?
			
		// Dry / Wet Values -
		METASOUND_PARAM(InParamWetValue, "Wet Value", "How strong the reverberated sound is") // clamp between 0 and 1
		METASOUND_PARAM(InParamDryValue, "Dry Value", "How strong the base sound is") // clamp between 0 and 1

		
		// -------------------- Outputs --------------------
		METASOUND_PARAM(OutParamAudio, "Out", "Audio output.")
		
	}

	// Actual Class with all functions / variables etc.
	/// 
	/// FFloatReadRef - Inputs
	/// FFloatWriteRef - Outputs
	class FReverberationOperator : public TExecutableOperator<FReverberationOperator>
	{
	public:

		// Returns metadata such as node name, type, etc
		static const FNodeClassMetadata& GetNodeInfo();
		// Returns the interface for the input and output vertex (connection points for data flow)
		static const FVertexInterface& GetVertexInterface();
		// Creates and returns a new instance of the operator, initializing it with the provided parameters.
		// Also reports any errors encountered during creation.
		static TUniquePtr<IOperator> CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors);

		// Constructor: Initialises the operator with settings and audio input data, including pitch shift and delay length.
		FReverberationOperator(const FOperatorSettings& InSettings,
			// Audio Input Buffer
			const FAudioBufferReadRef& InAudioInput,
			// Input Processing
			const FFloatReadRef& InPreDelayTime,
			const FFloatReadRef& InPreLowPassFilter,
			const FFloatReadRef& InLowPassCutoff,
			const FFloatReadRef& InParamAllPassCutOff,
			const FFloatReadRef& InInputDiffusion1,
			const FFloatReadRef& InInputDiffusion2,
			// Feedback Tail
			const FFloatReadRef& InDecayRate,
			const FFloatReadRef& InDecayDiffusion1,
			const FFloatReadRef& InDecayDiffusion2,
			const FFloatReadRef& InDecayDamping,
			const FFloatReadRef& InExcursionRate,
			const FFloatReadRef& InExcursionDepth,
			const FFloatReadRef& InWetValue,
			const FFloatReadRef& InDryValue);
			// Audio Output Buffer
			//const FFloatReadRef& InCutOff);

		// Returns the inputs for the operator (usually audio data or control parameters).
		virtual FDataReferenceCollection GetInputs() const override;
    
		// Returns the outputs of the operator (usually processed audio data).
		virtual FDataReferenceCollection GetOutputs() const override;
		
		// Low Pass Filter
		void LowPassFilter();

		// All Pass Filter
		void AllPassFilter();

		// modular functions for each segment
		void ProcessPreDelay();

		void ProcessLPF();

		void ProcessAPF();

		void InitialiseFeedbackParameters();

		void WriteDelaysAndInc(float ProcessedSample, float FeedbackSampleLeft, float FeedbackSampleRight);

		// Executes the pitch shifting operation, modifying the audio data based on the inputs.
		void Execute();
		
	private:
		float GetDelayLengthClamped() const;
		float GetPhasorPhaseIncrement() const;
		
		// -------------------- Audio Input Buffer --------------------
		
		FAudioBufferReadRef AudioInput;

		// -------------------- Input Processing --------------------

		FFloatReadRef PreDelayTime;

		FFloatReadRef PreLowPassFilter;

		FFloatReadRef LowPassCutoff;

		FFloatReadRef AllPassCutoff;
		
		FFloatReadRef InputDiffusion1;
		FFloatReadRef InputDiffusion2;

		// -------------------- Feedback Tail --------------------

		FFloatReadRef DecayRate;

		FFloatReadRef DecayDiffusion1;
		FFloatReadRef DecayDiffusion2;

		FFloatReadRef DecayDamping;

		FFloatReadRef ExcursionRate;
		FFloatReadRef ExcursionDepth;

		FFloatReadRef WetValue;
		FFloatReadRef DryValue;

		// -------------------- Audio Output Buffer --------------------
		
		FAudioBufferWriteRef AudioOutput;

		// The internal delay buffer
		Audio::FDelay DelayBuffer;

		// The sample rate of the node
		float SampleRate = 0.0f;
		
		// The delay length
		Audio::FExponentialEase CurrentDelayLength;
		
		// The previous pitch shift value
		float CurrentPreDelay = 0.0f;

		// The current phasor phase (goes between 0.0 and 1.0)
		float PhasorPhase = 0.0f;

		// The current phasor increment (a delta, plus or minus to add to the phase every frame)
		float PhasorPhaseIncrement = 0.0f;

		// The previous pitch shift value
		float CurrentPitchShift = 0.0f;
		
		// Low Pass Filter:
		Audio::FStateVariableFilter LPVariableFilter;
		//Audio::FBiquad LPVariableFilter;
		float PreviousFrequency{ -1.f };
		float PreviousResonance{ -1.f };
		float PreviousBandStopControl{ -1.f };

		// All Pass
		float PreviousAllPassFrequency{ -1.f };
		float PreviousAllPassResonance{ -1.f };

		// Delay lengths for Dattorro AllPass in samples (e.g., 142, 379, 107, 277)
		TArray<int32> DelayLengths = {142, 379, 107, 277};

		// Feedback Tail

		TArray<Audio::FDelayAPF> DattorroAllPassFilters;
		Audio::FDelayAPF DecayDiffusionFilter1Left;
		Audio::FDelayAPF DecayDiffusionFilter2Left;

		Audio::FDelayAPF DecayDiffusionFilter1Right;
		Audio::FDelayAPF DecayDiffusionFilter2Right;

		// Delay
		Audio::FDelay PostLPFFeedbackDelayLeft;
		Audio::FDelay PostLPFFeedbackDelayRight;
		Audio::FDelay FeedbackDelayLeft;
		// The delay for the left side
		Audio::FExponentialEase FeedbackDelayEaseLeft;
		Audio::FDelay FeedbackDelayRight;
		// The delay for the right side
		Audio::FExponentialEase FeedbackDelayEaseRight;		
		
		Audio::FStateVariableFilter LPDampingFilter;
		
		// Used to sum the feedback into the reverb

		int32 BufferIndex = 0;
	};

	/// Summary
	///
	/// For the constructor, I'm going to make a first-pass guess that we'll want our delay length to be 40 ms
	/// so we can initialize our delay buffer with the input sample rate and the desired 40 ms delay length. 
	///
	/// Summary
	FReverberationOperator::FReverberationOperator(const FOperatorSettings& InSettings,
		// Audio Input Buffer
		const FAudioBufferReadRef& InAudioInput,
		// Input Processing
		const FFloatReadRef& InPreDelayTime,
		const FFloatReadRef& InPreLowPassFilter,
		const FFloatReadRef& InLowPassCutoff,
		const FFloatReadRef& InParamAllPassCutOff,
		const FFloatReadRef& InInputDiffusion1,
		const FFloatReadRef& InInputDiffusion2,
		// Feedback Tail
		const FFloatReadRef& InDecayRate,
		const FFloatReadRef& InDecayDiffusion1,
		const FFloatReadRef& InDecayDiffusion2,
		const FFloatReadRef& InDecayDamping,
		const FFloatReadRef& InExcursionRate,
		const FFloatReadRef& InExcursionDepth,
		const FFloatReadRef& InWetValue,
		const FFloatReadRef& InDryValue)

		// CHANGE THIS
		: AudioInput(InAudioInput)
		, PreDelayTime(InPreDelayTime)
		, PreLowPassFilter(InPreLowPassFilter)
		, LowPassCutoff(InLowPassCutoff)
		, AllPassCutoff(InParamAllPassCutOff)
		, InputDiffusion1(InInputDiffusion1)
		, InputDiffusion2(InInputDiffusion2)
		, DecayRate(InDecayRate)
		, DecayDiffusion1(InDecayDiffusion1)
		, DecayDiffusion2(InDecayDiffusion2)
		, DecayDamping(InDecayDamping)
		, ExcursionRate(InExcursionRate)
		, ExcursionDepth(InExcursionDepth)
		, WetValue(InWetValue)
		, DryValue(InDryValue)
		, AudioOutput(FAudioBufferWriteRef::CreateNew(InSettings))
		, SampleRate(InSettings.GetSampleRate())
	{
		// Initialize the delay buffer with the initial delay length 
		CurrentDelayLength.Init(GetDelayLengthClamped());

		PhasorPhaseIncrement = GetPhasorPhaseIncrement();
		
		SampleRate = InSettings.GetSampleRate();
		DelayBuffer.Init(InSettings.GetSampleRate(), (0.001f * *PreDelayTime));
		BufferIndex = 0; // Start buffer index at 0

		LPVariableFilter.Init(SampleRate, 1);
		LPVariableFilter.SetFilterType(Audio::EFilter::LowPass);

		DattorroAllPassFilters.SetNum(DelayLengths.Num());
		for (int i = 0; i < DelayLengths.Num(); i++)
		{
			DattorroAllPassFilters[i] = Audio::FDelayAPF();
			DattorroAllPassFilters[i].Init(SampleRate);
		}

		// Set buffer to be delay size
		InitialiseFeedbackParameters();

		LPDampingFilter.Init(SampleRate, 1);
		LPDampingFilter.SetFilterType(Audio::EFilter::LowPass);
	}
	
	FDataReferenceCollection FReverberationOperator::GetInputs() const
	{
		using namespace Reverberate;

		FDataReferenceCollection InputDataReferences;
		// Audio Input Buffer
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamAudioInput), FAudioBufferReadRef(AudioInput));
		// Inputs
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamPreDelay), FFloatReadRef(PreDelayTime));
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamPreLPF), FFloatReadRef(PreLowPassFilter));
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamLowPassCutOff), FFloatReadRef(LowPassCutoff));
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamAllPassCutOff), FFloatReadRef(AllPassCutoff));
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamPreDiffuse_1), FFloatReadRef(InputDiffusion1));
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamPreDiffuse_2), FFloatReadRef(InputDiffusion2));
		// Feedback
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamDecayRate), FFloatReadRef(DecayRate));
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamDecayDiffusion_1), FFloatReadRef(DecayDiffusion1));
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamDecayDiffusion_2), FFloatReadRef(DecayDiffusion2));
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamDelayDamping), FFloatReadRef(DecayDamping));
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamExcursionRate), FFloatReadRef(ExcursionRate));
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamExcursionDepth), FFloatReadRef(ExcursionDepth));
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamWetValue), FFloatReadRef(WetValue));
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamDryValue), FFloatReadRef(DryValue));

		return InputDataReferences;
	}

	FDataReferenceCollection FReverberationOperator::GetOutputs() const
	{
		using namespace Reverberate;

		FDataReferenceCollection OutputDataReferences;
		OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(OutParamAudio), FAudioBufferReadRef(AudioOutput));
		return OutputDataReferences;
	}

	// The getter utility functions simply utilize the constants defined in the parameter section earlier.
	// 
	// Setup getters for getting clamped values for each input.
	
	float FReverberationOperator::GetDelayLengthClamped() const
	{
		return FMath::Clamp(*PreDelayTime, 10, 100);		
	}

	float FReverberationOperator::GetPhasorPhaseIncrement() const
	{
		const float PhasorFrequency = 1.0f / (CurrentDelayLength.PeekCurrentValue()); // Phasor frequency is based on delay length
		return PhasorFrequency / SampleRate; // Normalize by sample rate

	}


	void FReverberationOperator::LowPassFilter()
	{
		//Code below taken from MetasoundBasicFilters.cpp
		const float CurrentFrequency = FMath::Clamp(*LowPassCutoff, 0.f, (0.5f * SampleRate));
		const float CurrentBandStopControl = FMath::Clamp(0.f, 0.f, 1.f);

		if (bool bNeedsUpdate = (!FMath::IsNearlyEqual(PreviousFrequency, CurrentFrequency))
			|| (!FMath::IsNearlyEqual(PreviousBandStopControl, CurrentBandStopControl)))
		{
			LPVariableFilter.SetQ(*PreLowPassFilter);
			LPVariableFilter.SetFrequency(*LowPassCutoff);
			LPVariableFilter.SetBandStopControl(CurrentBandStopControl);
			
			LPVariableFilter.Update();

			LPDampingFilter.SetFrequency(1 - *DecayDamping);
			LPDampingFilter.SetQ(1);
			LPVariableFilter.SetBandStopControl(CurrentBandStopControl);

			LPDampingFilter.Update();

			PreviousFrequency = CurrentFrequency;
			PreviousBandStopControl = CurrentBandStopControl;
		}
	}

	void FReverberationOperator::AllPassFilter()
	{
		const float CurrentFrequency = FMath::Clamp(*AllPassCutoff, 0.f, (0.5f * SampleRate));
		const float CurrentResonance = FMath::Clamp(*InputDiffusion1, 0.f, 10.f);

		if (bool bNeedsUpdate = (!FMath::IsNearlyEqual(PreviousFrequency, CurrentFrequency)
					|| !FMath::IsNearlyEqual(PreviousResonance, CurrentResonance)))
		{
			for (int i = 0; i < DelayLengths.Num(); i++)
			{
				DattorroAllPassFilters[i].SetG(i < 2 ? *InputDiffusion1 : *InputDiffusion2);
				DattorroAllPassFilters[i].SetDelaySamples(DelayLengths[i]);
			}

			
			PreviousAllPassFrequency = CurrentFrequency;
			PreviousAllPassResonance = CurrentResonance;
		}
	}

	void FReverberationOperator::InitialiseFeedbackParameters()
	{
		FeedbackDelayLeft.Init(SampleRate, 0.001f * 4453);
		FeedbackDelayEaseLeft.Init(4453);
		FeedbackDelayRight.Init(SampleRate, 0.001f * 4217);
		FeedbackDelayEaseRight.Init(4217);

		
		// set delay sample rate for delay nodes
		PostLPFFeedbackDelayLeft.Init(SampleRate);
		PostLPFFeedbackDelayLeft.SetDelaySamples(3720);
		PostLPFFeedbackDelayRight.Init(SampleRate);
		PostLPFFeedbackDelayRight.SetDelaySamples(3163);
		
		FeedbackDelayLeft.SetDelaySamples(4453); // Feedback delay time
		FeedbackDelayEaseLeft.SetValue(4453);
			
		FeedbackDelayRight.SetDelaySamples(4217); // Feedback delay time
		FeedbackDelayEaseRight.SetValue(4217);
			
	
		
		// Set delay for All Pass Filters in Feedback tail

		DecayDiffusionFilter1Left.Init(SampleRate, 0.001f * *PreDelayTime);
		DecayDiffusionFilter1Left.SetG(*DecayDiffusion1);
		DecayDiffusionFilter1Left.SetDelaySamples(672 + *ExcursionRate);

		DecayDiffusionFilter1Right.Init(SampleRate, 0.001f * *PreDelayTime);
		DecayDiffusionFilter1Right.SetG(*DecayDiffusion1);
		DecayDiffusionFilter1Right.SetDelaySamples(908 + *ExcursionRate);

		DecayDiffusionFilter2Left.Init(SampleRate, 0.001f * *PreDelayTime);
		DecayDiffusionFilter2Left.SetG(*DecayDiffusion2);
		DecayDiffusionFilter2Left.SetDelaySamples(1800);

		DecayDiffusionFilter2Right.Init(SampleRate, 0.001f * *PreDelayTime);
		DecayDiffusionFilter2Right.SetG(*DecayDiffusion2);
		DecayDiffusionFilter2Right.SetDelaySamples(2656);
	}

	void FReverberationOperator::WriteDelaysAndInc(float ProcessedSample, float FeedbackSampleLeft, float FeedbackSampleRight)
	{
		// Write the new sample to the delay buffer (for future delay reads)
		DelayBuffer.WriteDelayAndInc((ProcessedSample));

		for (int i = 0; i < DelayLengths.Num(); i++)
		{
			DattorroAllPassFilters[i].WriteDelayAndInc(ProcessedSample);
		}
			
		FeedbackDelayLeft.WriteDelayAndInc(ProcessedSample);
		FeedbackDelayRight.WriteDelayAndInc(ProcessedSample);

		DecayDiffusionFilter1Left.WriteDelayAndInc(FeedbackSampleRight);
		DecayDiffusionFilter2Left.WriteDelayAndInc(FeedbackSampleRight);
		DecayDiffusionFilter1Right.WriteDelayAndInc(FeedbackSampleLeft);
		DecayDiffusionFilter2Right.WriteDelayAndInc(FeedbackSampleLeft);
	}

	void FReverberationOperator::Execute()
	{
		// Process the unmodified signal through the all-pass filter.
		const float* InputAudio = AudioInput->GetData();
		float* OutputAudio = AudioOutput->GetData();
		const int32 NumFrames = AudioInput->Num();
 
		const float DelayLength = *PreDelayTime;
		
		if (bool bDelayCheck = (!FMath::IsNearlyEqual(DelayLength, CurrentDelayLength.GetNextValue())))
		{
			UE_LOG(LogTemp, Log, TEXT("Set Value to PreDelayTime: %.3f"), DelayLength);
			CurrentDelayLength.SetValue(DelayLength);
		}
		
		// apply Low Pass
		LowPassFilter();

		// Array for low pass output.
		TArray<float> LowPassAudio;
		LowPassAudio.SetNumUninitialized(NumFrames);
		
		// Store low pass filter result.
		LPVariableFilter.ProcessAudio(AudioInput->GetData(), NumFrames, LowPassAudio.GetData());

		// Setup All Pass
		AllPassFilter();
		
		const float NewDelayLengthClamped = GetDelayLengthClamped();
		bool bRecomputePhasorIncrement = (!FMath::IsNearlyEqual(NewDelayLengthClamped, CurrentDelayLength.GetNextValue()));
		if (bRecomputePhasorIncrement)
		{
			UE_LOG(LogTemp, Log, TEXT("Set Value to PreDelayTime: %.3f"), CurrentDelayLength.PeekCurrentValue());
			PhasorPhaseIncrement = GetPhasorPhaseIncrement(); 
		}
		
		// mix original and low pass
		for (int32 FrameIndex = 0; FrameIndex < NumFrames; FrameIndex++)
		{
			// Update the interpolated delay length value
			if (!CurrentDelayLength.IsDone())
			{
				CurrentDelayLength.GetNextValue();
			}
			if(!FeedbackDelayEaseLeft.IsDone())
			{
				FeedbackDelayEaseLeft.GetNextValue();
			}
			if (!FeedbackDelayEaseRight.IsDone())
			{
				FeedbackDelayEaseRight.GetNextValue();
			}

			UE_LOG(LogTemp, Log, TEXT("DelayBuffer: %.1f"), CurrentDelayLength.PeekCurrentValue());

			const float PhasorReadPosition = CurrentDelayLength.PeekCurrentValue(); // Get the current phase position from the phasor
			//UE_LOG(LogTemp, Log, TEXT("DelayBuffer: %.6f"), PhasorReadPosition);
			const float DelayTapRead1 = FMath::Max(PhasorReadPosition, 0.0f);						 // Phasor controls the first delay tap
			const float DelayTapRead2 = PhasorReadPosition + PhasorPhaseIncrement;						 // The second tap, offset by the phasor increment
			UE_LOG(LogTemp, Log, TEXT("Increment: %.1f"), PhasorPhaseIncrement);
			
			// Read the delay lines at the given tap indices, apply the gains
			const float Sample1 = DelayBuffer.ReadDelayAt(DelayTapRead1);
			// if Delay tap 2 less than 0, add sample size
			const float Sample2 = DelayBuffer.ReadDelayAt(DelayTapRead2);
			//const float Sample2 = DattorroAllPassFilters[0].ReadDelayAt(DelayTapRead2);
			
			// The delayed signal, mixing the two delay taps
			const float DelayedSample = Sample1 + Sample2;
			
			// Mix the original (InputAudio) with the all-pass processed audio
			const float OriginalSample = InputAudio[FrameIndex];
			// Process each audio sample through the all-pass filter
			float ProcessedSample = LowPassAudio[FrameIndex];
			for (int i = 0; i < DelayLengths.Num(); i++)
			{
				ProcessedSample = DattorroAllPassFilters[i].ProcessAudioSample(ProcessedSample);
			}

			// Feedback Tail Code
			
			// Get the FeedbackReadPosition from the ease function
			float FeedbackReadPosition = FeedbackDelayEaseLeft.PeekCurrentValue();

			//UE_LOG(LogTemp, Log, TEXT("FeedbackSampleLeft1: %.1f"), FeedbackReadPosition);
			
			float FeedbackSampleLeft = FeedbackDelayLeft.ReadDelayAt(FeedbackReadPosition);

			//UE_LOG(LogTemp, Log, TEXT("FeedbackSampleLeft2: %.1f"), FeedbackSampleLeft);
			
			//FeedbackSampleLeft = DecayDiffusionFilter1Left.ProcessAudioSample(FeedbackSampleLeft);
			//FeedbackSampleLeft = FeedbackDelayLeft.ProcessAudioSample(FeedbackSampleLeft);

			FeedbackSampleLeft *= *DecayRate; // DecayRate is clamped between 0 and 1
			
			// Low-pass filter (damping)
			//float LowPassFeedbackSampleLeft = 0.0f;
			//LPDampingFilter.ProcessAudioFrame(&FeedbackSampleLeft, &LowPassFeedbackSampleLeft);
			
			//FeedbackSampleLeft = DecayDiffusionFilter2Left.ProcessAudioSample(LowPassFeedbackSampleLeft);

			//FeedbackSampleLeft = PostLPFFeedbackDelayLeft.ProcessAudioSample(FeedbackSampleLeft);

			// Right side of feedback tail
			// Get the FeedbackReadPosition from the ease function
			FeedbackReadPosition = FeedbackDelayEaseRight.PeekCurrentValue();
			
			float FeedbackSampleRight = FeedbackDelayRight.ReadDelayAt(FMath::Max(FeedbackReadPosition, 0.0f));

			//UE_LOG(LogTemp, Log, TEXT("FeedbackSampleRight: %.6f"), FeedbackSampleRight);
			
			//FeedbackSampleRight = DecayDiffusionFilter1Right.ProcessAudioSample(FeedbackSampleRight);
			//FeedbackSampleRight = FeedbackDelayRight.ProcessAudioSample(FeedbackSampleRight);

			FeedbackSampleRight *= *DecayRate; // DecayRate is clamped between 0 and 1
			
			// Low-pass filter (damping)
			//float LowPassFeedbackSampleRight = 0.0f;
			//LPDampingFilter.ProcessAudioFrame(&FeedbackSampleRight, &LowPassFeedbackSampleRight);
			
			//FeedbackSampleRight = DecayDiffusionFilter2Right.ProcessAudioSample(LowPassFeedbackSampleRight);

			//FeedbackSampleRight = PostLPFFeedbackDelayRight.ProcessAudioSample(FeedbackSampleRight);
			
			// Process Outputs

			const float MixedSample = (OriginalSample * *DryValue) + (DelayedSample * *WetValue) + (FeedbackSampleLeft * *WetValue) + (FeedbackSampleRight * *WetValue);
			//const float MixedSample = (OriginalSample * *DryValue) + (FeedbackSampleLeft * *WetValue) + (FeedbackSampleRight * *WetValue);
			OutputAudio[FrameIndex] = MixedSample;

			WriteDelaysAndInc(ProcessedSample, FeedbackSampleLeft, FeedbackSampleRight);
		}
	}

	/// Summary
	///
	///The vertex interface is basically the pin inputs and outputs.
	///This defines the actual C++ types to use for the pin and their default values (i.e. literal values that show up if the pin doesn't have a connection).
	///Not all types have literal values, but this is where you can suggest reasonable default values to the user.
	///In our case, the default pitch shift is reasonably 0.0 semitones. 
	///
	/// Summary
	const FVertexInterface& FReverberationOperator::GetVertexInterface()
	{
		using namespace Reverberate;

		static const FVertexInterface Interface(
			FInputVertexInterface(
				TInputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamAudioInput)),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamPreDelay), 0.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamPreLPF), 0.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamLowPassCutOff), 0.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamAllPassCutOff), 0.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamPreDiffuse_1), 0.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamPreDiffuse_2), 0.0f),

				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamDecayRate), 0.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamDecayDiffusion_1), 0.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamDecayDiffusion_2), 0.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamDelayDamping), 0.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamExcursionRate), 0.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamExcursionDepth), 0.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamWetValue), 0.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamDryValue), 0.0f)
			),
			FOutputVertexInterface(
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutParamAudio))
			)
		);

		return Interface;
	}

	
	const FNodeClassMetadata& FReverberationOperator::GetNodeInfo()
	{
		auto InitNodeInfo = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Info;
			Info.ClassName = { StandardNodes::Namespace, "Reverberation", StandardNodes::AudioVariant };
			Info.MajorVersion = 1;
			Info.MinorVersion = 1;
			Info.DisplayName = METASOUND_LOCTEXT("ReverbNode_DisplayName", "Dattorro Reverberation");
			Info.Description = METASOUND_LOCTEXT("ReverbNode_Description", "Reverberates the Audio Input.");
			Info.Author = PluginAuthor;
			Info.PromptIfMissing = PluginNodeMissingPrompt;
			Info.DefaultInterface = GetVertexInterface();
			Info.CategoryHierarchy.Emplace(NodeCategories::Functions);
			return Info;
		};

		static const FNodeClassMetadata Info = InitNodeInfo();

		return Info;
	}

	/// Summary
	///
	/// So the thing that does the thing in MetaSounds is an IOperator.
	/// It essentially has the Execute() function called on it and hooks up all the inputs and outputs for you.
	/// The CreateOperator function in the node API is essentially a factory function -- a function that makes new things.
	/// Static function called when a new node is created by metasound builder - passing in relevant information.
	///
	/// Here is where you retrieve your input references and pass them to your object and also allocate your write references (that your object owns).
	///
	/// Summary
	TUniquePtr<IOperator> FReverberationOperator::CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors)
	{
		using namespace Reverberate;

		const FDataReferenceCollection& InputCollection = InParams.InputDataReferences;
		const FInputVertexInterface& InputInterface = GetVertexInterface().GetInputInterface();

		FAudioBufferReadRef AudioIn = InputCollection.GetDataReadReferenceOrConstruct<FAudioBuffer>(METASOUND_GET_PARAM_NAME(InParamAudioInput), InParams.OperatorSettings);
		FFloatReadRef PreDelayTime = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamPreDelay), InParams.OperatorSettings);
		FFloatReadRef PreLowPassFilter = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamPreLPF), InParams.OperatorSettings);
		FFloatReadRef LowPassCutoff = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamLowPassCutOff), InParams.OperatorSettings);
		FFloatReadRef AllPassCutoff = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamAllPassCutOff), InParams.OperatorSettings);
		FFloatReadRef InputDiffusion1 = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamPreDiffuse_1), InParams.OperatorSettings);
		FFloatReadRef InputDiffusion2 = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamPreDiffuse_2), InParams.OperatorSettings);
		
		FFloatReadRef DecayRate = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamDecayRate), InParams.OperatorSettings);
		FFloatReadRef DecayDiffusion1 = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamDecayDiffusion_1), InParams.OperatorSettings);
		FFloatReadRef DecayDiffusion2 = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamDecayDiffusion_2), InParams.OperatorSettings);
		FFloatReadRef DelayDamping = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamDelayDamping), InParams.OperatorSettings);

		FFloatReadRef ExcursionRate = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamExcursionRate), InParams.OperatorSettings);
		FFloatReadRef ExcursionDepth = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamExcursionDepth), InParams.OperatorSettings);

		FFloatReadRef WetValue = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamWetValue), InParams.OperatorSettings);
		FFloatReadRef DryValue = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamDryValue), InParams.OperatorSettings);

		return MakeUnique<FReverberationOperator>(InParams.OperatorSettings, AudioIn, PreDelayTime, PreLowPassFilter, LowPassCutoff, AllPassCutoff, InputDiffusion1, InputDiffusion2, DecayRate, DecayDiffusion1, DecayDiffusion2, DelayDamping, ExcursionRate, ExcursionDepth, WetValue, DryValue);
	}

	class FReverbNode : public FNodeFacade
	{
	public:
		/**
		 * Constructor used by the Metasound Frontend.
		 */
		FReverbNode(const FNodeInitData& InitData)
			: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FReverberationOperator>())
		{
		}
	};


	METASOUND_REGISTER_NODE(FReverbNode)
}

#undef LOCTEXT_NAMESPACE
