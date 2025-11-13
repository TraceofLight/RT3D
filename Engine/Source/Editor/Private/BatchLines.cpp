#include "pch.h"
#include "Editor/Public/BatchLines.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Runtime/Renderer/Public/RenderResourceFactory.h"
#include "Global/Octree.h"
#include "Component/Public/DecalSpotLightComponent.h"

#include "Level/Public/Level.h"
#include "Physics/Public/OBB.h"

static inline void AddLine(
	TArray<FVertexPositionColor>& Vertices, TArray<int32>& Indices,
	const FVector& Point1, const FVector& Point2,
	const FVector4& LineColor)
{
	const int IndexP1 = Vertices.Num(); Vertices.Add({Point1, LineColor});
	const int IndexP2 = Vertices.Num(); Vertices.Add({Point2, LineColor});
	Indices.Add(IndexP1); Indices.Add(IndexP2);
}

static inline void AddRing(
	TArray<FVertexPositionColor>& Vertices, TArray<int32>& Indices,
	const FVector& Center, const FVector& AngleX, const FVector& AngleY,
	float Rotation, int Segment = 16,
	const FVector4& RingColor = FVector4(1, 1, 1, 1))
{
	if (Rotation <= 0.f) return;
	const float dth = 2.f * PI / float(Segment);
	FVector p0 = Center + (AngleX * Rotation);
	for (int s = 1; s <= Segment; ++s) {
		const float th = dth * s;
		const FVector p = Center + (AngleX * (cosf(th) * Rotation)) + (AngleY * (sinf(th) * Rotation));
		AddLine(Vertices, Indices, p0, p, RingColor);
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
	TArray<D3D11_INPUT_ELEMENT_DESC> Layout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(FVertexPositionColor, Position),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(FVertexPositionColor, Color),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	FRenderResourceFactory::CreateVertexShaderAndInputLayout(L"Asset/Shader/BatchLineShader.hlsl", Layout, &VertexShader, &InputLayout);
	ID3D11PixelShader* PixelShader;
	FRenderResourceFactory::CreatePixelShader(L"Asset/Shader/BatchLineShader.hlsl", &PixelShader);

	Primitive.VertexShader = VertexShader;
	Primitive.InputLayout = InputLayout;
	Primitive.PixelShader = PixelShader;
	Primitive.NumVertices = static_cast<uint32>(Vertices.Num());
	Primitive.NumIndices = static_cast<uint32>(Indices.Num());
	Primitive.VertexBuffer = FRenderResourceFactory::CreateVertexBuffer(Vertices.GetData(), Primitive.NumVertices * sizeof(FVertexPositionColor), true);
	Primitive.IndexBuffer = FRenderResourceFactory::CreateIndexBuffer(Indices.GetData(), Primitive.NumIndices * sizeof(uint32));
	Primitive.Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

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
    	const FMatrix WorldBoneMatrix = GlobalPose[i] * ComponentToWorld;
    	WorldPos[i] = XformPos(WorldBoneMatrix);
    }

    // 조인트: 3개 링
    constexpr float JointSphereRadius = 0.15f;
    for (int i=0; i<Num; ++i) {
        const float Radius = JointSphereRadius;

		// 색상 결정: Joint 전용 헬퍼 함수 사용
		FVector4 Color = GetJointColor(i, SelectedBone, Skeleton);

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
        AddRing(BoneLines.Vertices, BoneLines.Indices, Center, U, V, Radius, 14, Color);
        AddRing(BoneLines.Vertices, BoneLines.Indices, Center, V, Direction, Radius, 14, Color);
        AddRing(BoneLines.Vertices, BoneLines.Indices, Center, Direction, U, Radius, 14, Color);
    }

    // 본(부모 -> 자식) - 단일 사각뿔
    auto AddBonePyramid = [&](int Parent, int Child, FVector4 Color)
    {
        const FVector P = WorldPos[Parent];
        const FVector C = WorldPos[Child];
        FVector Direction = (C - P);
        const float L = Direction.Length();
        if (L < 1e-4f) return;
        Direction *= (1.0f / L);

        FVector U,V; OrthonormalBasis(Direction, U, V);

        const float Width = max(0.002f, L * WidthScale);  // 두께

        // Parent 위치에서 정사각형 Base 생성
        const FVector c0 = P + (U * Width);
        const FVector c1 = P + (V * Width);
        const FVector c2 = P - (U * Width);
        const FVector c3 = P - (V * Width);

        // Child는 Tip
        const FVector Tip = C;

        // Base 사각형 테두리
        AddLine(BoneLines.Vertices, BoneLines.Indices, c0, c1, Color);
		AddLine(BoneLines.Vertices, BoneLines.Indices, c1, c2, Color);
		AddLine(BoneLines.Vertices, BoneLines.Indices, c2, c3, Color);
		AddLine(BoneLines.Vertices, BoneLines.Indices, c3, c0, Color);

        // Tip에서 각 Base 코너로 모서리
        AddLine(BoneLines.Vertices, BoneLines.Indices, Tip, c0, Color);
		AddLine(BoneLines.Vertices, BoneLines.Indices, Tip, c1, Color);
		AddLine(BoneLines.Vertices, BoneLines.Indices, Tip, c2, Color);
		AddLine(BoneLines.Vertices, BoneLines.Indices, Tip, c3, Color);
    };

	int SelectedParent = -1;
	TArray<uint8> InSubtree;
	InSubtree.SetNum(Num); // 0/1 플래그

	if (SelectedBone >= 0 && SelectedBone < Num) {
		SelectedParent = Skeleton->Parents[SelectedBone];

		// DFS/BFS로 서브트리 마킹 (루트 포함)
		TArray<int32> stack;
		stack.Add(SelectedBone);
		InSubtree[SelectedBone] = 1;

		while (stack.Num() > 0) {
			int n = stack.Top();
			stack.Pop();

			for (int c : Skeleton->Childs[n]) {
				if (!InSubtree[c]) {
					InSubtree[c] = 1;
					stack.Add(c);
				}
			}
		}
	}

	for (int i = 0; i < Num; ++i) {
		for (int Child : Skeleton->Childs[i]) {
			FVector4 Color;

			// 특수 케이스: 부모→선택된 Joint로 향하는 Bone은 주황색
			if (Child == SelectedBone)
			{
				int32 SelectedParentIdx = (SelectedBone < Skeleton->Parents.Num()) ? Skeleton->Parents[SelectedBone] : -1;
				if (i == SelectedParentIdx)
				{
					Color = {1.0f, 0.5f, 0.0f, 1.0f};  // Orange
				}
				else
				{
					Color = GetBoneColor(i, SelectedBone, Skeleton);
				}
			}
			else
			{
				// 일반 케이스: Parent 색상 사용
				Color = GetBoneColor(i, SelectedBone, Skeleton);
			}

			AddBonePyramid(i, Child, Color);
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

		Primitive.VertexBuffer = FRenderResourceFactory::CreateVertexBuffer(Vertices.GetData(), Primitive.NumVertices * sizeof(FVertexPositionColor), true);
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

	// Skeleton
	RenderSkeleton();

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
			sizeof(FVertexPositionColor),
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
			sizeof(FVertexPositionColor),
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
			sizeof(FVertexPositionColor),
			sizeof(uint32),
			NumGridIndices,
			NumBoundingIndices
		);
	}
}

