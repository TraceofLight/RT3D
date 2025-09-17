#include "pch.h"
#include "Actor/Public/CubeActor.h"
#include "Component/Mesh/Public/CubeComponent.h"

IMPLEMENT_CLASS(ACubeActor, AActor)

ACubeActor::ACubeActor()
{
	CubeComponent = CreateDefaultSubobject<UCubeComponent>("CubeComponent");
	CubeComponent->SetOwner(this);
	SetRootComponent(CubeComponent);
}
