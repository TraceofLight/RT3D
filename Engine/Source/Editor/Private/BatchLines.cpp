#include "pch.h"
#include "Editor/Public/BatchLines.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Runtime/Renderer/Public/RenderResourceFactory.h"
#include "Global/Octree.h"
#include "Component/Public/DecalSpotLightComponent.h"

#include "Level/Public/Level.h"
#include "Physics/Public/OBB.h"

static inline void AddLine(TArray<FVector>& V, TArray<int32>& I,
						   const FVector& a, const FVector& b)
{
	const int ia = V.Num(); V.Add(a);
	const int ib = V.Num(); V.Add(b);
	I.Add(ia); I.Add(ib);
}

static inline void AddRing(TArray<FVector>& V, TArray<int32>& I,
						   const FVector& c, const FVector& ax, const FVector& ay,
						   float r, int seg = 16)
{
	if (r <= 0.f) return;
	const float dth = 2.f * PI / float(seg);
	FVector p0 = c + (ax * r);
	for (int s=1; s<=seg; ++s) {
		float th = dth * s;
		FVector p = c + (ax * (cosf(th)*r)) + (ay * (sinf(th)*r));
		AddLine(V,I, p0, p);
		p0 = p;
	}
}

static inline void OrthonormalBasis(const FVector& Direction, FVector& U, FVector& V)
{
	const FVector Up = (fabsf(Direction.Z) < 0.99f) ? FVector(0,0,1) : FVector(0,1,0);
	U = Up.Cross(Direction); U.Normalize();
	V = Direction.Cross(U); V.Normalize();
}

IMPLEMENT_CLASS(UBatchLines, UObject)

UBatchLines::UBatchLines() : Grid(), BoundingBoxLines()
{
	Vertices.Reserve(Grid.GetNumVertices() + BoundingBoxLines.GetNumVertices());
	Vertices.SetNum(Grid.GetNumVertices() + BoundingBoxLines.GetNumVertices());

	Grid.MergeVerticesAt(Vertices, 0);
	BoundingBoxLines.MergeVerticesAt(Vertices, Grid.GetNumVertices());

	SetIndices();

	ID3D11VertexShader* VertexShader;
	ID3D11InputLayout* InputLayout;
	TArray<D3D11_INPUT_ELEMENT_DESC> Layout = { {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0} };
	FRenderResourceFactory::CreateVertexShaderAndInputLayout(L"Asset/Shader/BatchLineShader.hlsl", Layout, &VertexShader, &InputLayout);
	ID3D11PixelShader* PixelShader;
	FRenderResourceFactory::CreatePixelShader(L"Asset/Shader/BatchLineShader.hlsl", &PixelShader);

	Primitive.VertexShader = VertexShader;
	Primitive.InputLayout = InputLayout;
	Primitive.PixelShader = PixelShader;
	Primitive.NumVertices = static_cast<uint32>(Vertices.Num());
	Primitive.NumIndices = static_cast<uint32>(Indices.Num());
	Primitive.VertexBuffer = FRenderResourceFactory::CreateVertexBuffer(Vertices.GetData(), Primitive.NumVertices * sizeof(FVector), true);
	Primitive.IndexBuffer = FRenderResourceFactory::CreateIndexBuffer(Indices.GetData(), Primitive.NumIndices * sizeof(uint32));	Primitive.Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
}

UBatchLines::~UBatchLines()
{
	SafeRelease(Primitive.InputLayout);
	SafeRelease(Primitive.VertexShader);
	SafeRelease(Primitive.PixelShader);
	SafeRelease(Primitive.VertexBuffer);
	SafeRelease(Primitive.IndexBuffer);
}

void UBatchLines::UpdateUGridVertices(const float newCellSize)
{
	if (newCellSize == Grid.GetCellSize()) { return; }
	Grid.UpdateVerticesBy(newCellSize);
	bChangedVertices = true;
}