void UBatchLines::RenderSkeleton()
{
	if (!bRenderBones) return;

	// ShowFlags 체크 (OwningWorld 우선, 없으면 EditorWorld)
	UWorld* TargetWorld = OwningWorld;
	if (!TargetWorld && GEditor)
	{
		TargetWorld = GEditor->GetEditorWorldContext().World();
	}

	if (TargetWorld && TargetWorld->GetLevel())
	{
		uint64 ShowFlags = TargetWorld->GetLevel()->GetShowFlags();
		if (!(ShowFlags & EEngineShowFlags::SF_Bone))
		{
			return;
		}
	}

	URenderer& Renderer = URenderer::GetInstance();

	const uint32 NumGridIndices = Grid.GetNumVertices();

	const EBoundingVolumeType BoundingType = BoundingBoxLines.GetCurrentType();
	const uint32 NumBoundingIndices = BoundingBoxLines.GetNumIndices(BoundingType);

	uint32 NumSpotLightIndices = 0;
	if (bRenderSpotLight)
	{
		const EBoundingVolumeType SpotLightType = SpotLightLines.GetCurrentType();
		NumSpotLightIndices = SpotLightLines.GetNumIndices(SpotLightType);
	}

	const uint32 NumBoneIndices = BoneLines.GetNumIndices();
	if (NumBoneIndices == 0) return;

	const uint32 BoneStartIndex = NumGridIndices + NumBoundingIndices + NumSpotLightIndices;

	Renderer.RenderEditorPrimitiveIndexed(
		Primitive,
		Primitive.RenderState,
		sizeof(FVertexPositionColor),
		sizeof(uint32),
		BoneStartIndex,
		NumBoneIndices,
		true
	);
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

	const uint32 NumBoneIndices = bRenderBones ? BoneLines.GetNumIndices() : 0;

	uint32 NumOctreeIndices = 0;
	for (auto& OctreeLine : OctreeLines)
	{
		const EBoundingVolumeType OctreeType = OctreeLine.GetCurrentType();
		NumOctreeIndices += OctreeLine.GetNumIndices(OctreeType);
	}

	const uint32 OctreeStartIndex = NumGridIndices + NumBoundingIndices + NumSpotLightIndices + NumBoneIndices;
	if (NumOctreeIndices > 0)
	{
		Renderer.RenderEditorPrimitiveIndexed(
			Primitive,
			Primitive.RenderState,
			sizeof(FVertexPositionColor),
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

/**
 * @brief HitProxy용 Bone 입체 메시 생성 (단일 사각뿔 형태)
 * @param Skeleton 스켈레톤 데이터
 * @param GlobalPose 본별 월드 행렬
 * @param ComponentToWorld 컴포넌트 월드 행렬
 * @param WidthScale Bone 두께 스케일
 * @param BaseBias Base Center 위치 (0: Parent, 1: Child) - 현재 미사용
 * @param OutMesh 출력 메시 데이터
 */
void UBatchLines::GenerateBoneMeshForHitProxy(
	const FSkeleton* Skeleton,
	const TArray<FMatrix>& GlobalPose,
	const FMatrix& ComponentToWorld,
	float WidthScale,
	float BaseBias,
	FBoneMesh& OutMesh)
{
	OutMesh.Reset();

	if (!Skeleton || GlobalPose.IsEmpty())
	{
		return;
	}

	auto XformPos = [](const FMatrix& M) -> FVector
	{
		return M.TransformPosition(FVector(0, 0, 0));
	};

	const int32 Num = Skeleton->Parents.Num();
	if (Num <= 0)
	{
		return;
	}

	// 월드 보정된 본 위치 계산
	TArray<FVector> WorldPos;
	WorldPos.SetNum(Num);
	for (int32 i = 0; i < Num; ++i)
	{
		const FMatrix WorldBoneMatrix = GlobalPose[i] * ComponentToWorld;
		WorldPos[i] = XformPos(WorldBoneMatrix);
	}

	// 각 Bone에 대해 단일 사각뿔 메시 생성 (Parent -> Child)
	for (int32 ParentIdx = 0; ParentIdx < Num; ++ParentIdx)
	{
		for (int32 ChildIdx : Skeleton->Childs[ParentIdx])
		{
			const FVector P = WorldPos[ParentIdx];
			const FVector C = WorldPos[ChildIdx];
			FVector Direction = (C - P);
			const float L = Direction.Length();

			if (L < 1e-4f)
			{
				continue;  // 너무 짧은 Bone은 스킵
			}

			Direction *= (1.0f / L);  // Normalize

			// 직교 축 계산
			FVector U, V;
			OrthonormalBasis(Direction, U, V);

			// Bone 두께 계산 (Parent에서의 Base 두께)
			const float Width = max(0.002f, L * WidthScale);

			// Parent 위치에서 정사각형 Base 정점 4개 생성
			const FVector c0 = P + (U * Width);
			const FVector c1 = P + (V * Width);
			const FVector c2 = P - (U * Width);
			const FVector c3 = P - (V * Width);

			// 정점 인덱스 시작
			const uint32 BaseVertIdx = OutMesh.Vertices.Num();

			// 정점 추가 (5개: Child Tip, 4개 Base 정점)
			OutMesh.Vertices.Add(C);   // 0: Child (Tip)
			OutMesh.Vertices.Add(c0);  // 1: Base corner 0
			OutMesh.Vertices.Add(c1);  // 2: Base corner 1
			OutMesh.Vertices.Add(c2);  // 3: Base corner 2
			OutMesh.Vertices.Add(c3);  // 4: Base corner 3

			// Helper lambda: 삼각형 추가
			auto AddTriangle = [&](uint32 i0, uint32 i1, uint32 i2, int32 BoneIdx)
			{
				OutMesh.Indices.Add(BaseVertIdx + i0);
				OutMesh.Indices.Add(BaseVertIdx + i1);
				OutMesh.Indices.Add(BaseVertIdx + i2);
				OutMesh.BoneIndices.Add(BoneIdx);
			};

			// 사각뿔 4개 삼각형 (Child Tip에서 Base로) - ParentIdx HitProxy (Base가 Parent 위치이므로)
			AddTriangle(0, 1, 2, ParentIdx);
			AddTriangle(0, 2, 3, ParentIdx);
			AddTriangle(0, 3, 4, ParentIdx);
			AddTriangle(0, 4, 1, ParentIdx);
		}
	}
}

/**
 * @brief 자손 여부 확인 (DFS)
 * @param TestBoneIndex 확인할 본 인덱스
 * @param AncestorIndex 조상 본 인덱스
 * @param Skeleton 스켈레톤 데이터
 * @return TestBoneIndex가 AncestorIndex의 자손이면 true
 */
bool UBatchLines::IsDescendant(int32 TestBoneIndex, int32 AncestorIndex, const FSkeleton* Skeleton)
{
	if (!Skeleton || TestBoneIndex < 0 || AncestorIndex < 0)
	{
		return false;
	}

	// DFS로 AncestorIndex의 모든 자손 탐색
	TArray<int32> Stack;
	for (int32 ChildIdx : Skeleton->Childs[AncestorIndex])
	{
		Stack.Add(ChildIdx);
	}

	while (!Stack.IsEmpty())
	{
		int32 CurrentIdx = Stack.Last();
		Stack.RemoveAt(Stack.Num() - 1);

		if (CurrentIdx == TestBoneIndex)
		{
			return true;
		}

		// 자식들을 스택에 추가
		for (int32 ChildIdx : Skeleton->Childs[CurrentIdx])
		{
			Stack.Add(ChildIdx);
		}
	}

	return false;
}

/**
 * @brief Joint 색상 결정 (선택 상태에 따라)
 * @param JointIndex 색상을 결정할 Joint 인덱스
 * @param SelectedBoneIndex 선택된 본 인덱스 (-1이면 선택 없음)
 * @param Skeleton 스켈레톤 데이터
 * @return RGBA 색상 (0-1 범위)
 *
 * Joint 색상: 선택(초록), 자손(흰색), 나머지(검은색)
 */
FVector4 UBatchLines::GetJointColor(int32 JointIndex, int32 SelectedBoneIndex, const FSkeleton* Skeleton)
{
	// 선택된 본이 없으면 기본 색상 (검은색)
	if (SelectedBoneIndex < 0 || !Skeleton)
	{
		return {0.0f, 0.0f, 0.0f, 1.0f};  // Black
	}

	// 본인 선택: 초록색
	if (JointIndex == SelectedBoneIndex)
	{
		return {0.0f, 1.0f, 0.0f, 1.0f};  // Green
	}

	// 자손: 흰색
	if (IsDescendant(JointIndex, SelectedBoneIndex, Skeleton))
	{
		return {1.0f, 1.0f, 1.0f, 1.0f};  // White
	}

	// 나머지: 검은색
	return {0.0f, 0.0f, 0.0f, 1.0f};  // Black
}

/**
 * @brief Bone 색상 결정 (선택 상태에 따라)
 * @param BoneIndex 색상을 결정할 본 인덱스
 * @param SelectedBoneIndex 선택된 본 인덱스 (-1이면 선택 없음)
 * @param Skeleton 스켈레톤 데이터
 * @return RGBA 색상 (0-1 범위)
 *
 * Bone 색상: 선택(초록), 자손(흰색), 나머지(검은색)
 * 주의: 부모 Bone의 주황색은 렌더링 루프에서 직접 처리됨
 */
FVector4 UBatchLines::GetBoneColor(int32 BoneIndex, int32 SelectedBoneIndex, const FSkeleton* Skeleton)
{
	// 선택된 본이 없으면 기본 색상 (검은색)
	if (SelectedBoneIndex < 0 || !Skeleton)
	{
		return {0.0f, 0.0f, 0.0f, 1.0f};  // Black
	}

	// 본인 선택: 초록색
	if (BoneIndex == SelectedBoneIndex)
	{
		return {0.0f, 1.0f, 0.0f, 1.0f};  // Green
	}

	// 자손 본: 흰색
	if (IsDescendant(BoneIndex, SelectedBoneIndex, Skeleton))
	{
		return {1.0f, 1.0f, 1.0f, 1.0f};  // White
	}

	// 나머지: 검은색
	return {0.0f, 0.0f, 0.0f, 1.0f};  // Black
}

