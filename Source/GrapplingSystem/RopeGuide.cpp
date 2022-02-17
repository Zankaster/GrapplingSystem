// Fill out your copyright notice in the Description page of Project Settings.


#include "RopeGuide.h"

#include <ThirdParty/openexr/Deploy/OpenEXR-2.3.0/OpenEXR/include/ImathMath.h>

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

// Sets default values
ARopeGuide::ARopeGuide()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	ElapsedTime = 0.f;
	TargetTime = 0.5f;
	bIsFlying = false;
}

void ARopeGuide::SetTarget(FVector StartPosition, FVector EndPosition)
{
	Start = StartPosition;
	End = EndPosition;
	Direction = UKismetMathLibrary::GetDirectionUnitVector(Start, End);
	bIsFlying = true;
	ElapsedTime = 0.f;
}

void ARopeGuide::UpdatePosition(float DeltaTime)
{
	if(!bIsFlying) return;
	FLatentActionInfo LatentInfo;
	LatentInfo.CallbackTarget = this;
	ElapsedTime+= DeltaTime;
	ElapsedTime = UKismetMathLibrary::FClamp(ElapsedTime, 0.f, TargetTime);
	FVector NewPosition = UKismetMathLibrary::VLerp(Start, End, ElapsedTime/TargetTime);
	SetActorLocation(NewPosition,false);

	// When the object reaches destination, destroy it
	if(ElapsedTime >= TargetTime)
	{
		bIsFlying = false;
		this->Destroy();
	}
}

// Called when the game starts or when spawned
void ARopeGuide::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ARopeGuide::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdatePosition(DeltaTime);
}

