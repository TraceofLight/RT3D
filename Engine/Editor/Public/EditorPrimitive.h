#pragma once
#include <d3d11.h>
#include "Global/Vector.h"

struct FEditorPrimitive
{
	ID3D11Buffer* Vertexbuffer;
	ID3D11Buffer* IndexBuffer;
	uint32 NumVertices;
	uint32 NumIndices;
	D3D11_PRIMITIVE_TOPOLOGY Topology;
	FVector4 Color;
	FVector Location;
	FVector Rotation;
	FVector Scale;
	FRenderState RenderState;
	bool bShouldAlwaysVisible = false;
};
