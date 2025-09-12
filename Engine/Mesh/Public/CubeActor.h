#pragma once

#include "Mesh/Public/Actor.h"

UCLASS()
class ACubeActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(ACubeActor, AActor)

public:
	ACubeActor();

private:
	UCubeComponent* CubeComponent = nullptr;
};
