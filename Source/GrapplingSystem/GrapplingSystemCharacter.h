// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CableComponent.h"
#include "RopeGuide.h"
#include "GameFramework/Character.h"
#include "GrapplingSystemCharacter.generated.h"

UCLASS(config=Game)
class AGrapplingSystemCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
public:
	AGrapplingSystemCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	/** Component that will act as rope end */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grappling", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ARopeGuide> RopeGuideObject;

protected:

	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

	void Tick(float DeltaSeconds) override;

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION(BlueprintCallable, Category = "Grappling")
	//void AnimNotify_GrappleLeapStartNow();
	void AnimNotify_GrappleLeapStart(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Grappling")
	UAnimNotify* ThrowGrapple;

protected:

    /** Is the character looking at a grappling point? */
	bool bGrapplePointFocused;
	
	/** Is the character doing a grappling leap? */
	bool bIsGrappling;

	/** Is the character doing a grappling leap? */
	bool bIsRotatingTowardsGrapplePoint;
	
	/** Distance between the character and the grappling point when he starts the leap */
	float GrappleTotalDistance;

	/** Time passed since the character started the grappling leap */
	float ElapsedGrapplingTime;

	/** Estimate total time it takes for the character to reach the grappling point when he starts the leap */
	float GrappleTotalDuration;

	/** Vertical offset to make the character land a bit above the grappling point */
	float GrappleEndVerticalOffset;

	/** Character location when he starts the grappling leap */
	FVector GrappleStartLocation;

	/** Grappling point location */
	FVector GrappleEndLocation;

	/** Rope that is spawned when the character starts the grappling leap */
	UCableComponent* ThrowableRope;
	
	/** Raytrace looking for a grappling point */
	bool LineTraceGrapplingPoint();

	/** Evaluates if the character can start a leap, checking if a grappling point is selected and if there
	 *  are no obstacles in the path */
	void StartGrappling();
	
	/** Launches the character towards the focused grappling point */
	void Grapple(float DeltaTime);

	/** Rotate the character towards the focused grappling point */
	void RotateTowardsGrapplingPoint(float DeltaTime);

	/** Vertical displacement in the grappling leap motion */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grappling", meta = (AllowPrivateAccess = "true"))
	UCurveFloat* GrapplingVerticalCurve;

	/** Base speed multiplier of grappling leap */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grappling", meta = (AllowPrivateAccess = "true"))
	float GrapplingSpeed;

	/** Spawn a rope in the end of the character when he starts the grappling leap */
	void Rope();

	/** Reference to the focused grappling point, needed to set the end point of the spawned rope */
	USceneComponent* GrappleScenePoint;

	/** Montage for throwing the grapple */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grappling", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* GrappleThrowMontage;

	/** Reference to the Animations */
	UAnimInstance* AnimInstance;

};







