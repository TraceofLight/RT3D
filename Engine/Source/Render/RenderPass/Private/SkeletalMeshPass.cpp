#include "pch.h"
#include "Render/RenderPass/Public/SkeletalMeshPass.h"

#include "Component/Mesh/Public/SkeletalMeshComponent.h"
#include "Component/Mesh/Public/SkinnedMeshComponent.h"
#include "Render/Renderer/Public/Pipeline.h"
#include "Runtime/Renderer/Public/RenderResourceFactory.h"
#include "Texture/Public/Texture.h"
#include "Render/RenderPass/Public/ShadowMapPass.h"
#include "Component/Public/PointLightComponent.h"
#include "Texture/Public/ShadowMapResources.h"
#include "Texture/Public/Material.h"

FSkeletalMeshPass::FSkeletalMeshPass(UPipeline* InPipeline, ID3D11Buffer* InConstantBufferCamera, ID3D11Buffer* InConstantBufferModel,
                                     ID3D11VertexShader* InVS, ID3D11PixelShader* InPS, ID3D11InputLayout* InLayout, ID3D11DepthStencilState* InDS)
	: FRenderPass(InPipeline, InConstantBufferCamera, InConstantBufferModel), VS(InVS), PS(InPS), InputLayout(InLayout), DS(InDS)
{
	ConstantBufferMaterial = FRenderResourceFactory::CreateConstantBuffer<FMaterialConstants>();
}

void FSkeletalMeshPass::SetRenderTargets(class UDeviceResources* DeviceResources)
{
	ID3D11RenderTargetView* RTVs[] = { DeviceResources->GetDestinationRTV(), DeviceResources->GetNormalBufferRTV() };
	ID3D11DepthStencilView* DSV = DeviceResources->GetDepthBufferDSV();
	Pipeline->SetRenderTargets(2, RTVs, DSV);
}

