#pragma once
#include "Runtime/CoreUObject/Public/Class.h"
#include "Source/Component/Public/PrimitiveComponent.h"

class UMaterial;

UCLASS();
class UMeshComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UMeshComponent, UPrimitiveComponent)

public:
	virtual void SetMaterial(int32 ElementIndex, UMaterial* Material);
	virtual UMaterial* GetMaterial(int32 ElementIndex) const;

protected:
	TArray<UMaterial*> OverrideMaterials;
};
