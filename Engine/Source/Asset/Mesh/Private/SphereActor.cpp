#include "pch.h"
#include "Asset/Mesh/Public/SphereActor.h"
#include "Asset/Mesh//Public/SphereComponent.h"

IMPLEMENT_CLASS(ASphereActor, AActor)

ASphereActor::ASphereActor()
{
	SphereComponent = CreateDefaultSubobject<USphereComponent>("SphereComponent");
	SphereComponent->SetOwner(this);
	SetRootComponent(SphereComponent);
}
