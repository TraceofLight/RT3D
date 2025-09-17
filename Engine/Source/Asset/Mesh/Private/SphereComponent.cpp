#include "pch.h"
#include "Asset/Mesh/Public/SphereComponent.h"

#include "Asset/Mesh/Public/PrimitiveComponent.h"
#include "Manager/Asset/Public/AssetManager.h"

IMPLEMENT_CLASS(USphereComponent, UPrimitiveComponent)

USphereComponent::USphereComponent()
{
	UAssetManager& ResourceManager = UAssetManager::GetInstance();
	Type = EPrimitiveType::Sphere;
	Vertices = ResourceManager.GetVertexData(Type);
	Vertexbuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);
	RenderState.CullMode = ECullMode::Back;
	RenderState.FillMode = EFillMode::Solid;
	BoundingBox = &ResourceManager.GetAABB(Type);
}
