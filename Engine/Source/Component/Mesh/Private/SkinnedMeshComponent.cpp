#include "pch.h"
#include "Component/Mesh/Public/SkinnedMeshComponent.h"

#include <execution>

#include "Component/Mesh/Public/SkeletalMeshComponent.h"
#include "Runtime/Renderer/Public/RenderResourceFactory.h"

IMPLEMENT_CLASS(USkinnedMeshComponent, UMeshComponent)
USkinnedMeshComponent::USkinnedMeshComponent()
{
	bOwnsBoundingBox = true;
	BoundingBox = new FAABB();
}
USkinnedMeshComponent::~USkinnedMeshComponent()
{
	if (bOwnsBoundingBox && BoundingBox)
	{
		SafeDelete(BoundingBox);
	}
	if (DynamicMeshBuffer != nullptr)
	{
		FRenderResourceFactory::ReleaseDynamicMeshBuffer(DynamicMeshBuffer);
	}
}

void USkinnedMeshComponent::SetSkeletalMesh(USkeletalMesh* InMesh)
{
    SkeletalMesh = InMesh;
    FinalSkinMatrices.Empty();
    bSkinnedVerticesDirty = true;
    if (SkeletalMesh && SkeletalMesh->GetSkeleton())
    {
        FinalSkinMatrices.SetNum(SkeletalMesh->GetSkeleton()->GetNumBones());
        // Initialize with identity skinning (ref pose)
        for (int32 i = 0; i < FinalSkinMatrices.Num(); ++i)
        {
            FinalSkinMatrices[i] = FMatrix::Identity();
        }

        // USkeletalMeshComponent인 경우 UseReferencePose 호출
        if (USkeletalMeshComponent* SkelComp = Cast<USkeletalMeshComponent>(this))
        {
            SkelComp->UseReferencePose();
        }

		RenderState.CullMode = ECullMode::Back;
		RenderState.FillMode = EFillMode::Solid;
		if (DynamicMeshBuffer != nullptr)
		{
			FRenderResourceFactory::ReleaseDynamicMeshBuffer(DynamicMeshBuffer);
		}
		DynamicMeshBuffer = FRenderResourceFactory::CreateDynamicMeshBuffer(SkeletalMesh->GetVertices(), SkeletalMesh->GetIndices());
    }
}

void USkinnedMeshComponent::SetSkinMatrices(const TArray<FMatrix>& InMatrices)
{
    FinalSkinMatrices = InMatrices;
    bSkinnedVerticesDirty = true;
}

void USkinnedMeshComponent::SetSkinMatrices(const FMatrix* InMatrices, int32 Count)
{
    if (!InMatrices || Count <= 0)
    {
	    FinalSkinMatrices.Empty(); return;
    }
    FinalSkinMatrices.SetNum(Count);
    for (int32 i = 0; i < Count; ++i)
    {
	    FinalSkinMatrices[i] = InMatrices[i];
    }
    bSkinnedVerticesDirty = true;
}

FVector USkinnedMeshComponent::SkinTangent(const FSkeletalVertex& V, const FSkeletalMeshSection& Sec, const TArray<FMatrix>& SkinMats)
{
	FVector Out = FVector::ZeroVector();

	for (int k = 0; k < 4; ++k)
	{
		const float w = V.Skin.BoneWeights[k];
		if (w <= 0.0f)
		{
			continue;
		}

		const uint16 secIdx = V.Skin.BoneIndices[k];
		if (secIdx >= Sec.BoneMap.Num())
		{
			continue;
		}
		const uint16 skelIdx = Sec.BoneMap[secIdx];
		if (skelIdx >= SkinMats.Num())
		{
			continue;
		}
		const FMatrix& M = SkinMats[skelIdx];
		Out += M.TransformVector( FVector(V.Vertex.Tangent.X, V.Vertex.Tangent.Y, V.Vertex.Tangent.Z) ) * w;
	}
	Out.Normalize();
	return Out;
}