void UBatchLines::UpdateBoundingBoxVertices(const IBoundingVolume* NewBoundingVolume)
{
	if (NewBoundingVolume && NewBoundingVolume->GetType() != EBoundingVolumeType::SpotLight)
	{
		bRenderSpotLight = false;
	}

	BoundingBoxLines.UpdateVertices(NewBoundingVolume);
	bChangedVertices = true;
}


void UBatchLines::UpdateOctreeVertices(const FOctree* InOctree)
{
	OctreeLines.Empty();
	if (InOctree)
	{
		TraverseOctree(InOctree);
	}
	bChangedVertices = true;
}

void UBatchLines::UpdateDecalSpotLightVertices(UDecalSpotLightComponent* SpotLightComponent)
{
	if (!SpotLightComponent)
	{
		bRenderSpotLight = false;
		return;
	}

	// GetBoundingBox updates the underlying volume, so we need non-const access.
	const FSpotLightOBB* SpotLightBounding = SpotLightComponent ? SpotLightComponent->GetSpotLightBoundingBox() : nullptr;
	if (!SpotLightBounding)
	{
		bRenderSpotLight = false;
		return;
	}

	SpotLightLines.UpdateVertices(SpotLightBounding);
	bRenderSpotLight = true;
	bChangedVertices = true;
}

void UBatchLines::UpdateConeVertices(const FVector& InCenter, float InGeneratingLineLength
	, float InOuterHalfAngleRad, float InInnerHalfAngleRad, FQuat InRotation)
{
	// SpotLight는 scale의 영향을 받지 않으므로 world transformation matrix 직접 계산
	FMatrix TranslationMat = FMatrix::TranslationMatrix(InCenter);
	FMatrix ScaleMat = FMatrix::ScalingMatrix(FVector(InGeneratingLineLength, InGeneratingLineLength, InGeneratingLineLength));
	FMatrix RotationMat = InRotation.ToRotationMatrix();

	if (InGeneratingLineLength <= 0.0f)
	{
		bRenderSpotLight = false;
		return;
	}

	constexpr uint32 NumSegments = 40;
	const float CosOuter = cosf(InOuterHalfAngleRad);
	const float SinOuter = sinf(InOuterHalfAngleRad);
	const float CosInner = InInnerHalfAngleRad > MATH_EPSILON ? cosf(InInnerHalfAngleRad) : 0.0f;
	const float SinInner = InInnerHalfAngleRad > MATH_EPSILON ? sinf(InInnerHalfAngleRad) : 0.0f;

	constexpr float BaseSegmentAngle = (2.0f * PI) / static_cast<float>(NumSegments);
	const float ArcSegmentAngle = 2.0f * InOuterHalfAngleRad / static_cast<float>(NumSegments);
	const bool bHasInnerCone = InInnerHalfAngleRad > MATH_EPSILON;

	TArray<FVector> LocalVertices;
	LocalVertices.Reserve(1 + NumSegments + (bHasInnerCone ? NumSegments : 0));

	// x, y 평면 위 호 버텍스
	for (uint32 Segment = 0; Segment <= NumSegments; ++Segment)
	{
		const float Angle = -InOuterHalfAngleRad + ArcSegmentAngle * static_cast<float>(Segment);

		LocalVertices.Emplace(cosf(Angle), sinf(Angle), 0.0f);
	}

	// z, x 평면 위 호 버텍스
	for (uint32 Segment = 0; Segment <= NumSegments; ++Segment)
	{
		const float Angle = -InOuterHalfAngleRad + ArcSegmentAngle * static_cast<float>(Segment);

		LocalVertices.Emplace(cosf(Angle), 0.0f, sinf(Angle));
	}

	LocalVertices.Emplace(0.0f, 0.0f, 0.0f); // Apex

	// 외곽 원 버텍스
	for (uint32 Segment = 0; Segment < NumSegments; ++Segment)
	{
		const float Angle = BaseSegmentAngle * static_cast<float>(Segment);
		const float CosValue = cosf(Angle);
		const float SinValue = sinf(Angle);

		LocalVertices.Emplace(CosOuter, SinOuter * CosValue, SinOuter * SinValue);
	}

	// 내곽 원 버텍스 (있을 경우)
	if (bHasInnerCone)
	{
		for (uint32 Segment = 0; Segment < NumSegments; ++Segment)
		{
			const float Angle = BaseSegmentAngle * static_cast<float>(Segment);
			const float CosValue = cosf(Angle);
			const float SinValue = sinf(Angle);

			LocalVertices.Emplace(CosInner, SinInner * CosValue, SinInner * SinValue);
		}
	}

	FMatrix WorldMatrix = ScaleMat;
	WorldMatrix *= RotationMat;
	WorldMatrix *= TranslationMat;

	TArray<FVector> WorldVertices(LocalVertices.Num());
	for (int32 Index = 0; Index < LocalVertices.Num(); ++Index)
	{
		WorldVertices[Index] = WorldMatrix.TransformPosition(LocalVertices[Index]);
	}

	SpotLightLines.UpdateSpotLightVertices(WorldVertices);
	bRenderSpotLight = true;
	bChangedVertices = true;
}

