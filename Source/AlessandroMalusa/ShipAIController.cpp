// Fill out your copyright notice in the Description page of Project Settings.

#include "AlessandroMalusa.h"
#include "ShipAIController.h"
#include "AlessandroMalusaPawn.h"

AShipAIController::AShipAIController(const FObjectInitializer & OI) : Super(OI)
{
	PrimaryActorTick.bCanEverTick = true;
}

void AShipAIController::Possess(APawn* PossessedPawn)
{
	//assign and cast only on possession
	m_ptrShipPawn = dynamic_cast<AAlessandroMalusaPawn*>(PossessedPawn);
	
	SetPawn(PossessedPawn);
}

void AShipAIController::UnPossess()
{
	SetPawn(nullptr);
}

void AShipAIController::Tick(float DeltaTime)
{

	//optimized code
	if (m_ptrShipPawn != nullptr)
	{
		m_ptrShipPawn->FireShot(FVector::ForwardVector);
	}

}


