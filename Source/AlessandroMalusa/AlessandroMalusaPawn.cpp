// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AlessandroMalusa.h"
#include "AlessandroMalusaPawn.h"
#include "ShipAIController.h"
#include "AlessandroMalusaProjectile.h"
#include "AlessandroMalusaProjectileC.h"
#include "TimerManager.h"

const FName AAlessandroMalusaPawn::MoveForwardBinding("MoveForward");
const FName AAlessandroMalusaPawn::MoveRightBinding("MoveRight");
const FName AAlessandroMalusaPawn::FireForwardBinding("FireForward");
const FName AAlessandroMalusaPawn::FireRightBinding("FireRight");

AAlessandroMalusaPawn::AAlessandroMalusaPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{	
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ShipMesh(TEXT("/Game/TwinStick/Meshes/TwinStickUFO.TwinStickUFO"));
	// Create the mesh component
	ShipMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	RootComponent = ShipMeshComponent;
	ShipMeshComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	ShipMeshComponent->SetStaticMesh(ShipMesh.Object);
	
	// Cache our sound effect
	static ConstructorHelpers::FObjectFinder<USoundBase> FireAudio(TEXT("/Game/TwinStick/Audio/TwinStickFire.TwinStickFire"));
	FireSound = FireAudio.Object;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->AttachTo(RootComponent);
	CameraBoom->bAbsoluteRotation = true; // Don't want arm to rotate when ship does
	CameraBoom->TargetArmLength = 1200.f;
	CameraBoom->RelativeRotation = FRotator(-80.f, 0.f, 0.f);
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	CameraComponent->AttachTo(CameraBoom, USpringArmComponent::SocketName);
	CameraComponent->bUsePawnControlRotation = false;	// Camera does not rotate relative to arm

	// Movement
	MoveSpeed = 1000.0f;
	// Weapon
	GunOffset = FVector(90.f, 0.f, 0.f);
	FireRate = 0.1f;
	bCanFire = true;

	//default firemode is Linear!!!
	FireMode = Linear;
}

template<AAlessandroMalusaPawn::FireType eFireType>
void AAlessandroMalusaPawn::ChangeFire()
{
	ChangeFire(eFireType);
}

inline void AAlessandroMalusaPawn::ChangeFire(FireType eFireType)
{
  FireMode = eFireType;
}

void AAlessandroMalusaPawn::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	check(InputComponent);

	//add shift key binding to change firemode
	InputComponent->BindAction("FireChange", IE_Pressed, this, &AAlessandroMalusaPawn::ChangeFire<Curve>);
	InputComponent->BindAction("FireChange", IE_Released, this, &AAlessandroMalusaPawn::ChangeFire<Linear>);


	// set up gameplay key bindings
	InputComponent->BindAxis(MoveForwardBinding);
	InputComponent->BindAxis(MoveRightBinding);
	InputComponent->BindAxis(FireForwardBinding);
	InputComponent->BindAxis(FireRightBinding);
}

void AAlessandroMalusaPawn::Tick(float DeltaSeconds)
{
	// Find movement direction
	const float ForwardValue = GetInputAxisValue(MoveForwardBinding);
	const float RightValue = GetInputAxisValue(MoveRightBinding);

	// Clamp max size so that (X=1, Y=1) doesn't cause faster movement in diagonal directions
	const FVector MoveDirection = FVector(ForwardValue, RightValue, 0.f).GetClampedToMaxSize(1.0f);

	// Calculate  movement
	const FVector Movement = MoveDirection * MoveSpeed * DeltaSeconds;

	// If non-zero size, move this actor
	if (Movement.SizeSquared() > 0.0f)
	{
		const FRotator NewRotation = Movement.Rotation();
		FHitResult Hit(1.f);
		RootComponent->MoveComponent(Movement, NewRotation, true, &Hit);
		
		if (Hit.IsValidBlockingHit())
		{
			const FVector Normal2D = Hit.Normal.GetSafeNormal2D();
			const FVector Deflection = FVector::VectorPlaneProject(Movement, Normal2D) * (1.f - Hit.Time);
			RootComponent->MoveComponent(Deflection, NewRotation, true);
		}
	}
	
	// Create fire direction vector
	const float FireForwardValue = GetInputAxisValue(FireForwardBinding);
	const float FireRightValue = GetInputAxisValue(FireRightBinding);
	const FVector FireDirection = FVector(FireForwardValue, FireRightValue, 0.f);

	// Try and fire a shot
	FireShot(FireDirection);
}

void AAlessandroMalusaPawn::FireShot(FVector FireDirection)
{
	// If we it's ok to fire again
	if (bCanFire == true)
	{
		// If we are pressing fire stick in a direction
		if (FireDirection.SizeSquared() > 0.0f)
		{
			const FRotator FireRotation = FireDirection.Rotation();
			// Spawn projectile at an offset from this pawn
			const FVector SpawnLocation = GetActorLocation() + FireRotation.RotateVector(GunOffset);

			UWorld* const World = GetWorld();

			if (World != nullptr)
			{
				// spawn the projectile

				switch (FireMode)

				{

				case Linear:
					World->SpawnActor<AAlessandroMalusaProjectile>(SpawnLocation, FireRotation);
					break;
				case Curve:
					World->SpawnActor<AAlessandroMalusaProjectileC>(SpawnLocation, FireRotation);
					break;
				default:
					break;
				}
			}

			bCanFire = false;
			World->GetTimerManager().SetTimer(TimerHandle_ShotTimerExpired, this, &AAlessandroMalusaPawn::ShotTimerExpired, FireRate);

			// try and play the sound if specified
			if (FireSound != nullptr)
			{
				UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
			}

			bCanFire = false;
		}
	}
}

void AAlessandroMalusaPawn::ShotTimerExpired()
{
	bCanFire = true;
}

void AAlessandroMalusaPawn::SpawnDefaultController()
{

	if (AIControllerClass != nullptr && AIControllerClass == AShipAIController::StaticClass())
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Instigator = Instigator;
		SpawnInfo.bNoCollisionFail = true;
		SpawnInfo.OverrideLevel = GetLevel();

		AController * NewController = GetWorld()->SpawnActor<AShipAIController>(AIControllerClass, GetActorLocation(), GetActorRotation(), SpawnInfo);
	
		if (NewController != nullptr)
		{
			// if successful will result in setting this->Controller 
			// as part of possession mechanics
			NewController->Possess(this);
		}
	}
}