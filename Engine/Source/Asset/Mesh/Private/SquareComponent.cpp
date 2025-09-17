#include "pch.h"
#include "Asset/Mesh/Public/SquareComponent.h"

#include "Manager/Asset/Public/AssetManager.h"

IMPLEMENT_CLASS(USquareComponent, UPrimitiveComponent)

USquareComponent::USquareComponent()
{
	UAssetManager& ResourceManager = UAssetManager::GetInstance();
	Type = EPrimitiveType::Square;
	Vertices = ResourceManager.GetVertexData(Type);
	Vertexbuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);
	RenderState.CullMode = ECullMode::None;
	RenderState.FillMode = EFillMode::Solid;
	BoundingBox = &ResourceManager.GetAABB(Type);
}
