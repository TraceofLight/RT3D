#include "pch.h"
#include "Render/RenderPass/Public/HitProxyPass.h"
#include "Render/Renderer/Public/Pipeline.h"
#include "Render/Renderer/Public/DeviceResources.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Runtime/Renderer/Public/RenderResourceFactory.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Mesh/Public/SkeletalMeshComponent.h"
#include "Component/Public/EditorIconComponent.h"
#include "Render/HitProxy/Public/HitProxy.h"

namespace
{
	// 본 Sphere 반지름 계산 (자식 본까지의 거리 기반)
	float CalculateBoneSphereRadius(const FSkeleton* Skeleton, const TArray<FMatrix>& GlobalPose, int32 BoneIndex)
	{
		if (!Skeleton || BoneIndex < 0 || BoneIndex >= Skeleton->GetNumBones())
		{
			return 0.05f; // 기본값
		}

		// 현재 본 위치
		const FVector BonePos = GlobalPose[BoneIndex].GetLocation();

		// 자식 본이 없으면 부모와의 거리 기반 계산
		if (Skeleton->Childs[BoneIndex].IsEmpty())
		{
			// Leaf 본의 경우 부모와의 거리의 20% 사용
			const int32 ParentIdx = Skeleton->Parents[BoneIndex];
			if (ParentIdx >= 0)
			{
				const FVector ParentPos = GlobalPose[ParentIdx].GetLocation();
				const float DistToParent = (BonePos - ParentPos).Length();
				return std::max(DistToParent * 0.2f, 0.02f);
			}
			return 0.05f;
		}

		// 자식 본과의 평균 거리 계산
		float TotalDist = 0.0f;
		int32 ChildCount = 0;
		for (int32 ChildIdx : Skeleton->Childs[BoneIndex])
		{
			const FVector ChildPos = GlobalPose[ChildIdx].GetLocation();
			TotalDist += (BonePos - ChildPos).Length();
			ChildCount++;
		}

		if (ChildCount > 0)
		{
			const float AvgDist = TotalDist / static_cast<float>(ChildCount);
			return std::max(AvgDist * 0.25f, 0.02f); // 평균 거리의 25%
		}

		return 0.05f;
	}

	// 간단한 Icosphere 정점/인덱스 데이터 (Subdivision 0, 12 vertices, 20 triangles)
	struct FIcosphereMesh
	{
		static constexpr int32 NumVertices = 12;
		static constexpr int32 NumIndices = 60; // 20 triangles * 3

		static const FVector Vertices[NumVertices];
		static const uint32 Indices[NumIndices];
	};

	// Icosahedron 정점 (normalized)
	const FVector FIcosphereMesh::Vertices[NumVertices] = {
		FVector(0.000f,  0.000f,  1.000f),
		FVector(0.894f,  0.000f,  0.447f),
		FVector(0.276f,  0.851f,  0.447f),
		FVector(-0.724f,  0.526f,  0.447f),
		FVector(-0.724f, -0.526f,  0.447f),
		FVector(0.276f, -0.851f,  0.447f),
		FVector(0.724f,  0.526f, -0.447f),
		FVector(-0.276f,  0.851f, -0.447f),
		FVector(-0.894f,  0.000f, -0.447f),
		FVector(-0.276f, -0.851f, -0.447f),
		FVector(0.724f, -0.526f, -0.447f),
		FVector(0.000f,  0.000f, -1.000f)
	};

	// Icosahedron 인덱스
	const uint32 FIcosphereMesh::Indices[NumIndices] = {
		0, 1, 2,  0, 2, 3,  0, 3, 4,  0, 4, 5,  0, 5, 1,
		1, 6, 2,  2, 7, 3,  3, 8, 4,  4, 9, 5,  5, 10, 1,
		6, 7, 2,  7, 8, 3,  8, 9, 4,  9, 10, 5,  10, 6, 1,
		11, 6, 7, 11, 7, 8, 11, 8, 9, 11, 9, 10, 11, 10, 6
	};
}


FHitProxyPass::FHitProxyPass(UPipeline* InPipeline, ID3D11Buffer* InConstantBufferCamera, ID3D11Buffer* InConstantBufferModel,
                             ID3D11VertexShader* InVS, ID3D11PixelShader* InPS, ID3D11InputLayout* InLayout, ID3D11DepthStencilState* InDS)
	: FRenderPass(InPipeline, InConstantBufferCamera, InConstantBufferModel), VS(InVS), PS(InPS), InputLayout(InLayout), DS(InDS)
{
	// HitProxyColor 상수 버퍼 생성 (b2 슬롯)
	ConstantBufferHitProxyColor = FRenderResourceFactory::CreateConstantBuffer<FVector4>();
}

