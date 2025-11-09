#include "pch.h"
#include "Actor/Public/Actor.h"
#include "Actor/Public/SkeletalMeshActor.h"

IMPLEMENT_CLASS(ASkeletalMeshActor, AActor)

ASkeletalMeshActor::ASkeletalMeshActor()
{
}

UClass* ASkeletalMeshActor::GetDefaultRootComponent()
{
	return USkeletalMeshComponent::StaticClass();
}
