#include "pch.h"
#include "Component/Mesh/Public/SkinnedMeshComponent.h"

#include <execution>

#include "Component/Mesh/Public/SkeletalMeshComponent.h"

IMPLEMENT_CLASS(USkinnedMeshComponent, UMeshComponent)

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
                OutVert.Color = SkelVert.Vertex.Color;
                OutVert.TexCoord = SkelVert.Vertex.TexCoord;
                OutVert.Tangent = SkelVert.Vertex.Tangent;
            });

        // Index 복사 (vertex offset 적용)
        for (int32 i = 0; i < SectionIndexCount; ++i)
        {
            CachedSkinnedIndices[CurrentIndexOffset + i] = Section.Indices[i] + CurrentVertexOffset;
        }

        CurrentVertexOffset += SectionVertexCount;
        CurrentIndexOffset += SectionIndexCount;
    }

    bSkinnedVerticesDirty = false;
}
