#pragma once
#include "Core/Public/Object.h"
#include "Mesh/Public/AABB.h"

/**
 * @brief 전역의 On-Memory Asset을 관리하는 매니저 클래스
 */
UCLASS()
class UResourceManager
	: public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UResourceManager, UObject)

public:
	void Initialize();
	void Release();

	// Vertex 관련 함수들
	TArray<FVertex>* GetVertexData(EPrimitiveType InType);
	ID3D11Buffer* GetVertexbuffer(EPrimitiveType InType);
	uint32 GetNumVertices(EPrimitiveType InType);

	// Texture 관련 함수들
	ID3D11ShaderResourceView* LoadTexture(const FString& InFilePath);
	ID3D11ShaderResourceView* GetTexture(const FString& InFilePath);
	void ReleaseTexture(const FString& InFilePath);
	bool HasTexture(const FString& InFilePath) const;

	// Create Texture
	static ID3D11ShaderResourceView* CreateTextureFromFile(const FString& InFilePath);
	ID3D11ShaderResourceView* CreateTextureFromMemory(const void* InData, size_t InDataSize);

	// Bounding Box
	const FAABB& GetCubeAABB() const;
	const FAABB& GetSphereAABB() const;

private:
	// Vertex Resource
	TMap<EPrimitiveType, ID3D11Buffer*> Vertexbuffers;
	TMap<EPrimitiveType, uint32> NumVertices;
	TMap<EPrimitiveType, TArray<FVertex>*> VertexDatas;

	// Texture Resource
	TMap<FString, ID3D11ShaderResourceView*> TextureCache;

	// Release Functions
	void ReleaseAllTextures();

	// AABB Resource
	FAABB CubeAABB;
	FAABB SphereAABB;
};
