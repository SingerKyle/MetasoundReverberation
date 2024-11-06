// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CMP407_Reverberation : ModuleRules
{
	public CMP407_Reverberation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "MetasoundGraphCore", "MetasoundEngine",
			"MetasoundFrontend", "MetasoundStandardNodes", "SignalProcessing"  });
	}
}
