#include "pch.h"
#include "Mesh/Public/SphereComponent.h"

#include "Manager/Resource/Public/ResourceManager.h"

IMPLEMENT_CLASS(USphereComponent, UPrimitiveComponent)

USphereComponent::USphereComponent()
{
	UResourceManager& ResourceManager = UResourceManager::GetInstance();
	Type = EPrimitiveType::Sphere;
	Vertices = ResourceManager.GetVertexData(Type);
	Vertexbuffer = ResourceManager.GetVertexbuffer(Type);
	NumVertices = ResourceManager.GetNumVertices(Type);
	RenderState.CullMode = ECullMode::Back;
	RenderState.FillMode = EFillMode::Solid;
}
