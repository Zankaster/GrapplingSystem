// Copyright Epic Games, Inc. All Rights Reserved.

#include "GrapplingSystemCharacter.h"

#include "DrawDebugHelpers.h"
#include "GrapplingPoint.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "CableComponent.h"
#include "RopeGuide.h"

//////////////////////////////////////////////////////////////////////////
// AGrapplingSystemCharacter

AGrapplingSystemCharacter::AGrapplingSystemCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)

	// Grappling parameters
	GrappleEndVerticalOffset = 100.f;
	GrapplingSpeed = 1500.f;
}

void AGrapplingSystemCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Check if the character is looking at a grappling point
	LineTraceGrapplingPoint();

	//Rotate towards the focused grappling point, if any
	RotateTowardsGrapplingPoint(DeltaSeconds);

	//Leap towards the focused grappling point, if any
	Grapple(DeltaSeconds);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AGrapplingSystemCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGrapplingSystemCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGrapplingSystemCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AGrapplingSystemCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AGrapplingSystemCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AGrapplingSystemCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AGrapplingSystemCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AGrapplingSystemCharacter::OnResetVR);

	PlayerInputComponent->BindAction("Grapple", IE_Pressed, this, &AGrapplingSystemCharacter::StartGrappling);
}

void AGrapplingSystemCharacter::OnResetVR()
{
	// If GrapplingSystem is added to a project via 'Add Feature' in the Unreal Editor the dependency on HeadMountedDisplay in GrapplingSystem.Build.cs is not automatically propagated
	// and a linker error will result.
	// You will need to either:
	//		Add "HeadMountedDisplay" to [YourProject].Build.cs PublicDependencyModuleNames in order to build successfully (appropriate if supporting VR).
	// or:
	//		Comment or delete the call to ResetOrientationAndPosition below (appropriate if not supporting VR)
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AGrapplingSystemCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void AGrapplingSystemCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void AGrapplingSystemCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AGrapplingSystemCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AGrapplingSystemCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AGrapplingSystemCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void AGrapplingSystemCharacter::StartGrappling()
{
	// If the character is not looking at a grapple point, or if he's 
	// already leaping towards one, do nothing
	if(!bGrapplePointFocused) return;
	if(bIsGrappling|| bIsRotatingTowardsGrapplePoint) return;

	// Get the character and grappling point positions to calculate the leap duration
	// Note that these values are not exact in the case the character starts leaping while
	// in the air, but overall it still works
	GrappleStartLocation = GetActorLocation();
	GrappleTotalDistance = UKismetMathLibrary::Vector_Distance(GrappleStartLocation, GrappleEndLocation);
	GrappleTotalDuration = GrappleTotalDistance/GrapplingSpeed;
	
	// Check if the path from start to end is clear, doing a discrete number of capsule-casts
	// along the trajectory the character would have to travel across
	const int TestCount = 10;
	FVector Direction = UKismetMathLibrary::GetDirectionUnitVector(GrappleStartLocation, GrappleEndLocation);
	FVector TestLocation;
	float Offset = GrappleTotalDistance / TestCount;
	float CurveValue; 

	FHitResult OutHitResult;
	bool bHitObstacle;
	bool bFoundAnyObstacle = false;
	for(int i = 0; i <= TestCount; i++)
	{
		CurveValue = GrapplingVerticalCurve->GetFloatValue(float(i)/TestCount);
		TestLocation = Direction * i * Offset + GrappleStartLocation + GetActorUpVector()*CurveValue*300.f;
		bHitObstacle = GetWorld()->SweepSingleByChannel(
			OutHitResult,TestLocation,TestLocation,FQuat::Identity, ECollisionChannel::ECC_Visibility,
			FCollisionShape::MakeCapsule(GetCapsuleComponent()->GetScaledCapsuleRadius(),
			                             GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
		if(bHitObstacle) bFoundAnyObstacle = true;
		DrawDebugCapsule(GetWorld(),TestLocation,GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), GetCapsuleComponent()->GetScaledCapsuleRadius(),
		                 FQuat::Identity,bHitObstacle ? FColor::Red : FColor::Blue, false,2.f,3,0.25f);
	}

	// If obstacles have been found, exit
	if(bFoundAnyObstacle) return;

	// If no obstacle has been found in the path between the character and the grappling point,
	// spawn the rope, fire the rope throw animation, and rotate towards the grappling point
	Rope();
	bIsRotatingTowardsGrapplePoint = true;
	AnimInstance = GetMesh()->GetAnimInstance();
	AnimInstance->Montage_Play(GrappleThrowMontage);
	AnimInstance->Montage_JumpToSection(FName("Default"));
}

void AGrapplingSystemCharacter::RotateTowardsGrapplingPoint(float DeltaTime)
{
	if(!bIsRotatingTowardsGrapplePoint) return;

	// Simple rotation towards the grappling point, before to actually leap towards it
	FVector GrappleDirection = UKismetMathLibrary::GetDirectionUnitVector(GrappleStartLocation, GrappleEndLocation);
	FRotator NewRotation = UKismetMathLibrary::MakeRotFromX(GrappleDirection);
	FRotator InterpRotation = UKismetMathLibrary::RInterpTo(GetActorRotation(),NewRotation,DeltaTime,10.f);
	InterpRotation.Pitch = 0.f;
	InterpRotation.Roll = 0.f;
	GetCapsuleComponent()->SetWorldRotation(InterpRotation);
}

void AGrapplingSystemCharacter::Grapple(float DeltaTime)
{
	// Since the function is called every frame it has to check if the character
	// is actually doing the leap, if not exit
	if(!bIsGrappling) return;

	// Stop rotating towards the grapple point
	bIsRotatingTowardsGrapplePoint = false;

	// Increase the elapsed time, which is used as Linear Interpolation Alpha for the leap motion
	ElapsedGrapplingTime+= DeltaTime;
	const float LerpValue = FMath::Clamp(ElapsedGrapplingTime/GrappleTotalDuration,0.f,1.f);

	// Sample the curve value to determine the vertical position of the character
	const float CurveValue = GrapplingVerticalCurve->GetFloatValue(LerpValue);
	const FVector NewLocation = UKismetMathLibrary::VLerp(GrappleStartLocation, GrappleEndLocation, LerpValue) + GetActorUpVector()*CurveValue*300.f;
	FLatentActionInfo LatentInfo;
	LatentInfo.CallbackTarget = this;

	// Move the character to the new position
	UKismetSystemLibrary::MoveComponentTo(GetCapsuleComponent(), NewLocation, GetActorRotation(),
	                                      false, false, 0.1f, false, EMoveComponentAction::Move, LatentInfo);

	// If the character reached destination, stop all the leap logic for next
	// frame and destroy the rope he's holding
	if(ElapsedGrapplingTime >= GrappleTotalDuration)
	{
		bIsGrappling = false;
		ThrowableRope->DestroyComponent();
	}
}

void AGrapplingSystemCharacter::Rope()
{
	// Spawn the object that will act as rope end and move towards the grappling point
	FActorSpawnParameters SpawnParams;
	ARopeGuide* RopeGuide = GetWorld()->SpawnActor<ARopeGuide>(RopeGuideObject, GetTransform(), SpawnParams);
	RopeGuide->SetTarget(GetActorLocation(),GrappleScenePoint->GetComponentLocation());
	RopeGuide->RegisterAllComponents();

	// Spawn the cable component that will act as rope, and fix one end to the character's hand, and the
	// other end to the object that moves towards the grappling point
	FAttachmentTransformRules AttRules = FAttachmentTransformRules( EAttachmentRule::KeepRelative, false );;
	ThrowableRope = NewObject<UCableComponent>(this, UCableComponent::StaticClass(), FName("ThrowableRope"));
	ThrowableRope->CableLength = UKismetMathLibrary::Vector_Distance(GrappleStartLocation, GrappleEndLocation)/2;
	ThrowableRope->EndLocation = {0,0,0};
	ThrowableRope->AttachToComponent(GetMesh(), AttRules,"hand_rSocket");
	ThrowableRope->SetAttachEndToComponent(RopeGuide->Mesh);
	ThrowableRope->RegisterComponent();
}

void AGrapplingSystemCharacter::AnimNotify_GrappleLeapStart(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	// The leap start point is updated here to handle cases where the 
	// character starts the grapple when he's in the air and moving
	GrappleStartLocation = GetActorLocation();
	
	// Setting bGrappling to true, the leap movement will start
	bIsGrappling = true;
	
	// bGrapplePointFocused was set to true to avoid the grapple button spam,
	// but now that the leap started it's reset so the character can grapple again
	bGrapplePointFocused = false;
	ElapsedGrapplingTime = 0;
	
}

bool AGrapplingSystemCharacter::LineTraceGrapplingPoint()
{
	// Disable line tracing while the grappling routine has started
	if(bIsGrappling || bIsRotatingTowardsGrapplePoint) return false;
	
	// Get Viewport Size
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	// Get screen space location of crosshairs
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	FHitResult OutHitResult;

	// Get world position and direction of crosshairs
	const bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection);

	if (bScreenToWorld)
	{
		// Trace from Crosshair world location outward
		const FVector Start{ CrosshairWorldPosition };
		const FVector End{ Start + CrosshairWorldDirection * 50'000.f };

		GetWorld()->SweepSingleByChannel(OutHitResult, Start, End,FQuat::Identity, ECC_GameTraceChannel1,
		                                 FCollisionShape::MakeBox(FVector(0.01f, 0.01f, 0.01f)));

		if(OutHitResult.bBlockingHit)
		{
			// If the line trace was successful check if the hit was a grappling point,
			// it should be since the sweep was set to find only objects with that
			// collision channel
			AGrapplingPoint* TraceHitItem= Cast<AGrapplingPoint>(OutHitResult.Actor);
			if(TraceHitItem)
			{
				// If the hit item is a GrapplingPoint, send the message to enable it
				// (which will cause its UI Widget to enlarge
				TraceHitItem->EnableFocused();
				bGrapplePointFocused = true;
				// Set the end location as the GrapplingPoint plus a small vertical offset
				GrappleEndLocation = TraceHitItem->GetActorLocation() + FVector::UpVector*GrappleEndVerticalOffset;
				GrappleScenePoint = OutHitResult.GetComponent();
				return true;
			}
		}
	}
	bGrapplePointFocused = false;
	return false;
}