void UBatchLines::UpdateSkeletonVertices(const FSkeleton* Skeleton,
                                         const TArray<FMatrix>& GlobalPose,
                                         const FMatrix& ComponentToWorld,
                                         int32 SelectedBone,
                                         float JointRadius,
                                         float WidthScale,
                                         float BaseBias)
{
    BoneLines.Reset();
    bRenderBones = false;
    if (!Skeleton || GlobalPose.IsEmpty()) return;

    auto XformPos = [](const FMatrix& M)->FVector {
        // 행렬에서 위치를 얻는 가장 안전한 방법: 원점 변환
        return M.TransformPosition(FVector(0,0,0));
    };

    const int Num = Skeleton->Parents.Num();
    if (Num <= 0) return;

    // 월드 보정된 본 위치(컴포넌트->월드)
    TArray<FVector> WorldPos; WorldPos.SetNum(Num);
    for (int i=0; i<Num; ++i) {
        FVector Position = XformPos(GlobalPose[i]);
        WorldPos[i] = ComponentToWorld.TransformPosition(Position);
    }

    // 조인트: 3개 링
    for (int i=0; i<Num; ++i) {
        const bool bSelected = (i == SelectedBone);
        const float Radius = JointRadius * (bSelected ? 1.35f : 1.0f);
        const FVector Center = WorldPos[i];

        // 기준축: 부모가 있으면 본 방향, 없으면 세계축
        FVector Direction(1,0,0); // 임시, X forward
        if (Skeleton->Parents[i] >= 0) {
            const FVector p0 = Center;
            const FVector pp = WorldPos[Skeleton->Parents[i]];
            Direction = (p0 - pp).GetSafeNormal();
        }
        FVector U,V; OrthonormalBasis(Direction, U, V); // Direction에 수직이면서, 서로 수직인 벡터 -> 직교 좌표축

        // 세 개 평면(U V, V * Direction, Direction * U)으로 링
        AddRing(BoneLines.Vertices, BoneLines.Indices, Center, U, V, Radius, 14);
        AddRing(BoneLines.Vertices, BoneLines.Indices, Center, V, Direction, Radius, 14);
        AddRing(BoneLines.Vertices, BoneLines.Indices, Center, Direction, U, Radius, 14);
    }

    // 본(부모 -> 자식)
    auto AddBoneDiamond = [&](int Parent, int Child, bool bSelected)
    {
        const FVector P = WorldPos[Parent];
        const FVector C = WorldPos[Child];
        FVector Direction = (C - P);
        const float L = Direction.Length();
        if (L < 1e-4f) return;
        Direction *= (1.0f / L);

        FVector U,V; OrthonormalBasis(Direction, U, V);

        const float Width = max(0.002f, L * WidthScale * (bSelected ? 1.5f : 1.0f));  // 두께
        // base를 회전축(여기선 parent 조인트) 쪽으로 편향
        const float t = clamp(BaseBias, 0.05f, 0.95f); // 0에 가까울수록 parent 쪽
        const FVector baseCenter = P + Direction * (L * t);

        // 정사각 코너(대칭)
        const FVector c0 = baseCenter + (U * Width);
        const FVector c1 = baseCenter + (V * Width);
        const FVector c2 = baseCenter - (U * Width);
        const FVector c3 = baseCenter - (V * Width);

        // 두 개의 뾰족한 끝(쌍뿔) : 본의 양 끝
        const FVector tip0 = P;
        const FVector tip1 = C;

        // 사각 테두리
        AddLine(BoneLines.Vertices, BoneLines.Indices, c0, c1);
        AddLine(BoneLines.Vertices, BoneLines.Indices, c1, c2);
        AddLine(BoneLines.Vertices, BoneLines.Indices, c2, c3);
        AddLine(BoneLines.Vertices, BoneLines.Indices, c3, c0);

        // tip0/1에서 각 코너로
        AddLine(BoneLines.Vertices, BoneLines.Indices, tip0, c0);
        AddLine(BoneLines.Vertices, BoneLines.Indices, tip0, c1);
        AddLine(BoneLines.Vertices, BoneLines.Indices, tip0, c2);
        AddLine(BoneLines.Vertices, BoneLines.Indices, tip0, c3);

        AddLine(BoneLines.Vertices, BoneLines.Indices, tip1, c0);
        AddLine(BoneLines.Vertices, BoneLines.Indices, tip1, c1);
        AddLine(BoneLines.Vertices, BoneLines.Indices, tip1, c2);
        AddLine(BoneLines.Vertices, BoneLines.Indices, tip1, c3);
    };

    for (int i=0;i<Num;++i) {
        for (int Child : Skeleton->Childs[i]) {
            const bool bSelected = (SelectedBone == i) || (SelectedBone == Child);
            AddBoneDiamond(i, Child, bSelected);
        }
    }

    bRenderBones = (BoneLines.GetNumVertices() > 0);
    bChangedVertices = true;
}


