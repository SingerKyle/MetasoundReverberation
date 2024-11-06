// Copyright Epic Games, Inc. All Rights Reserved.

#include "Internationalization/Text.h"
#include "MetasoundExecutableOperator.h"
#include "MetasoundNodeRegistrationMacro.h"
#include "MetasoundParamHelper.h"
#include "MetasoundPrimitives.h"
#include "MetasoundStandardNodesNames.h"
#include "MetasoundAudioBuffer.h"
#include "DSP/Delay.h"
#include "MetasoundStandardNodesCategories.h"
#include "MetasoundFacade.h"

#define LOCTEXT_NAMESPACE "MetasoundStandardNodesPitchShift"

namespace Metasound
{
	namespace PitchShift
	{
		// METASOUND_PARAM: Variable Name - Node Name - Node Description.
		METASOUND_PARAM(InParamAudioInput, "In", "Audio input.")
		METASOUND_PARAM(InParamPitchShift, "Pitch Shift", "The amount to pitch shift the audio signal, in semitones.")
		METASOUND_PARAM(InParamDelayLength, "Delay Length", "The delay length of the internal delay buffer in milliseconds (10 ms to 100 ms). Changing this can reduce artifacts in certain pitch shift regions.")
		METASOUND_PARAM(OutParamAudio, "Out", "Audio output.")

		static constexpr float MinDelayLength = 10.0f;
		static constexpr float MaxDelayLength = 100.0f;
		static constexpr float MaxAbsPitchShiftInOctaves = 6.0f;
	}

	// Actual Class with all functions / variables etc.
	class FPitchShiftOperator : public TExecutableOperator<FPitchShiftOperator>
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
		FPitchShiftOperator(const FOperatorSettings& InSettings, 
			const FAudioBufferReadRef& InAudioInput, 
			const FFloatReadRef& InPitchShift,
			const FFloatReadRef& InDelayLength);

		// Returns the inputs for the operator (usually audio data or control parameters).
		virtual FDataReferenceCollection GetInputs() const override;
    
		// Returns the outputs of the operator (usually processed audio data).
		virtual FDataReferenceCollection GetOutputs() const override;
    
		// Executes the pitch shifting operation, modifying the audio data based on the inputs.
		void Execute();

	private:
		float GetPitchShiftClamped() const;
		float GetDelayLengthClamped() const;
		float GetPhasorPhaseIncrement() const;
		
		// The input audio buffer
		FAudioBufferReadRef AudioInput;

		// The user-defined pitch shift in semitones
		FFloatReadRef PitchShift;

		// The user-defined delay length (in milliseconds) of the internal delay buffer
		FFloatReadRef DelayLength;

		// The audio output
		FAudioBufferWriteRef AudioOutput;

		// The internal delay buffer
		Audio::FDelay DelayBuffer;

		// The sample rate of the node
		float SampleRate = 0.0f;
		
		// The delay length
		Audio::FExponentialEase CurrentDelayLength;
		
		// The previous pitch shift value
		float CurrentPitchShift = 0.0f;

		// The current phasor phase (goes between 0.0 and 1.0)
		float PhasorPhase = 0.0f;

