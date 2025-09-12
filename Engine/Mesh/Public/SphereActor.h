#pragma once

#include "Mesh/Public/Actor.h"

UCLASS()
class ASphereActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(ASphereActor, AActor)

public:
	ASphereActor();
private:
	USphereComponent* SphereComponent = nullptr;
};