void FHitProxyPass::SetRenderTargets(class UDeviceResources* DeviceResources)
{
	// HitProxy RTV/DSV 설정
	ID3D11RenderTargetView* HitProxyRTV = DeviceResources->GetHitProxyRTV();
	ID3D11DepthStencilView* DSV = DeviceResources->GetDepthBufferDSV();
	Pipeline->SetRenderTargets(1, &HitProxyRTV, DSV);

	// HitProxy 텍스처 클리어 (검은색 = 배경)
	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	DeviceResources->GetDeviceContext()->ClearRenderTargetView(HitProxyRTV, ClearColor);
	DeviceResources->GetDeviceContext()->ClearDepthStencilView(DSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void FHitProxyPass::Execute(FRenderingContext& Context)
{
	if (!VS || !PS || !InputLayout)
	{
		return;
	}

	const auto& Renderer = URenderer::GetInstance();
	const auto& DeviceResources = Renderer.GetDeviceResources();
	GPU_EVENT(DeviceResources->GetDeviceContext(), "HitProxyPass");

	// Viewport 설정 (Context에서 전달받은 Viewport 사용)
	DeviceResources->GetDeviceContext()->RSSetViewports(1, &Context.Viewport);

	// 카메라 상수 버퍼 업데이트
	FCameraConstants CameraConstants = Context.ViewInfo.CameraConstants;
	CameraConstants.ViewWorldLocation = Context.ViewInfo.Location;
	CameraConstants.NearClip = Context.ViewInfo.NearClipPlane;
	CameraConstants.FarClip = Context.ViewInfo.FarClipPlane;
	FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferCamera, CameraConstants);

	// RenderState 설정 (StaticMeshPass와 동일)
	FRenderState RenderState = UStaticMeshComponent::GetClassDefaultRenderState();

	ID3D11RasterizerState* RS = FRenderResourceFactory::GetRasterizerState(RenderState);

	// Pipeline 설정
	FPipelineInfo PipelineInfo = { InputLayout, VS, RS, DS, PS, nullptr, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST };
	Pipeline->UpdatePipeline(PipelineInfo);

	// 상수 버퍼 바인딩
	Pipeline->SetConstantBuffer(0, EShaderType::VS, ConstantBufferModel);
	Pipeline->SetConstantBuffer(1, EShaderType::VS | EShaderType::PS, ConstantBufferCamera);
	Pipeline->SetConstantBuffer(2, EShaderType::PS, ConstantBufferHitProxyColor);

	// HitProxyManager 초기화
	FHitProxyManager& HitProxyManager = FHitProxyManager::GetInstance();
	HitProxyManager.ClearAllHitProxies();

	// EditorIcon 컴포넌트 렌더링
	for (UEditorIconComponent* IconComp : Context.EditorIcons)
	{
		if (!IconComp->IsVisible())
		{
			continue;
		}

		// Billboard 카메라 정렬 적용
		FVector CameraForward = Context.ViewInfo.Rotation.RotateVector(FVector::ForwardVector());
		IconComp->FaceCamera(CameraForward);

		// HitProxy ID 할당
		HComponent* ComponentProxy = new HComponent(IconComp, InvalidHitProxyId);
		FHitProxyId ProxyId = HitProxyManager.AllocateHitProxyId(ComponentProxy);

		// HitProxyColor 상수 버퍼 업데이트
		FVector4 ProxyColor = ProxyId.GetColor();
		FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferHitProxyColor, ProxyColor);
		Pipeline->SetConstantBuffer(2, EShaderType::PS, ConstantBufferHitProxyColor);

		// Vertex/Index 버퍼 바인딩
		Pipeline->SetVertexBuffer(IconComp->GetVertexBuffer(), sizeof(FNormalVertex));
		Pipeline->SetIndexBuffer(IconComp->GetIndexBuffer(), 0);

		// Model 상수 버퍼 업데이트
		FMatrix WorldMatrix;
		if (IconComp->IsScreenSizeScaled())
		{
			FVector FixedWorldScale = IconComp->GetRelativeScale3D();
			FVector IconLocation = IconComp->GetWorldLocation();
			FQuat IconRotation = IconComp->GetWorldRotationAsQuaternion();
			WorldMatrix = FMatrix::GetModelMatrix(IconLocation, IconRotation, FixedWorldScale);
		}
		else
		{
			WorldMatrix = IconComp->GetWorldTransformMatrix();
		}
		FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferModel, WorldMatrix);
		Pipeline->SetConstantBuffer(0, EShaderType::VS, ConstantBufferModel);

		// 렌더링
		Pipeline->DrawIndexed(IconComp->GetNumIndices(), 0, 0);
	}

	// StaticMesh 컴포넌트 렌더링
	FStaticMesh* CurrentMeshAsset = nullptr;

	for (UStaticMeshComponent* MeshComp : Context.StaticMeshes)
	{
		if (!MeshComp->IsVisible()) { continue; }
		if (!MeshComp->GetStaticMesh()) { continue; }

		FStaticMesh* MeshAsset = MeshComp->GetStaticMesh()->GetStaticMeshAsset();
		if (!MeshAsset) { continue; }

		// HitProxy ID 할당
		HComponent* ComponentProxy = new HComponent(MeshComp, InvalidHitProxyId);
		FHitProxyId ProxyId = HitProxyManager.AllocateHitProxyId(ComponentProxy);

		// HitProxyColor 상수 버퍼 업데이트
		FVector4 ProxyColor = ProxyId.GetColor();
		FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferHitProxyColor, ProxyColor);
		Pipeline->SetConstantBuffer(2, EShaderType::PS, ConstantBufferHitProxyColor);

		// 메쉬가 변경되면 버퍼 바인딩
		if (CurrentMeshAsset != MeshAsset)
		{
			CurrentMeshAsset = MeshAsset;
			Pipeline->SetVertexBuffer(MeshComp->GetVertexBuffer(), sizeof(FNormalVertex));
			Pipeline->SetIndexBuffer(MeshComp->GetIndexBuffer(), 0);
		}

		// Model 상수 버퍼 업데이트 (World Transform)
		FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferModel, MeshComp->GetWorldTransformMatrix());
		Pipeline->SetConstantBuffer(0, EShaderType::VS, ConstantBufferModel);

		// 렌더링
		Pipeline->DrawIndexed(static_cast<uint32>(MeshAsset->Indices.Num()), 0, 0);
	}

	// SkeletalMesh 컴포넌트 렌더링
	for (USkeletalMeshComponent* MeshComp : Context.SkeletalMeshes)
	{
		if (!MeshComp->IsVisible())
		{
			continue;
		}
		if (!MeshComp->GetSkeletalMesh())
		{
			continue;
		}

		USkeletalMesh* SkeletalMeshAsset = MeshComp->GetSkeletalMesh();
		if (!SkeletalMeshAsset || !SkeletalMeshAsset->IsValid())
		{
			continue;
		}

		FSkeletalMesh* MeshData = SkeletalMeshAsset->GetSkeletalMeshAsset();
		if (!MeshData || MeshData->Sections.IsEmpty())
		{
			continue;
		}

		const TArray<FMatrix>& SkinMatrices = MeshComp->GetSkinMatrices();
		if (SkinMatrices.IsEmpty())
		{
			continue;
		}

		// Component에서 캐시된 Skinned Vertices 가져오기
		const TArray<FNormalVertex>& SkinnedVertices = MeshComp->GetSkinnedVertices();
		const TArray<uint32>& SkinnedIndices = MeshComp->GetSkinnedIndices();

		if (SkinnedVertices.IsEmpty() || SkinnedIndices.IsEmpty())
		{
			continue;
		}

		// Bone Edit Mode가 아니면 전체 메쉬를 단일 HComponent로 렌더링
		if (!MeshComp->IsInBoneEditMode())
		{
			// HitProxy ID 할당
			HComponent* ComponentProxy = new HComponent(MeshComp, InvalidHitProxyId);
			FHitProxyId ProxyId = HitProxyManager.AllocateHitProxyId(ComponentProxy);

			// HitProxyColor 상수 버퍼 업데이트
			FVector4 ProxyColor = ProxyId.GetColor();
			FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferHitProxyColor, ProxyColor);
			Pipeline->SetConstantBuffer(2, EShaderType::PS, ConstantBufferHitProxyColor);

			// 단일 Vertex/Index Buffer 생성 (모든 Section 통합)
			ID3D11Buffer* DynamicVB = FRenderResourceFactory::CreateDynamicVertexBuffer(
				SkinnedVertices.GetData(),
				static_cast<int32>(SkinnedVertices.Num() * sizeof(FNormalVertex))
			);

			ID3D11Buffer* DynamicIB = FRenderResourceFactory::CreateDynamicIndexBuffer(
				SkinnedIndices.GetData(),
				static_cast<int32>(SkinnedIndices.Num() * sizeof(uint32))
			);

			if (!DynamicVB || !DynamicIB)
			{
				SafeRelease(DynamicVB);
				SafeRelease(DynamicIB);
				continue;
			}

			Pipeline->SetVertexBuffer(DynamicVB, sizeof(FNormalVertex));
			Pipeline->SetIndexBuffer(DynamicIB, 0);

			// Model 상수 버퍼 업데이트 (World Transform)
			const FMatrix& WorldMatrix = MeshComp->GetWorldTransformMatrix();
			FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferModel, WorldMatrix);
			Pipeline->SetConstantBuffer(0, EShaderType::VS, ConstantBufferModel);

			// 전체 메쉬를 단일 색상으로 렌더링
			Pipeline->DrawIndexed(static_cast<uint32>(SkinnedIndices.Num()), 0, 0);

			// 버퍼 해제
			SafeRelease(DynamicVB);
			SafeRelease(DynamicIB);
		}
		// Bone Edit Mode: 본별 Sphere 렌더링
		else
		{
			const FSkeleton* Skeleton = SkeletalMeshAsset->GetSkeleton();
			if (!Skeleton)
			{
				continue;
			}

			const TArray<FMatrix>& GlobalPose = MeshComp->GetGlobalPose();
			if (GlobalPose.IsEmpty())
			{
				continue;
			}

			const int32 NumBones = Skeleton->GetNumBones();
			const FMatrix& ComponentToWorld = MeshComp->GetWorldTransformMatrix();

			// 각 본에 대해 Sphere 렌더링
			for (int32 BoneIdx = 0; BoneIdx < NumBones; ++BoneIdx)
			{
				// HBone HitProxy 생성
				HBone* BoneProxy = new HBone(MeshComp, BoneIdx, InvalidHitProxyId);
				FHitProxyId BoneProxyId = HitProxyManager.AllocateHitProxyId(BoneProxy);

				// HitProxyColor 업데이트
				FVector4 BoneProxyColor = BoneProxyId.GetColor();
				FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferHitProxyColor, BoneProxyColor);
				Pipeline->SetConstantBuffer(2, EShaderType::PS, ConstantBufferHitProxyColor);

				// Sphere 반지름 계산
				const float SphereRadius = CalculateBoneSphereRadius(Skeleton, GlobalPose, BoneIdx);

				// 본 위치 (Component Space)
				const FVector BonePosCS = GlobalPose[BoneIdx].GetLocation();

				// World Space로 변환
				const FVector BonePosWS = ComponentToWorld.TransformPosition(BonePosCS);

				// Sphere 변환 행렬 (Translation * Scale)
				const FMatrix SphereWorld = FMatrix::GetModelMatrix(BonePosWS, FQuat::Identity(), FVector(SphereRadius, SphereRadius, SphereRadius));

				// Model 상수 버퍼 업데이트
				FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferModel, SphereWorld);
				Pipeline->SetConstantBuffer(0, EShaderType::VS, ConstantBufferModel);

				// Icosphere 버퍼 생성 (동적)
				TArray<FNormalVertex> SphereVertices;
				SphereVertices.SetNum(FIcosphereMesh::NumVertices);
				for (int32 i = 0; i < FIcosphereMesh::NumVertices; ++i)
				{
					SphereVertices[i].Position = FIcosphereMesh::Vertices[i];
					SphereVertices[i].Normal = FIcosphereMesh::Vertices[i]; // Sphere는 Normal = Position
				}

				ID3D11Buffer* SphereVB = FRenderResourceFactory::CreateDynamicVertexBuffer(
					SphereVertices.GetData(),
					static_cast<int32>(SphereVertices.Num() * sizeof(FNormalVertex))
				);

				ID3D11Buffer* SphereIB = FRenderResourceFactory::CreateDynamicIndexBuffer(
					FIcosphereMesh::Indices,
					static_cast<int32>(FIcosphereMesh::NumIndices * sizeof(uint32))
				);

				if (!SphereVB || !SphereIB)
				{
					SafeRelease(SphereVB);
					SafeRelease(SphereIB);
					continue;
				}

				Pipeline->SetVertexBuffer(SphereVB, sizeof(FNormalVertex));
				Pipeline->SetIndexBuffer(SphereIB, 0);

				// Sphere 렌더링
				Pipeline->DrawIndexed(FIcosphereMesh::NumIndices, 0, 0);

				// 버퍼 해제
				SafeRelease(SphereVB);
				SafeRelease(SphereIB);
			}
		}
	}
}

void FHitProxyPass::Release()
{
	if (ConstantBufferHitProxyColor)
	{
		ConstantBufferHitProxyColor->Release();
		ConstantBufferHitProxyColor = nullptr;
	}
}

void FHitProxyPass::SetShaders(ID3D11VertexShader* InVS, ID3D11PixelShader* InPS, ID3D11InputLayout* InLayout)
{
	VS = InVS;
	PS = InPS;
	InputLayout = InLayout;
}
