#pragma once
#include "Core/Public/Object.h"
#include "Mesh/Public/AABB.h"

class UResourceManager : public UObject
{
	DECLARE_SINGLETON(UResourceManager)
public:
	void Initialize();
	void Release();

	TArray<FVertex>* GetVertexData(EPrimitiveType Type);
	ID3D11Buffer* GetVertexbuffer(EPrimitiveType Type);
	uint32 GetNumVertices(EPrimitiveType Type);

	const FAABB& GetCubeAABB() const;
	const FAABB& GetSphereAABB() const;

private:
	TMap<EPrimitiveType, ID3D11Buffer*> Vertexbuffers;
	TMap<EPrimitiveType, uint32> NumVertices;
	TMap<EPrimitiveType, TArray<FVertex>*> VertexDatas;

	FAABB CubeAABB;
	FAABB SphereAABB;
};
