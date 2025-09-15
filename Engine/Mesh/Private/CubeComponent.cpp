#include "pch.h"
#include "Mesh/Public/CubeComponent.h"

#include "Manager/Resource/Public/ResourceManager.h"

IMPLEMENT_CLASS(UCubeComponent, UPrimitiveComponent)

UCubeComponent::UCubeComponent()
{
	UResourceManager& ResourceManager = UResourceManager::GetInstance();
	Type = EPrimitiveType::Cube;
	Vertices = ResourceManager.GetVertexData(Type);
	Vertexbuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);
	RenderState.CullMode = ECullMode::Back;
	RenderState.FillMode = EFillMode::Solid;
	BoundingBox = &ResourceManager.GetAABB(Type);
}