		// The current phasor increment (a delta, plus or minus to add to the phase every frame)
		float PhasorPhaseIncrement = 0.0f;
	};

	/// Summary
	///
	/// For the constructor, I'm going to make a first-pass guess that we'll want our delay length to be 40 ms
	/// so we can initialize our delay buffer with the input sample rate and the desired 40 ms delay length. 
	///
	/// Summary
	FPitchShiftOperator::FPitchShiftOperator(const FOperatorSettings& InSettings,
		const FAudioBufferReadRef& InAudioInput,
		const FFloatReadRef& InPitchShift,
		const FFloatReadRef& InDelayLength)

		: AudioInput(InAudioInput)
		, PitchShift(InPitchShift)
		, DelayLength(InDelayLength)
		, AudioOutput(FAudioBufferWriteRef::CreateNew(InSettings))
		, SampleRate(InSettings.GetSampleRate())
	{
		// Initialize the delay buffer with the initial delay length 
		CurrentDelayLength.Init(GetDelayLengthClamped());
		DelayBuffer.Init(InSettings.GetSampleRate(), 0.001f * PitchShift::MaxDelayLength);
		CurrentPitchShift = GetPitchShiftClamped();
		PhasorPhaseIncrement = GetPhasorPhaseIncrement();
	}

	// The getter utility functions simply utilize the constants defined in the parameter section earlier.
	
	float FPitchShiftOperator::GetPitchShiftClamped() const
	{ 
		return FMath::Clamp(*PitchShift, -12.0f * PitchShift::MaxAbsPitchShiftInOctaves, 12.0f * PitchShift::MaxAbsPitchShiftInOctaves);
	}

	float FPitchShiftOperator::GetDelayLengthClamped() const
	{
		return FMath::Clamp(*DelayLength, PitchShift::MinDelayLength, PitchShift::MaxDelayLength);		
	}

	/// Summary
	///
	/// We'll want the utility to compute the phasor frequency and then the corresponding "phasor increment",
	/// which as mentioned before, is the amount of phase to increment each time a sample is rendered. 
	///
	/// Summary
	float FPitchShiftOperator::GetPhasorPhaseIncrement() const
	{
		// Derived from the following formula for doppler shift using a phasor
		// FrequencyOut = FrequencyIn * (1.0 - PhasorFrequency * DurationSeconds)
		// PitchScale = FrequencyOut / FrequencyIn
		// PhasorFrequency = (1.0f - PitchScale) / DurationSeconds
		const float PitchShiftRatio = Audio::GetFrequencyMultiplier(CurrentPitchShift);
		const float PhasorFrequency =  (1.0f - PitchShiftRatio) / (0.001f * CurrentDelayLength.PeekCurrentValue());
		return PhasorFrequency / SampleRate;
	}
	
	FDataReferenceCollection FPitchShiftOperator::GetInputs() const
	{
		using namespace PitchShift;

		FDataReferenceCollection InputDataReferences;
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamAudioInput), FAudioBufferReadRef(AudioInput));
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamPitchShift), FFloatReadRef(PitchShift));
		InputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(InParamDelayLength), FFloatReadRef(DelayLength));

		return InputDataReferences;
	}

	FDataReferenceCollection FPitchShiftOperator::GetOutputs() const
	{
		using namespace PitchShift;

		FDataReferenceCollection OutputDataReferences;
		OutputDataReferences.AddDataReadReference(METASOUND_GET_PARAM_NAME(OutParamAudio), FAudioBufferReadRef(AudioOutput));
		return OutputDataReferences;
	}

		void FPitchShiftOperator::Execute()
	{
		const float NewDelayLengthClamped = GetDelayLengthClamped();
		bool bRecomputePhasorIncrement = (!FMath::IsNearlyEqual(NewDelayLengthClamped, CurrentDelayLength.GetNextValue()));

		// Update the pitch shift data if it's changed
		const float NewPitchShiftClamped = GetPitchShiftClamped();
		bRecomputePhasorIncrement |=!FMath::IsNearlyEqual(NewPitchShiftClamped, CurrentPitchShift);

		if (bRecomputePhasorIncrement)
		{
			CurrentDelayLength.SetValue(NewDelayLengthClamped);
			CurrentPitchShift = NewPitchShiftClamped;
			PhasorPhaseIncrement = GetPhasorPhaseIncrement();
		}

		// The const-only input audio data
		const float* InputAudio = AudioInput->GetData();

		// The writable output audio data
		float* OutputAudio = AudioOutput->GetData();

		// The number of frames we're rendering this block
		const int32 NumFrames = AudioInput->Num();

		for (int32 FrameIndex = 0; FrameIndex < NumFrames; ++FrameIndex)
		{
			// Update the interpolated delay length value
			if (!CurrentDelayLength.IsDone())
			{
				PhasorPhaseIncrement = GetPhasorPhaseIncrement();
				CurrentDelayLength.GetNextValue();
			}
		
			// Compute the two tap delay read locations, one shifted 90 degrees out of phase
			const float PhasorPhaseOffset = FMath::Fmod(PhasorPhase + 0.5f, 1.0f);
			const float DelayTapRead1 = CurrentDelayLength.PeekCurrentValue() * PhasorPhase;
			const float DelayTapRead2 = CurrentDelayLength.PeekCurrentValue() * PhasorPhaseOffset;

			// This produces an overlapping cosine function that avoids pops in the output
			const float DelayTapGain1 = FMath::Cos(PI * (PhasorPhase - 0.5f));
			const float DelayTapGain2 = FMath::Cos(PI * (PhasorPhaseOffset - 0.5f));
			
			// Read the delay lines at the given tap indices, apply the gains
			const float Sample1 = DelayTapGain1 * DelayBuffer.ReadDelayAt(DelayTapRead1);
			const float Sample2 = DelayTapGain2 * DelayBuffer.ReadDelayAt(DelayTapRead2);

			// Sum the outputs into the output frame
			const float OutputSample = Sample1 + Sample2;
			OutputAudio[FrameIndex] = OutputSample;

			// Update the phasor state
			PhasorPhase += PhasorPhaseIncrement;
			// Make sure we wrap to between 0.0 and 1.0 after incrementing the phase 
			PhasorPhase = FMath::Wrap(PhasorPhase, 0.0f, 1.0f);

			// Write the output to the delay buffer
			DelayBuffer.WriteDelayAndInc(InputAudio[FrameIndex]);
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
	const FVertexInterface& FPitchShiftOperator::GetVertexInterface()
	{
		using namespace PitchShift;

		static const FVertexInterface Interface(
			FInputVertexInterface(
				TInputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamAudioInput)),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamPitchShift), 0.0f),
				TInputDataVertex<float>(METASOUND_GET_PARAM_NAME_AND_METADATA(InParamDelayLength), 30.0f)
			),
			FOutputVertexInterface(
				TOutputDataVertex<FAudioBuffer>(METASOUND_GET_PARAM_NAME_AND_METADATA(OutParamAudio))
			)
		);

		return Interface;
	}

	
	const FNodeClassMetadata& FPitchShiftOperator::GetNodeInfo()
	{
		auto InitNodeInfo = []() -> FNodeClassMetadata
		{
			FNodeClassMetadata Info;
			Info.ClassName = { StandardNodes::Namespace, "Pitch Shift", StandardNodes::AudioVariant };
			Info.MajorVersion = 1;
			Info.MinorVersion = 1;
			Info.DisplayName = METASOUND_LOCTEXT("DelayNode_DisplayName", "Pitch Shift");
			Info.Description = METASOUND_LOCTEXT("DelayNode_Description", "Pitch shifts the audio buffer using a doppler shift method.");
			Info.Author = PluginAuthor;
			Info.PromptIfMissing = PluginNodeMissingPrompt;
			Info.DefaultInterface = GetVertexInterface();
			Info.CategoryHierarchy.Emplace(NodeCategories::Delays);
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
	TUniquePtr<IOperator> FPitchShiftOperator::CreateOperator(const FCreateOperatorParams& InParams, FBuildErrorArray& OutErrors)
	{
		using namespace PitchShift; 

		const FDataReferenceCollection& InputCollection = InParams.InputDataReferences;
		const FInputVertexInterface& InputInterface = GetVertexInterface().GetInputInterface();

		FAudioBufferReadRef AudioIn = InputCollection.GetDataReadReferenceOrConstruct<FAudioBuffer>(METASOUND_GET_PARAM_NAME(InParamAudioInput), InParams.OperatorSettings);
		FFloatReadRef PitchShift = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamPitchShift), InParams.OperatorSettings);
		FFloatReadRef DelayLength = InputCollection.GetDataReadReferenceOrConstructWithVertexDefault<float>(InputInterface, METASOUND_GET_PARAM_NAME(InParamDelayLength), InParams.OperatorSettings);

		return MakeUnique<FPitchShiftOperator>(InParams.OperatorSettings, AudioIn, PitchShift, DelayLength);
	}

	class FPitchShiftNode : public FNodeFacade
	{
	public:
		/**
		 * Constructor used by the Metasound Frontend.
		 */
		FPitchShiftNode(const FNodeInitData& InitData)
			: FNodeFacade(InitData.InstanceName, InitData.InstanceID, TFacadeOperatorClass<FPitchShiftOperator>())
		{
		}
	};


	METASOUND_REGISTER_NODE(FPitchShiftNode)
}

#undef LOCTEXT_NAMESPACE
