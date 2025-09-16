#include "pch.h"
#include "Mesh/Public/TriangleComponent.h"

#include "Manager/Asset/Public/AssetManager.h"

IMPLEMENT_CLASS(UTriangleComponent, UPrimitiveComponent)

UTriangleComponent::UTriangleComponent()
{
	UAssetManager& ResourceManager = UAssetManager::GetInstance();
	Type = EPrimitiveType::Triangle;
	Vertices = ResourceManager.GetVertexData(Type);
	Vertexbuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);
	RenderState.CullMode = ECullMode::None;
	RenderState.FillMode = EFillMode::Solid;
	BoundingBox = &ResourceManager.GetAABB(Type);
}
