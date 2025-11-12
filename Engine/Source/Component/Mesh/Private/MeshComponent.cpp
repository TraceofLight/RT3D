#include "pch.h"
#include "Runtime/CoreUObject/Public/Class.h"       // UObject 기반 클래스 및 매크로
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Mesh/Public/MeshComponent.h"
#include "Texture/Public/Material.h"

IMPLEMENT_ABSTRACT_CLASS(UMeshComponent, UPrimitiveComponent)

void UMeshComponent::SetMaterial(int32 ElementIndex, UMaterial* Material)
{
	if (ElementIndex < 0)
	{
		return;
	}

	if (OverrideMaterials.Num() <= ElementIndex)
	{
		OverrideMaterials.SetNum(ElementIndex + 1);
	}
	
	OverrideMaterials[ElementIndex] = Material;
}

UMaterial* UMeshComponent::GetMaterial(int32 ElementIndex) const
{
	if (ElementIndex >= 0 && ElementIndex < OverrideMaterials.Num())
	{
		return OverrideMaterials[ElementIndex];
	}
	return nullptr;
}
