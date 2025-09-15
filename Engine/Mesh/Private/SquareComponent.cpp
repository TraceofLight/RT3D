#include "pch.h"
#include "Mesh/Public/SquareComponent.h"

#include "Manager/Resource/Public/ResourceManager.h"

IMPLEMENT_CLASS(USquareComponent, UPrimitiveComponent)

USquareComponent::USquareComponent()
{
	UResourceManager& ResourceManager = UResourceManager::GetInstance();
	Type = EPrimitiveType::Square;
	Vertices = ResourceManager.GetVertexData(Type);
	Vertexbuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);
	RenderState.CullMode = ECullMode::None;
	RenderState.FillMode = EFillMode::Solid;
	BoundingBox = &ResourceManager.GetAABB(Type);
}
