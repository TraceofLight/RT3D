#include "pch.h"
#include "Asset/Mesh/Public/SquareActor.h"
#include "Asset/Mesh/Public/SquareComponent.h"

IMPLEMENT_CLASS(ASquareActor, AActor)

ASquareActor::ASquareActor()
{
	SquareComponent = CreateDefaultSubobject<USquareComponent>("SquareComponent");
	SquareComponent->SetRelativeRotation({ 90, 0, 0 });
	SquareComponent->SetOwner(this);
	SetRootComponent(SquareComponent);
}
