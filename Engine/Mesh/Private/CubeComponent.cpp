#include "pch.h"
#include "Mesh/Public/CubeComponent.h"

#include "Manager/Asset/Public/AssetManager.h"

IMPLEMENT_CLASS(UCubeComponent, UPrimitiveComponent)

UCubeComponent::UCubeComponent()
{
	UAssetManager& ResourceManager = UAssetManager::GetInstance();
	Type = EPrimitiveType::Cube;
	Vertices = ResourceManager.GetVertexData(Type);
	Vertexbuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);
	RenderState.CullMode = ECullMode::Back;
	RenderState.FillMode = EFillMode::Solid;
	BoundingBox = &ResourceManager.GetAABB(Type);
}
