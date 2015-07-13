// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AlessandroMalusa.h"
#include "AlessandroMalusaGameMode.h"
#include "AlessandroMalusaPawn.h"

AAlessandroMalusaGameMode::AAlessandroMalusaGameMode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// set default pawn class to our character class
	DefaultPawnClass = AAlessandroMalusaPawn::StaticClass();
}

