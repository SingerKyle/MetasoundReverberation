// Copyright Epic Games, Inc. All Rights Reserved.

#include "CMP407_ReverberationGameMode.h"
#include "CMP407_ReverberationCharacter.h"
#include "UObject/ConstructorHelpers.h"

ACMP407_ReverberationGameMode::ACMP407_ReverberationGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