void FSkeletalMeshPass::Execute(FRenderingContext& Context)
{
	const auto& Renderer = URenderer::GetInstance();
	GPU_EVENT(Renderer.GetDeviceContext(), "SkeletalMeshPass");

	FRenderState RenderState = { ECullMode::Back, EFillMode::Solid };
	if (Context.ViewMode == EViewModeIndex::VMI_Wireframe)
	{
		RenderState.CullMode = ECullMode::None;
		RenderState.FillMode = EFillMode::WireFrame;
	}
	else
	{
		VS = Renderer.GetVertexShader(Context.ViewMode);
		PS = Renderer.GetPixelShader(Context.ViewMode);
	}

	ID3D11RasterizerState* RS = FRenderResourceFactory::GetRasterizerState(RenderState);
	FPipelineInfo PipelineInfo = { InputLayout, VS, RS, DS, PS, nullptr, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST };
	Pipeline->UpdatePipeline(PipelineInfo);

	Pipeline->SetSamplerState(0, EShaderType::PS, URenderer::GetInstance().GetDefaultSampler());

	// Bind shadow map and comparison sampler
	Pipeline->SetSamplerState(1, EShaderType::PS, Renderer.GetShadowComparisonSampler());
	Pipeline->SetSamplerState(2, EShaderType::PS, Renderer.GetVarianceShadowSampler());
	Pipeline->SetSamplerState(3, EShaderType::PS, Renderer.GetPointShadowSampler());

	FShadowMapPass* ShadowPass = Renderer.GetShadowMapPass();
	if (ShadowPass)
	{
		FShadowMapResource* ShadowAtlas = ShadowPass->GetShadowAtlas();
		if (ShadowAtlas && ShadowAtlas->IsValid())
		{
			Pipeline->SetShaderResourceView(10, EShaderType::PS, ShadowAtlas->ShadowSRV.Get());
			Pipeline->SetShaderResourceView(11, EShaderType::PS, ShadowAtlas->VarianceShadowSRV.Get());
		}
	}

	Pipeline->SetConstantBuffer(0, EShaderType::VS, ConstantBufferModel);
	Pipeline->SetConstantBuffer(1, EShaderType::VS | EShaderType::PS, ConstantBufferCamera);

	if (!(Context.ShowFlags & EEngineShowFlags::SF_SkeletalMesh)) { return; }

	TArray<USkeletalMeshComponent*>& MeshComponents = Context.SkeletalMeshes;

	for (USkeletalMeshComponent* MeshComp : MeshComponents)
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

		const FMatrix& WorldMatrix = MeshComp->GetWorldTransformMatrix();
		FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferModel, WorldMatrix);
		Pipeline->SetConstantBuffer(0, EShaderType::VS, ConstantBufferModel);

		// Determinant 체크
		float Det = WorldMatrix.Determinant3x3();
		ECullMode DynamicCullMode = (Det < 0.0f) ? ECullMode::Front : ECullMode::Back;

		if (Context.ViewMode != EViewModeIndex::VMI_Wireframe && RenderState.CullMode != DynamicCullMode)
		{
			RenderState.CullMode = DynamicCullMode;
			ID3D11RasterizerState* DynamicRS = FRenderResourceFactory::GetRasterizerState(RenderState);
			FPipelineInfo DynamicPipeline = { InputLayout, VS, DynamicRS, DS, PS, nullptr, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST };
			Pipeline->UpdatePipeline(DynamicPipeline);
		}

		// Component에서 캐시된 Skinned Vertices 가져오기 (Dirty Flag 체크 포함)
		const TArray<FNormalVertex>& SkinnedVertices = MeshComp->GetSkinnedVertices();
		const TArray<uint32>& SkinnedIndices = MeshComp->GetSkinnedIndices();

		if (SkinnedVertices.IsEmpty() || SkinnedIndices.IsEmpty())
		{
			continue;
		}

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

		// Section별로 Material 바인딩 및 DrawIndexed 호출
		uint32 CurrentIndexOffset = 0;
		for (const FSkeletalMeshSection& Section : MeshData->Sections)
		{
			if (Section.Indices.IsEmpty()) { continue; }

			// Material 바인딩
			UMaterial* Material = SkeletalMeshAsset->GetMaterial(Section.MaterialSlot);
			if (Material)
			{
				FMaterialConstants MaterialConstants = {};
				FVector AmbientColor = Material->GetAmbientColor();
				MaterialConstants.Ka = FVector4(AmbientColor.X, AmbientColor.Y, AmbientColor.Z, 1.0f);
				FVector DiffuseColor = Material->GetDiffuseColor();
				MaterialConstants.Kd = FVector4(DiffuseColor.X, DiffuseColor.Y, DiffuseColor.Z, 1.0f);
				FVector SpecularColor = Material->GetSpecularColor();
				MaterialConstants.Ks = FVector4(SpecularColor.X, SpecularColor.Y, SpecularColor.Z, 1.0f);
				MaterialConstants.Ns = Material->GetShininess();
				MaterialConstants.Ni = Material->GetRefractiveIndex();
				MaterialConstants.D = (MaterialConstants.D == NULL) ? 1 :Material->GetOpacity();
				MaterialConstants.MaterialFlags = 0;

				if (Material->GetDiffuseTexture()) { MaterialConstants.MaterialFlags |= HAS_DIFFUSE_MAP; }
				if (Material->GetAmbientTexture()) { MaterialConstants.MaterialFlags |= HAS_AMBIENT_MAP; }
				if (Material->GetSpecularTexture()) { MaterialConstants.MaterialFlags |= HAS_SPECULAR_MAP; }
				if (Material->GetNormalTexture()) { MaterialConstants.MaterialFlags |= HAS_NORMAL_MAP; }
				if (Material->GetOpacityTexture()) { MaterialConstants.MaterialFlags |= HAS_ALPHA_MAP; }
				if (Material->GetBumpTexture()) { MaterialConstants.MaterialFlags |= HAS_BUMP_MAP; }
				MaterialConstants.Time = 0.0f;

				FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferMaterial, MaterialConstants);
				Pipeline->SetConstantBuffer(2, EShaderType::VS | EShaderType::PS, ConstantBufferMaterial);

				if (UTexture* DiffuseTexture = Material->GetDiffuseTexture())
				{
					Pipeline->SetShaderResourceView(0, EShaderType::PS, DiffuseTexture->GetTextureSRV());
					Pipeline->SetSamplerState(0, EShaderType::PS, DiffuseTexture->GetTextureSampler());
				}
				if (UTexture* AmbientTexture = Material->GetAmbientTexture())
				{
					Pipeline->SetShaderResourceView(1, EShaderType::PS, AmbientTexture->GetTextureSRV());
				}
				if (UTexture* SpecularTexture = Material->GetSpecularTexture())
				{
					Pipeline->SetShaderResourceView(2, EShaderType::PS, SpecularTexture->GetTextureSRV());
				}
				if (UTexture* NormalTexture = Material->GetNormalTexture())
				{
					Pipeline->SetShaderResourceView(3, EShaderType::PS, NormalTexture->GetTextureSRV());
				}
				if (UTexture* AlphaTexture = Material->GetOpacityTexture())
				{
					Pipeline->SetShaderResourceView(4, EShaderType::PS, AlphaTexture->GetTextureSRV());
				}
				if (UTexture* BumpTexture = Material->GetBumpTexture())
				{
					Pipeline->SetShaderResourceView(5, EShaderType::PS, BumpTexture->GetTextureSRV());
				}
			}

			// DrawIndexed (offset 사용)
			Pipeline->DrawIndexed(static_cast<uint32>(Section.Indices.Num()), CurrentIndexOffset, 0);

			CurrentIndexOffset += static_cast<uint32>(Section.Indices.Num());
		}

		// 버퍼 해제
		SafeRelease(DynamicVB);
		SafeRelease(DynamicIB);
	}

	Pipeline->SetConstantBuffer(2, EShaderType::PS, nullptr);

	// Unbind shadow maps
	Pipeline->SetShaderResourceView(10, EShaderType::PS, nullptr);
	Pipeline->SetShaderResourceView(11, EShaderType::PS, nullptr);
	Pipeline->SetShaderResourceView(12, EShaderType::PS, nullptr);
	Pipeline->SetShaderResourceView(13, EShaderType::PS, nullptr);
	Pipeline->SetShaderResourceView(14, EShaderType::PS, nullptr);
}

void FSkeletalMeshPass::Release()
{
	SafeRelease(ConstantBufferMaterial);
}
