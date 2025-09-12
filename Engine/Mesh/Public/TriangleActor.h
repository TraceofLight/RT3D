#pragma once

#include "Mesh/Public/Actor.h"

UCLASS()
class ATriangleActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(ATriangleActor, AActor)

	using Super = AActor;
public:
	ATriangleActor();
	virtual ~ATriangleActor() override {}


private:
	UTriangleComponent* TriangleComponent = nullptr;
};