void UBatchLines::TraverseOctree(const FOctree* InNode)
{
	if (!InNode) { return; }

	UBoundingBoxLines BoxLines;
	BoxLines.UpdateVertices(&InNode->GetBoundingBox());
	OctreeLines.Add(BoxLines);

	if (!InNode->IsLeafNode())
	{
		for (const auto& Child : InNode->GetChildren())
		{
			TraverseOctree(Child);
		}
	}
}

void UBatchLines::UpdateVertexBuffer()
{
	if (bChangedVertices)
	{
		uint32 NumGridVertices = Grid.GetNumVertices();
		uint32 NumBoxVertices = BoundingBoxLines.GetNumVertices();
		uint32 NumSpotLightVertices = bRenderSpotLight ? SpotLightLines.GetNumVertices() : 0;
		uint32 NumBoneVertices = bRenderBones ? BoneLines.GetNumVertices() : 0;
		uint32 NumOctreeVertices = 0;
		for (const auto& Line : OctreeLines)
		{
			NumOctreeVertices += Line.GetNumVertices();
		}

		Vertices.SetNum(NumGridVertices + NumBoxVertices + NumSpotLightVertices + NumOctreeVertices + NumBoneVertices);

		Grid.MergeVerticesAt(Vertices, 0);
		BoundingBoxLines.MergeVerticesAt(Vertices, NumGridVertices);

		uint32 CurrentOffset = NumGridVertices + NumBoxVertices;
		if (bRenderSpotLight)
		{
			SpotLightLines.MergeVerticesAt(Vertices, CurrentOffset);
			CurrentOffset += SpotLightLines.GetNumVertices();
		}

		if (bRenderBones) {
			BoneLines.MergeVerticesAt(Vertices, CurrentOffset);
			CurrentOffset += NumBoneVertices;
		}

		for (auto& Line : OctreeLines)
		{
			Line.MergeVerticesAt(Vertices, CurrentOffset);
			CurrentOffset += Line.GetNumVertices();
		}

		SetIndices();

		Primitive.NumVertices = static_cast<uint32>(Vertices.Num());
		Primitive.NumIndices = static_cast<uint32>(Indices.Num());

		SafeRelease(Primitive.VertexBuffer);
		SafeRelease(Primitive.IndexBuffer);

		Primitive.VertexBuffer = FRenderResourceFactory::CreateVertexBuffer(Vertices.GetData(), Primitive.NumVertices * sizeof(FVector), true);
		Primitive.IndexBuffer = FRenderResourceFactory::CreateIndexBuffer(Indices.GetData(), Primitive.NumIndices * sizeof(uint32));
	}
	bChangedVertices = false;
}

