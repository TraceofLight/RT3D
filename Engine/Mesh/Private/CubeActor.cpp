#include "pch.h"
#include "Mesh/Public/CubeActor.h"
#include "Mesh/Public/CubeComponent.h"

IMPLEMENT_CLASS(ACubeActor, AActor)

ACubeActor::ACubeActor()
{
	CubeComponent = CreateDefaultSubobject<UCubeComponent>("CubeComponent");
	CubeComponent->SetOwner(this);
	SetRootComponent(CubeComponent);
}
