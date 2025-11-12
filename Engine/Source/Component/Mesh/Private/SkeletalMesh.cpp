#include "pch.h"
#include "Component/Mesh/Public/SkeletalMesh.h"
#include "Texture/Public/Material.h"

IMPLEMENT_CLASS(USkeletalMesh, UObject)

void FSkeleton::BuildRefPoseGlobal()
{
	const int32 NumBones = BoneNames.Num();
	if (NumBones <= 0)
	{
		RefPoseGlobal = {};
		InvRefPoseGlobal = {};
		return;
	}
	RefPoseGlobal.SetNum(NumBones);
	InvRefPoseGlobal.SetNum(NumBones);

    if (RefPoseLocal.Num() != NumBones)
    {
        RefPoseLocal.SetNum(NumBones);
        for (int32 i = 0; i < NumBones; ++i)
        {
            RefPoseLocal[i] = {};
        }
    }

    for (int32 i = 0; i < NumBones; ++i)
    {
        const FTransform& L = RefPoseLocal[i];
        const FQuat LocalRot = L.Rotation;
        const FMatrix LocalM = FMatrix::GetModelMatrix(L.Location, LocalRot, L.Scale);

        const int32 Parent = (i < Parents.Num()) ? Parents[i] : -1;
        if (Parent >= 0)
        {
            RefPoseGlobal[i] = LocalM * RefPoseGlobal[Parent];
        }
        else
        {
            RefPoseGlobal[i] = LocalM; // 루트일 때
        }

        InvRefPoseGlobal[i] = RefPoseGlobal[i].Inverse();
    }
}

int32 FSkeleton::FindBoneIndex(const FName& BoneName) const
{
	for (int32 i = 0; i < BoneNames.Num(); i++)
	{
		if (BoneNames[i] == BoneName)
		{
			return i;
		}
	}
	return -1;
}

USkeletalMesh::USkeletalMesh()
	: SkeletalMesh(nullptr)
{
}

USkeletalMesh::~USkeletalMesh()
{
	if (SkeletalMesh && SkeletalMesh->Skeleton)
	{
		delete SkeletalMesh->Skeleton;
		SkeletalMesh->Skeleton = nullptr;
	}

	if (SkeletalMesh)
	{
		delete SkeletalMesh;
		SkeletalMesh = nullptr;
	}
}

void USkeletalMesh::SetSkeletalMeshAsset(FSkeletalMesh* InSkeletalMeshAsset)
{
	SkeletalMesh = InSkeletalMeshAsset;
}

UMaterial* USkeletalMesh::GetMaterial(int32 MaterialIndex) const
{
	if (MaterialIndex >= 0 && MaterialIndex < Materials.Num())
	{
		return Materials[MaterialIndex];
	}
	return nullptr;
}

void USkeletalMesh::SetMaterial(int32 MaterialIndex, UMaterial* Material)
{
	// Materials 배열 크기 자동 조정
	if (MaterialIndex >= Materials.Num())
	{
		Materials.SetNum(MaterialIndex + 1);
	}

	Materials[MaterialIndex] = Material;
}

int32 USkeletalMesh::GetNumMaterials() const
{
	if (SkeletalMesh)
	{
		return static_cast<int32>(SkeletalMesh->MaterialInfo.Num());
	}
	return 0;
}

const TArray<FNormalVertex> USkeletalMesh::GetVertices() const
{
	TArray<FNormalVertex> Vertices;
	for (FSkeletalMeshSection& Section : GetSkeletalMeshAsset()->Sections)
	{
		for (FSkeletalVertex& SkeletalVertex : Section.Vertices)
		{
			Vertices.Add(SkeletalVertex.Vertex);
		}
	}

	return Vertices;
}

TArray<FNormalVertex> USkeletalMesh::GetVertices()
{
	TArray<FNormalVertex> Vertices;
	for (FSkeletalMeshSection& Section : GetSkeletalMeshAsset()->Sections)
	{
		for (FSkeletalVertex& SkeletalVertex : Section.Vertices)
		{
			Vertices.Add(SkeletalVertex.Vertex);
		}
	}

	return Vertices;
}

const TArray<uint32> USkeletalMesh::GetIndices() const
{
	static TArray<uint32> EmptyIndices;
	TArray<uint32> Indices;
	for (FSkeletalMeshSection& Section : GetSkeletalMeshAsset()->Sections)
	{
		Indices.Append(&Section.Indices[0], Section.Indices.Num());
	}

	return Indices;
}