void UBatchLines::Render()
{
	// Grid + Light Lines
	RenderGridAndLightLines();

	// AABB
	RenderBoundingBox();

	// Octree
	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (EditorWorld && EditorWorld->GetLevel())
	{
		uint64 ShowFlags = EditorWorld->GetLevel()->GetShowFlags();
		if (ShowFlags & EEngineShowFlags::SF_Octree)
		{
			RenderOctree();
		}
	}
}

void UBatchLines::RenderGridAndLightLines()
{
	// Grid + Light Lines 렌더링 (항상 표시)
	URenderer& Renderer = URenderer::GetInstance();

	const uint32 NumGridVertices = Grid.GetNumVertices();
	const uint32 NumGridIndices = NumGridVertices; // Grid는 인덱스가 순차적

	const EBoundingVolumeType BoundingType = BoundingBoxLines.GetCurrentType();
	const uint32 NumBoundingIndices = BoundingBoxLines.GetNumIndices(BoundingType);

	uint32 NumSpotLightIndices = 0;
	if (bRenderSpotLight)
	{
		const EBoundingVolumeType SpotLightType = SpotLightLines.GetCurrentType();
		NumSpotLightIndices = SpotLightLines.GetNumIndices(SpotLightType);
	}

	// Grid 렌더링
	if (NumGridIndices > 0)
	{
		Renderer.RenderEditorPrimitiveIndexed(
			Primitive,
			Primitive.RenderState,
			sizeof(FVector),
			sizeof(uint32),
			0,
			NumGridIndices
		);
	}

	// SpotLight 렌더링
	const uint32 SpotLightStartIndex = NumGridIndices + NumBoundingIndices;
	if (NumSpotLightIndices > 0)
	{
		Renderer.RenderEditorPrimitiveIndexed(
			Primitive,
			Primitive.RenderState,
			sizeof(FVector),
			sizeof(uint32),
			SpotLightStartIndex,
			NumSpotLightIndices
		);
	}
}

void UBatchLines::RenderBoundingBox()
{
	// AABB 렌더링 (선택 시에만 데이터 있음)
	URenderer& Renderer = URenderer::GetInstance();

	const uint32 NumGridVertices = Grid.GetNumVertices();
	const uint32 NumGridIndices = NumGridVertices;

	const EBoundingVolumeType BoundingType = BoundingBoxLines.GetCurrentType();
	const uint32 NumBoundingIndices = BoundingBoxLines.GetNumIndices(BoundingType);

	if (NumBoundingIndices > 0)
	{
		Renderer.RenderEditorPrimitiveIndexed(
			Primitive,
			Primitive.RenderState,
			sizeof(FVector),
			sizeof(uint32),
			NumGridIndices,
			NumBoundingIndices
		);
	}
}