FVector USkinnedMeshComponent::SkinPosition(const FSkeletalVertex& V, const FSkeletalMeshSection& Sec, const TArray<FMatrix>& SkinMats)
{
    FVector Out = FVector::ZeroVector();
    for (int k = 0; k < 4; ++k)
    {
        const float w = V.Skin.BoneWeights[k];
        if (w <= 0.0f)
        {
	        continue;
        }
        const uint16 secIdx = V.Skin.BoneIndices[k];
        if (secIdx >= Sec.BoneMap.Num())
        {
	        continue;
        }
        const uint16 skelIdx = Sec.BoneMap[secIdx];
        if (skelIdx >= SkinMats.Num())
        {
	        continue;
        }
        const FMatrix& M = SkinMats[skelIdx];
        Out += M.TransformPosition(V.Vertex.Position) * w;
    }
    return Out;
}

FVector USkinnedMeshComponent::SkinNormal(const FSkeletalVertex& V, const FSkeletalMeshSection& Sec, const TArray<FMatrix>& SkinMats)
{
    FVector Out = FVector::ZeroVector();
    for (int k = 0; k < 4; ++k)
    {
        const float w = V.Skin.BoneWeights[k];
        if (w <= 0.0f)
        {
	        continue;
        }
        const uint16 secIdx = V.Skin.BoneIndices[k];
        if (secIdx >= Sec.BoneMap.Num())
        {
	        continue;
        }
        const uint16 skelIdx = Sec.BoneMap[secIdx];
        if (skelIdx >= SkinMats.Num())
        {
	        continue;
        }
        const FMatrix& M = SkinMats[skelIdx];
        Out += M.TransformVector(V.Vertex.Normal) * w; // ignore translation
    }
    Out.Normalize();
    return Out;
}

const TArray<FNormalVertex>& USkinnedMeshComponent::GetSkinnedVertices() const
{
    if (bSkinnedVerticesDirty)
    {
        UpdateSkinnedVerticesCache();
    }
    return CachedSkinnedVertices;
}

const TArray<uint32>& USkinnedMeshComponent::GetSkinnedIndices() const
{
    if (bSkinnedVerticesDirty)
    {
        UpdateSkinnedVerticesCache();
    }
    return CachedSkinnedIndices;
}

