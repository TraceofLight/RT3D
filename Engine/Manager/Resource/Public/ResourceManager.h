#pragma once
#include "Core/Public/Object.h"

UCLASS()
class UResourceManager
	: public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UResourceManager, UObject)

public:
	void Initialize();
	void Release();

	TArray<FVertex>* GetVertexData(EPrimitiveType Type);
	ID3D11Buffer* GetVertexbuffer(EPrimitiveType Type);
	uint32 GetNumVertices(EPrimitiveType Type);

private:
	TMap<EPrimitiveType, ID3D11Buffer*> Vertexbuffers;
	TMap<EPrimitiveType, uint32> NumVertices;
	TMap<EPrimitiveType, TArray<FVertex>*> VertexDatas;
};
