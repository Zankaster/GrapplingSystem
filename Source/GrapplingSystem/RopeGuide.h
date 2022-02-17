// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RopeGuide.generated.h"

UCLASS()
class GRAPPLINGSYSTEM_API ARopeGuide : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARopeGuide();

	/** Visible Mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

	/** Sets the Start and End positions. Called by the Character when starting a grappling leap */
	void SetTarget(FVector StartPosition, FVector EndPosition);
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	float ElapsedTime;

	/** The time it takes for the object to reach destination */
	float TargetTime;

	/** Is the object moving? It should always be true, when it is not it
	 *  means the object gets destroyed */
	bool bIsFlying;

	/** Start position */
	FVector Start;

	/** End position */
	FVector End;

	/** Movement direction */
	FVector Direction;

	/** Called every frame to update the position of the object */
	void UpdatePosition(float DeltaTime);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};