void UBatchLines::RenderOctree()
{
	// Octree 렌더링 (SF_Octree 플래그로 제어)
	URenderer& Renderer = URenderer::GetInstance();

	const uint32 NumGridVertices = Grid.GetNumVertices();
	const uint32 NumGridIndices = NumGridVertices;

	const EBoundingVolumeType BoundingType = BoundingBoxLines.GetCurrentType();
	const uint32 NumBoundingIndices = BoundingBoxLines.GetNumIndices(BoundingType);

	uint32 NumSpotLightIndices = 0;
	if (bRenderSpotLight)
	{
		const EBoundingVolumeType SpotLightType = SpotLightLines.GetCurrentType();
		NumSpotLightIndices = SpotLightLines.GetNumIndices(SpotLightType);
	}

	uint32 NumOctreeIndices = 0;
	for (auto& OctreeLine : OctreeLines)
	{
		const EBoundingVolumeType OctreeType = OctreeLine.GetCurrentType();
		NumOctreeIndices += OctreeLine.GetNumIndices(OctreeType);
	}

	const uint32 OctreeStartIndex = NumGridIndices + NumBoundingIndices + NumSpotLightIndices;
	if (NumOctreeIndices > 0)
	{
		Renderer.RenderEditorPrimitiveIndexed(
			Primitive,
			Primitive.RenderState,
			sizeof(FVector),
			sizeof(uint32),
			OctreeStartIndex,
			NumOctreeIndices
		);
	}
}

void UBatchLines::SetIndices()
{
	Indices.Empty();

	const uint32 NumGridVertices = Grid.GetNumVertices();

	for (uint32 Index = 0; Index < NumGridVertices; ++Index)
	{
		Indices.Add(Index);
	}

	uint32 BaseVertexOffset = NumGridVertices;

	const EBoundingVolumeType BoundingType = BoundingBoxLines.GetCurrentType();
	int32* BoundingLineIdx = BoundingBoxLines.GetIndices(BoundingType);
	const uint32 NumBoundingIndices = BoundingBoxLines.GetNumIndices(BoundingType);

	if (BoundingLineIdx)
	{
		for (uint32 Idx = 0; Idx < NumBoundingIndices; ++Idx)
		{
			Indices.Add(BaseVertexOffset + BoundingLineIdx[Idx]);
		}
	}

	BaseVertexOffset += BoundingBoxLines.GetNumVertices();

	if (bRenderSpotLight)
	{
		const EBoundingVolumeType SpotLightType = SpotLightLines.GetCurrentType();
		int32* SpotLineIdx = SpotLightLines.GetIndices(SpotLightType);
		const uint32 NumSpotLightIndices = SpotLightLines.GetNumIndices(SpotLightType);

		if (SpotLineIdx)
		{
			for (uint32 Idx = 0; Idx < NumSpotLightIndices; ++Idx)
			{
				Indices.Add(BaseVertexOffset + SpotLineIdx[Idx]);
			}
		}

		BaseVertexOffset += SpotLightLines.GetNumVertices();
	}

	if (bRenderBones) {
		if (int32* BoneIndex = BoneLines.GetIndices()) {
			const uint32 BoneIdxCnt = BoneLines.GetNumIndices();
			for (uint32 k=0;k<BoneIdxCnt;++k) Indices.Add(BaseVertexOffset + BoneIndex[k]);
		}
		BaseVertexOffset += BoneLines.GetNumVertices();
	}

	for (auto& OctreeLine : OctreeLines)
	{
		const EBoundingVolumeType OctreeType = OctreeLine.GetCurrentType();
		int32* OctreeLineIdx = OctreeLine.GetIndices(OctreeType);
		const uint32 NumOctreeIndices = OctreeLine.GetNumIndices(OctreeType);

		if (!OctreeLineIdx)
		{
			continue;
		}

		for (uint32 Idx = 0; Idx < NumOctreeIndices; ++Idx)
		{
			Indices.Add(BaseVertexOffset + OctreeLineIdx[Idx]);
		}

		BaseVertexOffset += OctreeLine.GetNumVertices();
	}
}