void USkinnedMeshComponent::UpdateSkinnedVerticesCache() const
{
    if (!SkeletalMesh || !SkeletalMesh->IsValid())
    {
        CachedSkinnedVertices.Empty();
        CachedSkinnedIndices.Empty();
        bSkinnedVerticesDirty = false;
        return;
    }

    FSkeletalMesh* MeshData = SkeletalMesh->GetSkeletalMeshAsset();
    if (!MeshData || MeshData->Sections.IsEmpty())
    {
        CachedSkinnedVertices.Empty();
        CachedSkinnedIndices.Empty();
        bSkinnedVerticesDirty = false;
        return;
    }

    if (FinalSkinMatrices.IsEmpty())
    {
        CachedSkinnedVertices.Empty();
        CachedSkinnedIndices.Empty();
        bSkinnedVerticesDirty = false;
        return;
    }

    // 전체 Vertex/Index 개수 계산
    int32 TotalVertexCount = 0;
    int32 TotalIndexCount = 0;
    for (const FSkeletalMeshSection& Section : MeshData->Sections)
    {
        TotalVertexCount += Section.Vertices.Num();
        TotalIndexCount += Section.Indices.Num();
    }

	NumVertices = TotalVertexCount;
	NumIndices = TotalIndexCount;

    // 캐시 배열 크기 미리 할당
    CachedSkinnedVertices.SetNum(TotalVertexCount);
    CachedSkinnedIndices.SetNum(TotalIndexCount);

    // Section별 병렬 처리
    int32 CurrentVertexOffset = 0;
    int32 CurrentIndexOffset = 0;

    for (const FSkeletalMeshSection& Section : MeshData->Sections)
    {
        const int32 SectionVertexCount = Section.Vertices.Num();
        const int32 SectionIndexCount = Section.Indices.Num();

        if (SectionVertexCount == 0 || SectionIndexCount == 0)
        {
            continue;
        }

        // Vertex Skinning 병렬 처리
        const int32 VertexOffset = CurrentVertexOffset;

        std::for_each(std::execution::par,
            Section.Vertices.begin(), Section.Vertices.end(),
            [&, VertexOffset](const FSkeletalVertex& SkelVert)
            {
                int32 LocalIndex = static_cast<int32>(&SkelVert - &Section.Vertices[0]);
                int32 GlobalIndex = VertexOffset + LocalIndex;

                FNormalVertex& OutVert = CachedSkinnedVertices[GlobalIndex];
                OutVert.Position = SkinPosition(SkelVert, Section, FinalSkinMatrices);
                OutVert.Normal = SkinNormal(SkelVert, Section, FinalSkinMatrices);

				FVector SkinnedTangent = SkinTangent(SkelVert, Section, FinalSkinMatrices);
				OutVert.Tangent = FVector4(SkinnedTangent, SkelVert.Vertex.Tangent.W);

                OutVert.Color = SkelVert.Vertex.Color;
                OutVert.TexCoord = SkelVert.Vertex.TexCoord;
            });

        // Index 복사 (vertex offset 적용)
        for (int32 i = 0; i < SectionIndexCount; ++i)
        {
            CachedSkinnedIndices[CurrentIndexOffset + i] = Section.Indices[i] + CurrentVertexOffset;
        }

        CurrentVertexOffset += SectionVertexCount;
        CurrentIndexOffset += SectionIndexCount;

		DynamicMeshBuffer->UpdateVertexData(CachedSkinnedVertices);
		DynamicMeshBuffer->UpdateIndexData(CachedSkinnedIndices);
    }

	if (CachedSkinnedVertices.Num() > 0)
	{
		FVector Min, Max;
		Min = CachedSkinnedVertices[0].Position;
		Max = CachedSkinnedVertices[0].Position;

		for (auto& v : CachedSkinnedVertices)
		{
			Min.X = std::min(Min.X, v.Position.X);
			Min.Y = std::min(Min.Y, v.Position.Y);
			Min.Z = std::min(Min.Z, v.Position.Z);

			Max.X = std::max(Max.X, v.Position.X);
			Max.Y = std::max(Max.Y, v.Position.Y);
			Max.Z = std::max(Max.Z, v.Position.Z);
		}

		FAABB* AABB = static_cast<FAABB*>(BoundingBox);
		AABB->Min = Min;
		AABB->Max = Max;
		bIsAABBCacheDirty = true;
	}


    bSkinnedVerticesDirty = false;
}
const TArray<FNormalVertex>* USkinnedMeshComponent::GetVerticesData() const
{
	GetSkinnedVertices();
	return &CachedSkinnedVertices;
}
const TArray<uint32>* USkinnedMeshComponent::GetIndicesData() const
{
	//인덱스는 왜 재세팅 되는거지?
	GetSkinnedIndices();
	return &CachedSkinnedIndices;
}

ID3D11Buffer* USkinnedMeshComponent::GetVertexBuffer() const
{
	return DynamicMeshBuffer == nullptr ? nullptr : DynamicMeshBuffer->GetVB();
}
ID3D11Buffer* USkinnedMeshComponent::GetIndexBuffer() const
{
	return DynamicMeshBuffer == nullptr ? nullptr : DynamicMeshBuffer->GetIB();
}
UMaterial* USkinnedMeshComponent::GetMaterial(int32 ElementIndex) const
{
	if (ElementIndex >= 0 && ElementIndex < OverrideMaterials.Num())
	{
		if (OverrideMaterials[ElementIndex])
		{
			return OverrideMaterials[ElementIndex];
		}
	}

	if (SkeletalMesh)
	{
		return SkeletalMesh->GetMaterial(ElementIndex);
	}

	return nullptr;
}
