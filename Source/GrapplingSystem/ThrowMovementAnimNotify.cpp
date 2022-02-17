// Fill out your copyright notice in the Description page of Project Settings.


#include "ThrowMovementAnimNotify.h"

#include "GrapplingSystemCharacter.h"

void UThrowMovementAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	if(MeshComp && MeshComp->GetOwner())
	{
		AGrapplingSystemCharacter* Player = Cast<AGrapplingSystemCharacter>(MeshComp->GetOwner());
		if(Player)
		{
			Player->AnimNotify_GrappleLeapStart(MeshComp, Animation);
		}
	}
}
