// Copyright Epic Games, Inc. All Rights Reserved.

#include "ParkourFPSGameMode.h"
#include "ParkourFPSCharacter.h"
#include "UObject/ConstructorHelpers.h"

AParkourFPSGameMode::AParkourFPSGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
