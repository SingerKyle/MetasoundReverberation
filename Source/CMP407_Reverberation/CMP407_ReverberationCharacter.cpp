// Copyright Epic Games, Inc. All Rights Reserved.

#include "CMP407_ReverberationCharacter.h"
#include "CMP407_ReverberationProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ACMP407_ReverberationCharacter

ACMP407_ReverberationCharacter::ACMP407_ReverberationCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(35.f, 96.0f);

	// Create Spring Arm
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Spring Arm"));
	SpringArm->SetupAttachment(GetCapsuleComponent());
	SpringArm->TargetArmLength = 0;
	SpringArm->bDoCollisionTest = false;
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 10.f;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraRotationLagSpeed = 25.f;
	
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(SpringArm);
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = false;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	//Mesh1P->SetRelativeRotation(FRotator(0.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	GetCharacterMovement()->MaxWalkSpeed = 150.f;

	WalkSounds_Planet = CreateDefaultSubobject<USoundBase>(TEXT("Planet WalkSounds"));
	WalkSounds_Ship = CreateDefaultSubobject<USoundBase>(TEXT("Ship WalkSounds"));

	RunSounds_Planet = CreateDefaultSubobject<USoundBase>(TEXT("Planet RunSounds"));
	RunSounds_Ship = CreateDefaultSubobject<USoundBase>(TEXT("Ship RunSounds"));
	
	LandSounds_Planet = CreateDefaultSubobject<USoundBase>(TEXT("Planet LandSounds"));
	LandSounds_Ship = CreateDefaultSubobject<USoundBase>(TEXT("Ship LandSounds"));

	
	
}

void ACMP407_ReverberationCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	
	if (WalkCurveFloat)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Meow"));

		WalkTimeline.SetLooping(false); // Prevent infinite looping
		WalkTimeline.SetTimelineLength(0.5f); // Set total duration
		WalkTimeline.SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);
		
		
		// Bind to finish event
		FOnTimelineEvent TimelineEvent;
		TimelineEvent.BindUFunction(this, FName("TryFootstep"));
		WalkTimeline.AddEvent(0.5f, TimelineEvent);
	}
}

void ACMP407_ReverberationCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (WalkTimeline.IsPlaying())
	{
		WalkTimeline.TickTimeline(DeltaTime);
	}

	if(GetCharacterMovement()->Velocity.Length() <= 0)
	{
		WalkTimeline.Stop();
	}
}

void ACMP407_ReverberationCharacter::TryFootstep()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Try Step"));
	
	const FVector Start = GetActorLocation();
	FVector End = Start + (FVector(0,0,-GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
	float radius = GetCapsuleComponent()->GetScaledCapsuleRadius();

	FHitResult Outhit;
	FCollisionQueryParams Params;
	Params.bReturnPhysicalMaterial = true;

	if(GetWorld()->SweepSingleByChannel(Outhit, Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(radius), Params))
	{
		switch (Outhit.PhysMaterial->SurfaceType)
		{
		case EPhysicalSurface::SurfaceType1: // Metal
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), WalkSounds_Ship, Outhit.Location, 1, 1);
			break;
		case EPhysicalSurface::SurfaceType2: // Planet
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), WalkSounds_Planet, Outhit.Location, 1, 1);
			break;
		default:
            				
			break;
		}

		//WalkTimeline.Stop();
	}
}

void ACMP407_ReverberationCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	const FVector Start = GetActorLocation();
	FVector End = Start + (FVector(0,0,-GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2));
	float radius = GetCapsuleComponent()->GetScaledCapsuleRadius();

	FHitResult Outhit;
	FCollisionQueryParams Params;
	Params.bReturnPhysicalMaterial = true;

	if(GetWorld()->SweepSingleByChannel(Outhit, Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(radius), Params))
	{
		DrawDebugSphere(GetWorld(), Start, radius, 12, FColor::Orange, false, 5.0f);
		DrawDebugSphere(GetWorld(), End, radius, 12, FColor::Red, false, 5.0f);
		
		switch (Outhit.PhysMaterial->SurfaceType)
		{
		case EPhysicalSurface::SurfaceType1: // Metal
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), LandSounds_Ship, Outhit.Location, 1, 1);
			break;
		case EPhysicalSurface::SurfaceType2: // Planet
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), LandSounds_Planet, Outhit.Location, 1, 1);
			break;
		default:
				
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////// Input

void ACMP407_ReverberationCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACMP407_ReverberationCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACMP407_ReverberationCharacter::Look);

		// Sprinting
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ACMP407_ReverberationCharacter::Sprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ACMP407_ReverberationCharacter::EndSprint);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void ACMP407_ReverberationCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if(GetCharacterMovement()->Velocity.Length() > 0)
	{
		if (!WalkTimeline.IsPlaying())
		{
			WalkTimeline.PlayFromStart(); // Start the timeline from the beginning
			UE_LOG(LogTemp, Warning, TEXT("Timeline started."));
		}	
	}
	else
	{
		if (WalkTimeline.IsPlaying())
		{
			WalkTimeline.Stop();
		}
	}
	
	
	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void ACMP407_ReverberationCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ACMP407_ReverberationCharacter::Sprint()
{
	GetCharacterMovement()->MaxWalkSpeed = 300.f; // add variables
}

void ACMP407_ReverberationCharacter::EndSprint()
{
	GetCharacterMovement()->MaxWalkSpeed = 150.f; // add variables
}
