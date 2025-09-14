#pragma once
#include "Core/Public/Object.h"

class UResourceManager : public UObject
{
	DECLARE_SINGLETON(UResourceManager)
public:
	void Initialize();

	void Release();

	TArray<FVertex>* GetVertexData(EPrimitiveType Type);
	ID3D11Buffer* GetVertexbuffer(EPrimitiveType Type);
	uint32 GetNumVertices(EPrimitiveType Type);
	ID3D11VertexShader* GetVertexShader(EShaderType Type);
	ID3D11PixelShader* GetPixelShader(EShaderType Type);
	ID3D11InputLayout* GetIputLayout(EShaderType Type);

private:
	TMap<EPrimitiveType, ID3D11Buffer*> Vertexbuffers;
	TMap<EPrimitiveType, uint32> NumVertices;
	TMap<EPrimitiveType, TArray<FVertex>*> VertexDatas;
	TMap<EShaderType, ID3D11VertexShader*> VertexShaders;
	TMap<EShaderType, ID3D11InputLayout*> InputLayouts;
	TMap<EShaderType, ID3D11PixelShader*> PixelShaders;
};
