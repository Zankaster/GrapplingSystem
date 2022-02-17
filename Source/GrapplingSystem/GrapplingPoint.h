// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"
#include "GrapplingPoint.generated.h"

UCLASS()
class GRAPPLINGSYSTEM_API AGrapplingPoint : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGrapplingPoint();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:

	/** Skeletal Mesh for the grappling point */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interface", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* GrappleMesh;

	/** Sphere Hitbox */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interface", meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionSphere;
	
	/** Popup widget for the grappling point indicator on screen */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interface", meta = (AllowPrivateAccess = "true"))
	UWidgetComponent* GrappleWidget;

	/** Popup widget for the grappling point indicator on screen */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interface", meta = (AllowPrivateAccess = "true"))
	USceneComponent* RopeOffsetTransform; 

public:

	/** Is the player looking at this grappling point? */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Interface", meta = (AllowPrivateAccess = "true"))
	bool bCharacterFocused;

	/** Set the grappling point focused */
	UFUNCTION(BlueprintCallable, Category = "Interface", meta = (AllowPrivateAccess = "true"))
	void EnableFocused();
	

};
