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
		METASOUND_PARAM(InParamAudioInput, "In", "Incoming Audio Signal")
		//PreDelay - 
		METASOUND_PARAM(InParamPreDelay, "PreDelayTime", "Delay time before the reverb begins playing") // Clamped between Min and Max PreDelay
		// Pre Low Pass Filter
		METASOUND_PARAM(InParamPreLPF, "Pre Low Pass Filter Bandwidth", "Controls intensity of pre low pass filter - attenuates higher frequencies.") // Clamp Between 0 and 1.
		METASOUND_PARAM(InParamLowPassCutOff, "Low Pass CutOff", "Cut off frequency for low pass filter - controls the cutoff for frequencies in the sound") // Clamp Between 0 and 1.
		// Pre Diffusion - All Pass
		METASOUND_PARAM(InParamAllPassCutOff, "All Pass Cutoff", "Defines the cutoff frequency for all pass filter to create a smoother signal")
		METASOUND_PARAM(InParamPreDiffuse_1, "Input Diffusion 1", "Sets diffuse coefficient for the first all pass filter pair") // Clamp Between 0 and 1.
		METASOUND_PARAM(InParamPreDiffuse_2, "Input Diffusion 2", "Sets diffuse coefficient for the second all pass filter pair") // Clamp Between 0 and 1.
			
		// -------------------- Feedback Tail --------------------
		// Decay Rate
		METASOUND_PARAM(InParamDecayRate, "Decay Rate", "Adjusts how quickly the delay fades out") // clamp between 0 and 1

		// Delay 1 Delay Time
		METASOUND_PARAM(InParamFeedbackDelay_1, "Feedback Delay Left", "Sets delay time for the left feedback loop") // clamp between 0 and 1
		
		// Decay Diffusion
		METASOUND_PARAM(InParamDecayDiffusion_1, "Decay Diffusion 1", "Adjusts value for the first all pass filter in the reverb tail") // clamp between 0 and 1
		METASOUND_PARAM(InParamDecayDiffusion_2, "Decay Diffusion 2", "Adjusts value for the second all pass filter in the reverb tail") // clamp between 0 and 1

		// Damping 
		METASOUND_PARAM(InParamDelayDamping, "Delay Damping", "Controls the amount of damping applied to the delay signal.") // clamp between 0 and 1

		// Delay 2 Delay Time
		METASOUND_PARAM(InParamFeedbackDelay_2, "Feedback Delay Right", "Sets delay time for the right feedback loop") // clamp between 0 and 1
		
		// Delay rate - 
		METASOUND_PARAM(InParamRandomDelay, "Random Delay", "adjusts delay rate for specific filters") // clamp between 0 - 16?

		METASOUND_PARAM(InParamFinalDelay_1, "Final Delay Left", "Sets delay time for the final left feedback delay") // clamp between 0 and 1
		METASOUND_PARAM(InParamFinalDelay_2, "Final Delay Right", "Sets delay time for the final right feedback delay") // clamp between 0 and 1
		
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
			const FFloatReadRef& InParamFeedbackDelay_1,
			const FFloatReadRef& InDecayDiffusion1,
			const FFloatReadRef& InDecayDiffusion2,
			const FFloatReadRef& InDecayDamping,
			const FFloatReadRef& InParamRandomDelay,
			const FFloatReadRef& InParamFeedbackDelay_2,
			const FFloatReadRef& InParamFinalDelay_1,
			const FFloatReadRef& InParamFinalDelay_2,
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

		void InitialiseFeedbackParameters();

		void WriteDelaysAndInc(float ProcessedSample, float FeedbackSampleLeft, float FeedbackSampleRight, float FinalLeft, float FinalRight);

		// Executes the Reverberation operation
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

		FFloatReadRef InFeedbackDelay1;

		FFloatReadRef DecayDiffusion1;
		FFloatReadRef DecayDiffusion2;

		FFloatReadRef DecayDamping;

		FFloatReadRef RandomDelay;

		FFloatReadRef InFeedbackDelay2;

		FFloatReadRef InFinalDelayLeft;
		FFloatReadRef InFinalDelayRight;

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

		// Variable for calculation 1 - damping value
		float DampingMultiplicationValue;
		
		Audio::FStateVariableFilter LPDampingFilter;

		// These two values feed back into the previous 
		
		// Used to sum the feedback into the reverb
		float FeedbackLeft = NULL;
		float FeedbackRight = NULL;
		
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
		const FFloatReadRef& InParamFeedbackDelay_1,
		const FFloatReadRef& InDecayDiffusion1,
		const FFloatReadRef& InDecayDiffusion2,
		const FFloatReadRef& InDecayDamping,
		const FFloatReadRef& InParamRandomDelay,
		const FFloatReadRef& InParamFeedbackDelay_2,
		const FFloatReadRef& InParamFinalDelay_1,
		const FFloatReadRef& InParamFinalDelay_2,
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
		, InFeedbackDelay1(InParamFeedbackDelay_1)
		, DecayDiffusion1(InDecayDiffusion1)
		, DecayDiffusion2(InDecayDiffusion2)
		, DecayDamping(InDecayDamping)
		, RandomDelay(InParamRandomDelay)
		, InFeedbackDelay2(InParamFeedbackDelay_2)
		, InFinalDelayLeft(InParamFinalDelay_1)
		, InFinalDelayRight(InParamFinalDelay_2)
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
		DelayBuffer.SetDelaySamples(*PreDelayTime);
		BufferIndex = 0; // Start buffer index at 0

		LPVariableFilter.Init(SampleRate, 1);
		LPVariableFilter.SetFilterType(Audio::EFilter::LowPass);

		// Looping through the all pass filters to initialise and push into TArray
		DattorroAllPassFilters.SetNum(DelayLengths.Num());
		for (int i = 0; i < DelayLengths.Num(); i++)
		{
			DattorroAllPassFilters[i] = Audio::FDelayAPF();
			DattorroAllPassFilters[i].Init(SampleRate);
		}

		// Initialises each delay and filter
		InitialiseFeedbackParameters();

		// set value for damping multiplication (1 - damping).
		DampingMultiplicationValue = (1 - *DecayDamping);
		
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
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamFeedbackDelay_1), FFloatReadRef(InFeedbackDelay1));
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamDecayDiffusion_1), FFloatReadRef(DecayDiffusion1));
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamDecayDiffusion_2), FFloatReadRef(DecayDiffusion2));
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamDelayDamping), FFloatReadRef(DecayDamping));
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamRandomDelay), FFloatReadRef(RandomDelay));
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamFeedbackDelay_2), FFloatReadRef(InFeedbackDelay2));
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamFinalDelay_1), FFloatReadRef(InFinalDelayLeft));
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamFinalDelay_2), FFloatReadRef(InFinalDelayRight));
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
		const float DelayLength = CurrentDelayLength.PeekCurrentValue();
		const float PhasorFrequency = 1.0f / DelayLength;
		// Normalise by sample rate
		const float PhaseIncrement = PhasorFrequency / SampleRate;         
    
		//UE_LOG(LogTemp, Log, TEXT("PhasorIncrement: %.6f"), PhaseIncrement);
		return PhaseIncrement;
	}


	void FReverberationOperator::LowPassFilter()
	{
		// Some Code below taken from MetasoundBasicFilters.cpp
		const float CurrentFrequency = FMath::Clamp(*LowPassCutoff, 0.f, (0.5f * SampleRate));
		const float CurrentBandStopControl = FMath::Clamp(0.f, 0.f, 1.f);

		// this if statement maybe not needed anymore
		if (bool bNeedsUpdate = (!FMath::IsNearlyEqual(PreviousFrequency, CurrentFrequency))
			|| (!FMath::IsNearlyEqual(PreviousBandStopControl, CurrentBandStopControl)))
		{
			// 1 - bandwidth
			LPVariableFilter.SetQ(1 - *PreLowPassFilter);
			LPVariableFilter.SetFrequency(*LowPassCutoff);
			LPVariableFilter.SetBandStopControl(CurrentBandStopControl);
			
			LPVariableFilter.Update();

			LPDampingFilter.SetFrequency(*LowPassCutoff / 2);
			LPDampingFilter.SetQ(*DecayDamping);
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

		// this if statement maybe not needed anymore
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
		// Delay sample times from metasound node
		const float LeftSampleDelay = (*InFeedbackDelay1);
		const float RightSampleDelay = (*InFeedbackDelay2);
		
		// make custom delay variables?
		// Left Feedback Delay
		FeedbackDelayLeft.Init(SampleRate, 0.001f * LeftSampleDelay);
		FeedbackDelayLeft.SetDelaySamples(LeftSampleDelay); // Feedback delay time in samples
		FeedbackDelayEaseLeft.Init(LeftSampleDelay);
		FeedbackDelayEaseLeft.SetValue(LeftSampleDelay);
		// Right Feedback Delay
		FeedbackDelayRight.Init(SampleRate, 0.001f * RightSampleDelay);
		FeedbackDelayRight.SetDelaySamples(RightSampleDelay); // Feedback delay time in samples
		FeedbackDelayEaseRight.Init(RightSampleDelay);
		FeedbackDelayEaseRight.SetValue(RightSampleDelay);
		
		// set delay sample rate for delay nodes
		const float LeftSampleFinalDelay = *InFinalDelayLeft;
		const float RightSampleFinalDelay = *InFinalDelayRight;
		
		PostLPFFeedbackDelayLeft.Init(SampleRate);
		PostLPFFeedbackDelayLeft.SetDelaySamples(LeftSampleFinalDelay); // second feedback delay time in samples
		PostLPFFeedbackDelayRight.Init(SampleRate);
		PostLPFFeedbackDelayRight.SetDelaySamples(RightSampleFinalDelay); // second feedback delay time in samples
		
		
		// Set delay for All Pass Filters in Feedback tail
		// RandomDelay introduces slight differences in the delay sample amount
		DecayDiffusionFilter1Left.Init(SampleRate, 0.001f * *PreDelayTime);
		DecayDiffusionFilter1Left.SetG(*DecayDiffusion1);
		const int32 DelayRate = *RandomDelay;
		float RandomDelayValue = FMath::RandRange(0, DelayRate);
		DecayDiffusionFilter1Left.SetDelaySamples(250 + RandomDelayValue);

		DecayDiffusionFilter1Right.Init(SampleRate, 0.001f * *PreDelayTime);
		DecayDiffusionFilter1Right.SetG(*DecayDiffusion1);
		RandomDelayValue = FMath::RandRange(0, DelayRate);
		DecayDiffusionFilter1Right.SetDelaySamples(440 + RandomDelayValue);

		DecayDiffusionFilter2Left.Init(SampleRate, 0.001f * *PreDelayTime);
		DecayDiffusionFilter2Left.SetG(*DecayDiffusion1);
		DecayDiffusionFilter2Left.SetDelaySamples(770);

		DecayDiffusionFilter2Right.Init(SampleRate, 0.001f * *PreDelayTime);
		DecayDiffusionFilter2Right.SetG(*DecayDiffusion2);
		DecayDiffusionFilter2Right.SetDelaySamples(960);
	}

	void FReverberationOperator::WriteDelaysAndInc(float ProcessedSample, float FirstProcessedFeedbackSampleLeft, float FirstProcessedFeedbackSampleRight, float FinalDelayPassLeft, float FinalDelayPassRight)
	{
		//UE_LOG(LogTemp, Log, TEXT("ProcessedSample: %.2f, FeedbackLeft: %.2f, FeedbackRight: %.2f"), ProcessedSample, FeedbackSampleLeft, FeedbackSampleRight);
		
		// Write the new sample to the delay buffer (for future delay reads)
		DelayBuffer.WriteDelayAndInc(ProcessedSample);

		for (int i = 0; i < DelayLengths.Num(); i++)
		{
			DattorroAllPassFilters[i].WriteDelayAndInc(ProcessedSample);
		}

		// Write each specific sample to each specific delay.
		FeedbackDelayLeft.WriteDelayAndInc(FirstProcessedFeedbackSampleLeft);
		FeedbackDelayRight.WriteDelayAndInc(FirstProcessedFeedbackSampleRight);

		DecayDiffusionFilter1Left.WriteDelayAndInc(FirstProcessedFeedbackSampleLeft);
		DecayDiffusionFilter2Left.WriteDelayAndInc(FirstProcessedFeedbackSampleLeft);
		DecayDiffusionFilter1Right.WriteDelayAndInc(FirstProcessedFeedbackSampleRight);
		DecayDiffusionFilter2Right.WriteDelayAndInc(FirstProcessedFeedbackSampleRight);

		// write to final delay
		PostLPFFeedbackDelayLeft.WriteDelayAndInc(FinalDelayPassLeft);
		PostLPFFeedbackDelayRight.WriteDelayAndInc(FinalDelayPassRight);

		// Write to feedback variables - these will be summed with the processed sample in the feedback loop
		FeedbackLeft = FinalDelayPassRight;
		FeedbackRight = FinalDelayPassLeft;
	}

	void FReverberationOperator::Execute()
	{
		// assign input and output audio to variables at the start.
		const float* InputAudio = AudioInput->GetData();
		float* OutputAudio = AudioOutput->GetData();
		// NumFrames used for looping over each sample.
		const int32 NumFrames = AudioInput->Num();

		DampingMultiplicationValue = (1 - *DecayDamping);
		const float DecayRateVariable = *DecayRate;
		const float DelayLength = *PreDelayTime;
		
		if (bool bDelayCheck = (!FMath::IsNearlyEqual(DelayLength, CurrentDelayLength.GetNextValue())))
		{
			//UE_LOG(LogTemp, Log, TEXT("Set Value to PreDelayTime: %.3f"), DelayLength);
			CurrentDelayLength.SetValue(DelayLength);
		}
		
		// apply Low Pass
		LowPassFilter();

		// Array for low pass output.
		TArray<float> LowPassAudio;
		LowPassAudio.SetNumUninitialized(NumFrames);

		TArray<float> ScaledAudio;
		ScaledAudio.SetNumUninitialized(NumFrames);
		
		// Store low pass filter result.
		for (int32 FrameIndex = 0; FrameIndex < NumFrames; FrameIndex++)
		{
			// multiply by bandwidth value
			ScaledAudio[FrameIndex] = InputAudio[FrameIndex] * *PreLowPassFilter;
		}
		LPVariableFilter.ProcessAudio(ScaledAudio.GetData(), NumFrames, LowPassAudio.GetData());

		// Setup All Pass
		AllPassFilter();

		// used to change the phase increment on pitch shift - not used fully.
		const float NewDelayLengthClamped = GetDelayLengthClamped();
		bool bRecomputePhasorIncrement = (!FMath::IsNearlyEqual(NewDelayLengthClamped, CurrentDelayLength.GetNextValue()));
		if (bRecomputePhasorIncrement)
		{
			//UE_LOG(LogTemp, Log, TEXT("Set Value to PreDelayTime: %.3f"), CurrentDelayLength.PeekCurrentValue());
			PhasorPhaseIncrement = GetPhasorPhaseIncrement(); 
		}
		
		// mix original and low pass
		for (int32 FrameCount = 0; FrameCount < NumFrames; FrameCount++)
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

			// Process each audio sample through the all-pass filters
			float AllPassProcessedSample = 0.0f;
			float ProcessedSample = LowPassAudio[FrameCount];
			// Loop through all four all pass filters and add each together.
			for (int i = 0; i < DelayLengths.Num(); i++)
			{
				 AllPassProcessedSample += DattorroAllPassFilters[i].ProcessAudioSample(ProcessedSample);
			}

			// assign all pass samples to processed sample variable to continue.
			ProcessedSample = AllPassProcessedSample;
			
			//UE_LOG(LogTemp, Log, TEXT("DelayBuffer: %.1f"), CurrentDelayLength.PeekCurrentValue());

			const float PhasorReadPosition = CurrentDelayLength.PeekCurrentValue(); // Get the current phase position from the phasor
			//UE_LOG(LogTemp, Log, TEXT("DelayBuffer: %.6f"), PhasorReadPosition);
			const float DelayTapRead1 = FMath::Max(PhasorReadPosition, 0.0f);						 // Phasor controls the first delay tap
			// Add 100 sample tap on delay sample.
			const float DelayTapRead2 = FMath::Fmod(FMath::Max(PhasorReadPosition + 100, 0.0f), DelayBuffer.GetDelayLengthSamples());						 // The second tap, offset by the phasor increment
			//UE_LOG(LogTemp, Log, TEXT("Buffer Length: %.1f"), DelayBuffer.GetDelayLengthSamples());
			
			// Read the delay lines at the given tap locations, these will be summed together later.
			const float Sample1 = DelayBuffer.ReadDelayAt(DelayTapRead1);
			// if Delay tap 2 less than 0, add sample size
			const float Sample2 = DelayBuffer.ReadDelayAt(DelayTapRead2);

			// ------------------------------- Feedback Tail Code -------------------------------
			// ------------------------------- Left Side of the Feedback Tail ------------------------------
			
			float FirstProcessedFeedbackSampleLeft = ProcessedSample;
			if(FeedbackLeft != NULL) // If there is no value in the feedback loop then skip summing
			{
				FirstProcessedFeedbackSampleLeft = (ProcessedSample + FeedbackLeft);
			}

			// ------------------------------- Left - All Pass Filter - Diffuse 1 -------------------------------
			
			FirstProcessedFeedbackSampleLeft = DecayDiffusionFilter1Left.ProcessAudioSample(FirstProcessedFeedbackSampleLeft);

			// ------------------------------- Left - First Delay -------------------------------

			// Will write the FirstProcessedFeedbackSampleLeft into the delay buffer at the end of Execute().
			
			// Get the FeedbackReadPosition from the ease function
			float FeedbackReadPosition = FeedbackDelayEaseLeft.PeekCurrentValue();
			
			//UE_LOG(LogTemp, Log, TEXT("ReadPos: %.2f"), FeedbackReadPosition);
			
			const float FeedbackSampleLeft = FeedbackDelayLeft.ReadDelayAt(FMath::Fmod(FMath::Max(FeedbackReadPosition, 0.0f), FeedbackDelayLeft.GetDelayLengthSamples()));	
			
			// ----------------------------- Left - Low-pass filter - Damping -----------------------------
			
			float FinalProcessedFeedbackSampleLeft = FirstProcessedFeedbackSampleLeft;

			// multiply by damping before processing through low pass.
			FinalProcessedFeedbackSampleLeft *= DampingMultiplicationValue;
			
			float LowPassFeedbackSampleLeft = 0.0f;
			
			LPDampingFilter.ProcessAudioFrame(&FinalProcessedFeedbackSampleLeft, &LowPassFeedbackSampleLeft);
			

			// ------------------------------- Left - Decay Sound -------------------------------
			
			FinalProcessedFeedbackSampleLeft *= DecayRateVariable;

			// ------------------------------- Left - All Pass Filter - Diffuse 2 -------------------------------
			
			FinalProcessedFeedbackSampleLeft = DecayDiffusionFilter2Left.ProcessAudioSample(LowPassFeedbackSampleLeft);

			// ------------------------------- Left - Final Delay -------------------------------
			
			const float LeftSampleFinalDelay = FMath::Clamp(*InFinalDelayLeft, 0, 2000);
			
			const float FinalFeedbackSampleLeft = PostLPFFeedbackDelayLeft.ReadDelayAt(LeftSampleFinalDelay);

			// ------------------------------- Left - Decay Sound  -------------------------------
			
			FinalProcessedFeedbackSampleLeft *= DecayRateVariable;

			// ------------------------------- Right side of Feedback Tail -------------------------------

			float FirstProcessedFeedbackSampleRight = ProcessedSample;
			if(FeedbackRight != NULL) // If there is no value in the feedback loop then skip summing
			{
				FirstProcessedFeedbackSampleRight = (ProcessedSample + FeedbackRight);
			}

			// ------------------------------- Right - All Pass Filter - Diffuse 1 -------------------------------
			
			FirstProcessedFeedbackSampleRight = DecayDiffusionFilter1Right.ProcessAudioSample(FirstProcessedFeedbackSampleRight);

			// ------------------------------- Right - First Delay -------------------------------

			// Get the FeedbackReadPosition from the ease function
			FeedbackReadPosition = FeedbackDelayEaseRight.PeekCurrentValue();

			const float FeedbackSampleRight = FeedbackDelayRight.ReadDelayAt(FMath::Fmod(FMath::Max(FeedbackReadPosition, 0.0f), FeedbackDelayRight.GetDelayLengthSamples()));

			// ------------------------------- Right - Low-pass filter (damping) -------------------------------

			float FinalProcessedFeedbackSampleRight = FirstProcessedFeedbackSampleRight;

			// Apply damping before low pass.
			FinalProcessedFeedbackSampleRight *= DampingMultiplicationValue;

			float LowPassFeedbackSampleRight = 0.0f;
			
			LPDampingFilter.ProcessAudioFrame(&FinalProcessedFeedbackSampleRight, &LowPassFeedbackSampleRight);

			// ------------------------------- Right - Decay Sound -------------------------------
			
			FinalProcessedFeedbackSampleRight *= DecayRateVariable;

			// ------------------------------- Right - All Pass Filter - Diffuse 2 -------------------------------
			
			FinalProcessedFeedbackSampleRight = DecayDiffusionFilter2Right.ProcessAudioSample(LowPassFeedbackSampleRight);

			// ------------------------------- Right - Final Delay -------------------------------
			
			const float RightSampleFinalDelay = FMath::Clamp(*InFinalDelayRight, 0, 2000);
			const float FinalFeedbackSampleRight = PostLPFFeedbackDelayRight.ReadDelayAt(RightSampleFinalDelay);

			// ------------------------------- Right - Decay Sound  -------------------------------

			FinalProcessedFeedbackSampleRight *= DecayRateVariable;
			
			// ------------------------------- Process & Write Outputs -------------------------------

			// Mix the original (InputAudio) with the all-pass processed audio
			const float OriginalSample = InputAudio[FrameCount];

			// The delayed signal, mix the two delay taps together
			const float DelayedSample = Sample1 + Sample2;

			// Mix all output samples into one sample.
			const float MixedSample = (OriginalSample * *DryValue) 
			+ (DelayedSample * *WetValue)
			+ (FeedbackSampleLeft * *WetValue) + (FeedbackSampleRight * *WetValue)
			+ (FinalFeedbackSampleLeft * *WetValue) + (FinalFeedbackSampleRight * *WetValue);

			// Set output frame to this mixed sample
			OutputAudio[FrameCount] = MixedSample;

			// Write all delays using samples.
			WriteDelaysAndInc(ProcessedSample, FirstProcessedFeedbackSampleLeft, FirstProcessedFeedbackSampleRight, FinalProcessedFeedbackSampleLeft, FinalProcessedFeedbackSampleRight);
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
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamPreDelay), 50.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamPreLPF), 1.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamLowPassCutOff), 500.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamAllPassCutOff), 0.4f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamPreDiffuse_1), 0.750f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamPreDiffuse_2), 0.625f),

				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamDecayRate), 0.1f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamFeedbackDelay_1), 80.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamDecayDiffusion_1), 0.7f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamDecayDiffusion_2), 0.5f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamDelayDamping), 0.005f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamRandomDelay), 16.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamFeedbackDelay_2), 60.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamFinalDelay_1), 120.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamFinalDelay_2), 100.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamWetValue), 0.65f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamDryValue), 0.35f)
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
		FFloatReadRef FeedbackDelay1 = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamFeedbackDelay_1), InParams.OperatorSettings);
		FFloatReadRef DecayDiffusion1 = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamDecayDiffusion_1), InParams.OperatorSettings);
		FFloatReadRef DecayDiffusion2 = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamDecayDiffusion_2), InParams.OperatorSettings);
		FFloatReadRef DelayDamping = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamDelayDamping), InParams.OperatorSettings);

		FFloatReadRef RandomDelays = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamRandomDelay), InParams.OperatorSettings);
		FFloatReadRef FeedbackDelay2 = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamFeedbackDelay_2), InParams.OperatorSettings);

		FFloatReadRef FinalDelay1 = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamFinalDelay_1), InParams.OperatorSettings);
		FFloatReadRef FinalDelay2 = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamFinalDelay_2), InParams.OperatorSettings);

		FFloatReadRef WetValue = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamWetValue), InParams.OperatorSettings);
		FFloatReadRef DryValue = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamDryValue), InParams.OperatorSettings);

		return MakeUnique<FReverberationOperator>(InParams.OperatorSettings, AudioIn, PreDelayTime, PreLowPassFilter, LowPassCutoff, AllPassCutoff, InputDiffusion1, InputDiffusion2, DecayRate, FeedbackDelay1, DecayDiffusion1, DecayDiffusion2, DelayDamping, RandomDelays, FeedbackDelay2, FinalDelay1, FinalDelay2, WetValue, DryValue);
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
