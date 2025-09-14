#include "pch.h"
#include "Render/Renderer/Public/Renderer.h"

#include "Level/Public/Level.h"

#include "Manager/Level/Public/LevelManager.h"
#include "Manager/UI/Public/UIManager.h"
#include "Mesh/Public/Actor.h"
#include "Mesh/Public/PrimitiveComponent.h"
#include "Render/Renderer/Public/Pipeline.h"
#include "Editor/Public/Editor.h"

IMPLEMENT_SINGLETON(URenderer)

URenderer::URenderer() = default;

URenderer::~URenderer() = default;

void URenderer::Init(HWND InWindowHandle)
{
	DeviceResources = new UDeviceResources(InWindowHandle);
	Pipeline = new UPipeline(GetDeviceContext());

	// 래스터라이저 상태 생성
	CreateRasterizerState();
	CreateDepthStencilState();
	CreateDefaultShader();
	CreateConstantBuffer();
}

void URenderer::Release()
{
	ReleaseConstantBuffer();
	ReleaseDefaultShader();
	ReleaseResource();

	SafeDelete(Pipeline);
	SafeDelete(DeviceResources);
}

/**
 * @brief 래스터라이저 상태를 생성하는 함수
 */
void URenderer::CreateRasterizerState()
{

}

void URenderer::CreateDepthStencilState()
{
	D3D11_DEPTH_STENCIL_DESC DescDefault = {};

	DescDefault.DepthEnable = TRUE;
	DescDefault.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;

	DescDefault.DepthFunc = D3D11_COMPARISON_LESS;

	DescDefault.StencilEnable = FALSE;
	DescDefault.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	DescDefault.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	HRESULT hr = DeviceResources->GetDevice()->CreateDepthStencilState(
		&DescDefault,
		&DefaultDepthStencilState
	);

	D3D11_DEPTH_STENCIL_DESC descDisabled = {};

	descDisabled.DepthEnable = FALSE;

	DescDefault.StencilEnable = FALSE;
	DescDefault.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	DescDefault.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	hr = DeviceResources->GetDevice()->CreateDepthStencilState(
		&descDisabled,
		&DisabledDepthStencilState
	);
}

/**
 * @brief 래스터라이저 상태를 해제하는 함수
 */
void URenderer::ReleaseRasterizerState()
{
	for (auto& Cache : RasterCache)
	{
		if (Cache.second != nullptr)
		{
			Cache.second->Release();
		}
	}
	RasterCache.clear();
}

/**
 * @brief 렌더러에 사용된 모든 리소스를 해제하는 함수
 */
void URenderer::ReleaseResource()
{
	for (auto& Cache : RasterCache)
	{
		if (Cache.second != nullptr)
		{
			Cache.second->Release();
		}
	}
	RasterCache.clear();

	if (DefaultDepthStencilState)
	{
		DefaultDepthStencilState->Release();
		DefaultDepthStencilState = nullptr;
	}

	if (DisabledDepthStencilState)
	{
		DisabledDepthStencilState->Release();
		DisabledDepthStencilState = nullptr;
	}

	// 렌더 타겟을 초기화
	if (GetDeviceContext())
	{
		GetDeviceContext()->OMSetRenderTargets(0, nullptr, nullptr);
	}
}

/**
 * @brief Shader 기반의 CSO 생성 함수
 */
void URenderer::CreateDefaultShader()
{
	ID3DBlob* VertexShaderCSO;
	ID3DBlob* PixelShaderCSO;

	D3DCompileFromFile(L"Asset/Shader/SampleShader.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0,
	                   &VertexShaderCSO, nullptr);

	GetDevice()->CreateVertexShader(VertexShaderCSO->GetBufferPointer(),
	                           VertexShaderCSO->GetBufferSize(), nullptr, &DefaultVertexShader);

	D3DCompileFromFile(L"Asset/Shader/SampleShader.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0,
	                   &PixelShaderCSO, nullptr);

	GetDevice()->CreatePixelShader(PixelShaderCSO->GetBufferPointer(),
	                          PixelShaderCSO->GetBufferSize(), nullptr, &DefaultPixelShader);

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	GetDevice()->CreateInputLayout(layout, ARRAYSIZE(layout), VertexShaderCSO->GetBufferPointer(),
	                          VertexShaderCSO->GetBufferSize(), &DefaultInputLayout);

	Stride = sizeof(FVertex);

	VertexShaderCSO->Release();
	PixelShaderCSO->Release();
}

/**
 * @brief Shader Release
 */
void URenderer::ReleaseDefaultShader()
{
	if (DefaultInputLayout)
	{
		DefaultInputLayout->Release();
		DefaultInputLayout = nullptr;
	}

	if (DefaultPixelShader)
	{
		DefaultPixelShader->Release();
		DefaultPixelShader = nullptr;
	}

	if (DefaultVertexShader)
	{
		DefaultVertexShader->Release();
		DefaultVertexShader = nullptr;
	}
}

void URenderer::Update(UEditor* Editor)
{
	RenderBegin();

	RenderLevel();
	Editor->RenderEditor();

	//RenderLines();

	UUIManager::GetInstance().Render();

	RenderEnd();
}

/**
 * @brief Render Prepare Step
 */
void URenderer::RenderBegin()
{
	auto* rtv = DeviceResources->GetRenderTargetView();
	GetDeviceContext()->ClearRenderTargetView(rtv, ClearColor);
	auto* dsv = DeviceResources->GetDepthStencilView();
	GetDeviceContext()->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

	GetDeviceContext()->RSSetViewports(1, &DeviceResources->GetViewportInfo());

	ID3D11RenderTargetView* rtvs[] = { rtv };  // 배열 생성

	GetDeviceContext()->OMSetRenderTargets(1, rtvs, DeviceResources->GetDepthStencilView());
	DeviceResources->UpdateViewport();
}

/**
 * @brief Buffer에 데이터 입력 및 Draw
 */
void URenderer::RenderLevel()
{
	//
	// 여기에 카메라 VP 업데이트 한 번 싹
	//
	if (!ULevelManager::GetInstance().GetCurrentLevel())
		return;

	for (auto& PrimitiveComponent : ULevelManager::GetInstance().GetCurrentLevel()->GetLevelPrimitiveComponents())
	{
		if (!PrimitiveComponent) { continue; }
		///////////////////////////////////////////////////////////////////////////////////////////////////////
		ID3D11RasterizerState* LoadedRasterizerState = GetRasterizerState(PrimitiveComponent->GetRenderState());
		FPipelineInfo PipelineInfo = {
			DefaultInputLayout,
			DefaultVertexShader,
			LoadedRasterizerState,
			DefaultDepthStencilState,
			DefaultPixelShader,
			nullptr,
		};
		///////////////////////////////////////////////////////////////////////////////////////////////////////
		Pipeline->UpdatePipeline(PipelineInfo);

		Pipeline->SetConstantBuffer(0, true, ConstantBufferModels);
		UpdateConstant(
			PrimitiveComponent->GetRelativeLocation(),
			PrimitiveComponent->GetRelativeRotation(),
			PrimitiveComponent->GetRelativeScale3D() );

		Pipeline->SetConstantBuffer(2, true, ConstantBufferColor);
		UpdateConstant(PrimitiveComponent->GetColor());

		Pipeline->SetVertexBuffer(PrimitiveComponent->GetVertexBuffer(), Stride);
		Pipeline->Draw(static_cast<uint32>(PrimitiveComponent->GetVerticesData()->size()), 0);
	}
}

/**
 * @brief 스왑 체인의 백 버퍼와 프론트 버퍼를 교체하여 화면에 출력
 */
void URenderer::RenderEnd() const
{
	GetSwapChain()->Present(0, 0); // 1: VSync 활성화
}

static inline D3D11_CULL_MODE ToD3D11(ECullMode InCull)
{
	switch (InCull) {
	case ECullMode::Back:
		return D3D11_CULL_BACK;
	case ECullMode::Front:
		return D3D11_CULL_FRONT;
	case ECullMode::None:
		return D3D11_CULL_NONE;
	default:
		return D3D11_CULL_BACK;
	}
}

static inline D3D11_FILL_MODE ToD3D11(EFillMode InFill)
{
	switch (InFill) {
	case EFillMode::Solid:
		return D3D11_FILL_SOLID;
	case EFillMode::WireFrame:
		return D3D11_FILL_WIREFRAME;
	default:
		return D3D11_FILL_SOLID;
	}
}

void URenderer::RenderPrimitive(FEditorPrimitive& Primitive, struct FRenderState& InRenderState)
{
	ID3D11DepthStencilState* DepthStencilState =
		Primitive.bShouldAlwaysVisible ? DisabledDepthStencilState : DefaultDepthStencilState;

	ID3D11RasterizerState* RasterizerState =
		GetRasterizerState(InRenderState);

	FPipelineInfo PipelineInfo = {
			DefaultInputLayout,
			DefaultVertexShader,
			RasterizerState,
			DepthStencilState,
			DefaultPixelShader,
			nullptr,
			Primitive.Topology
	};

	Pipeline->UpdatePipeline(PipelineInfo);

	Pipeline->SetConstantBuffer(0, true, ConstantBufferModels);
	UpdateConstant(Primitive.Location, Primitive.Rotation, Primitive.Scale);

	Pipeline->SetConstantBuffer(2, true, ConstantBufferColor);
	UpdateConstant(Primitive.Color);

	Pipeline->SetVertexBuffer(Primitive.Vertexbuffer, Stride);
	Pipeline->Draw(Primitive.NumVertices, 0);
}

void URenderer::RenderPrimitiveIndexed(FEditorPrimitive& InPrimitive, FRenderState& InRenderState, bool bUseBaseConstantBuffer, uint32 stride, uint32 indexBufferStride)
{
	ID3D11DepthStencilState* DepthStencilState =
		InPrimitive.bShouldAlwaysVisible ? DisabledDepthStencilState : DefaultDepthStencilState;

	ID3D11RasterizerState* RasterizerState =
		GetRasterizerState(InRenderState);

	ID3D11InputLayout* inputLayout = InPrimitive.InputLayout ? InPrimitive.InputLayout : DefaultInputLayout;
	ID3D11VertexShader* vertexShader = InPrimitive.VertexShader ? InPrimitive.VertexShader : DefaultVertexShader;
	ID3D11PixelShader* pixelShader = InPrimitive.PixelShader ? InPrimitive.PixelShader : DefaultPixelShader;

	FPipelineInfo PipelineInfo = {
			inputLayout,
			vertexShader,
			RasterizerState,
			DepthStencilState,
			pixelShader,
			nullptr,
			InPrimitive.Topology
	};

	Pipeline->UpdatePipeline(PipelineInfo);

	if (bUseBaseConstantBuffer)
	{
		Pipeline->SetConstantBuffer(0, true, ConstantBufferModels);
		UpdateConstant(InPrimitive.Location, InPrimitive.Rotation, InPrimitive.Scale);

		Pipeline->SetConstantBuffer(2, true, ConstantBufferColor);
		UpdateConstant(InPrimitive.Color);
	}

	Pipeline->SetIndexBuffer(InPrimitive.IndexBuffer, indexBufferStride);
	Pipeline->SetVertexBuffer(InPrimitive.Vertexbuffer, stride);
	Pipeline->DrawIndexed(InPrimitive.NumIndices, 0, 0);
}

/**
 * @brief 정점 Buffer 생성 함수
 * @param InVertices
 * @param InByteWidth
 * @return
 */
ID3D11Buffer* URenderer::CreateVertexBuffer(FVertex* InVertices, uint32 InByteWidth) const
{
	// 2. Create a vertex buffer
	D3D11_BUFFER_DESC VertexBufferDesc = {};
	VertexBufferDesc.ByteWidth = InByteWidth;
	VertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // will never be updated
	VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA VertexBufferSRD = {InVertices};

	ID3D11Buffer* vertexBuffer;

	GetDevice()->CreateBuffer(&VertexBufferDesc, &VertexBufferSRD, &vertexBuffer);

	return vertexBuffer;
}

ID3D11Buffer* URenderer::CreateVertexBuffer(FVector* InVertices, uint32 InByteWidth, bool bCpuAccess) const
{
	// 2. Create a vertex buffer
	D3D11_BUFFER_DESC VertexBufferDesc = {};
	VertexBufferDesc.ByteWidth = InByteWidth;
	VertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // will never be updated
	VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	if (bCpuAccess)
	{
		VertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC; // CPU에서 자주 수정할 경우
		VertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // CPU 쓰기 가능
		VertexBufferDesc.MiscFlags = 0;
	}
	

	D3D11_SUBRESOURCE_DATA VertexBufferSRD = { InVertices };

	ID3D11Buffer* vertexBuffer;

	GetDevice()->CreateBuffer(&VertexBufferDesc, &VertexBufferSRD, &vertexBuffer);

	return vertexBuffer;
}

/**
 * @brief Index Buffer 생성 함수
 * @param InIndices
 * @param InByteWidth
 * @return
 */
ID3D11Buffer* URenderer::CreateIndexBuffer(const void* InIndices, uint32 InByteWidth) const
{
	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = InByteWidth;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA srd = {};
	srd.pSysMem = InIndices;

	ID3D11Buffer* buffer = nullptr;
	GetDevice()->CreateBuffer(&desc, &srd, &buffer);
	return buffer;
}

void URenderer::OnResize(uint32 InWidth, uint32 InHeight)
{
	if (!DeviceResources || !GetDevice() || !GetDeviceContext() || !GetSwapChain()) return;

	DeviceResources->ReleaseFrameBuffer();
	DeviceResources->ReleaseDepthBuffer();
	GetDeviceContext()->OMSetRenderTargets(0, nullptr, nullptr);

	// ResizeBuffers 호출
	HRESULT hr = GetSwapChain()->ResizeBuffers(2, InWidth, InHeight, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(hr))
	{
		UE_LOG("OnResize Failed");
		return;
	}
	DeviceResources->UpdateViewport();

	DeviceResources->CreateFrameBuffer();
	DeviceResources->CreateDepthBuffer();

	auto* rtv = DeviceResources->GetRenderTargetView();
	ID3D11RenderTargetView* rtvs[] = { rtv };  // 배열 생성
	GetDeviceContext()->OMSetRenderTargets(1, rtvs, DeviceResources->GetDepthStencilView());
}

/**
 * @brief Vertex Buffer 소멸 함수
 * @param InVertexBuffer
 */
void URenderer::ReleaseVertexBuffer(ID3D11Buffer* InVertexBuffer)
{
	InVertexBuffer->Release();
}

void URenderer::CreateVertexShaderAndInputLayout(const wstring& filePath, const TArray<D3D11_INPUT_ELEMENT_DESC>& inputLayoutDescs, ID3D11VertexShader** outVertexShader, ID3D11InputLayout** outInputLayout)
{
	ID3DBlob* VertexShaderCSO;
	ID3DBlob* errorBlob = nullptr;

	HRESULT hr = D3DCompileFromFile(filePath.c_str(), nullptr, nullptr, "mainVS", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
		&VertexShaderCSO, &errorBlob);

	if (FAILED(hr)) {
		if (errorBlob) {
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			errorBlob->Release();
		}
		// 실패 처리 (return 등)
		return;
	}

	GetDevice()->CreateVertexShader(VertexShaderCSO->GetBufferPointer(),
		VertexShaderCSO->GetBufferSize(), nullptr, outVertexShader);

	/*D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};*/

	GetDevice()->CreateInputLayout(inputLayoutDescs.data(), UINT(inputLayoutDescs.size()), VertexShaderCSO->GetBufferPointer(),
		VertexShaderCSO->GetBufferSize(), outInputLayout);

	Stride = sizeof(FVertex);

	VertexShaderCSO->Release();
}

void URenderer::CreatePixelShader(const wstring& filePath, ID3D11PixelShader** pixelShader)
{
	ID3DBlob* PixelShaderCSO;
	ID3DBlob* errorBlob = nullptr;

	HRESULT hr = D3DCompileFromFile(filePath.c_str(), nullptr, nullptr, "mainPS", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
		&PixelShaderCSO, &errorBlob);

	if (FAILED(hr)) {
		if (errorBlob) {
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			errorBlob->Release();
		}
		// 실패 처리 (return 등)
		return;
	}

	GetDevice()->CreatePixelShader(PixelShaderCSO->GetBufferPointer(),
		PixelShaderCSO->GetBufferSize(), nullptr, pixelShader);

	PixelShaderCSO->Release();
}

/**
 * @brief 상수 버퍼 생성 함수
 */
void URenderer::CreateConstantBuffer()
{
	/**
	 * @brief 모델에 사용될 상수 버퍼 생성
	 */
	{
		D3D11_BUFFER_DESC ConstantBufferDesc = {};
		ConstantBufferDesc.ByteWidth = sizeof(FMatrix) + 0xf & 0xfffffff0;
		// ensure constant buffer size is multiple of 16 bytes
		ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC; // will be updated from CPU every frame
		ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&ConstantBufferDesc, nullptr, &ConstantBufferModels);
	}

	/**
	 * @brief 색상 수정에 사용할 상수 버퍼
	 */
	{
		D3D11_BUFFER_DESC ConstantBufferDesc = {};
		ConstantBufferDesc.ByteWidth = sizeof(FVector4) + 0xf & 0xfffffff0;
		// ensure constant buffer size is multiple of 16 bytes
		ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC; // will be updated from CPU every frame
		ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&ConstantBufferDesc, nullptr, &ConstantBufferColor);
	}

	/**
	 * @brief 카메라에 사용될 상수 버퍼 생성
	 */
	{
		D3D11_BUFFER_DESC ConstantBufferDesc = {};
		ConstantBufferDesc.ByteWidth = sizeof(FViewProjConstants) + 0xf & 0xfffffff0;
		// ensure constant buffer size is multiple of 16 bytes
		ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC; // will be updated from CPU every frame
		ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&ConstantBufferDesc, nullptr, &ConstantBufferViewProj);
	}
}

/**
 * @brief 상수 버퍼 소멸 함수
 */
void URenderer::ReleaseConstantBuffer()
{
	if (ConstantBufferModels)
	{
		ConstantBufferModels->Release();
		ConstantBufferModels = nullptr;
	}

	if (ConstantBufferColor)
	{
		ConstantBufferColor->Release();
		ConstantBufferColor = nullptr;
	}

	if (ConstantBufferViewProj)
	{
		ConstantBufferViewProj->Release();
		ConstantBufferViewProj = nullptr;
	}
}


void URenderer::UpdateConstant(const UPrimitiveComponent* Primitive)
{
	if (ConstantBufferModels)
	{
		D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

		GetDeviceContext()->Map(ConstantBufferModels, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
		// update constant buffer every frame
		FMatrix* constants = (FMatrix*)constantbufferMSR.pData;
		{
			*constants = FMatrix::GetModelMatrix(Primitive->GetRelativeLocation(), FVector::GetDegreeToRadian(Primitive->GetRelativeRotation()), Primitive->GetRelativeScale3D());
		}
		GetDeviceContext()->Unmap(ConstantBufferModels, 0);
	}
}
/**
 * @brief 상수 버퍼 업데이트 함수
 * @param InOffset
 * @param InScale Ball Size
 */
void URenderer::UpdateConstant(const FVector& InPosition, const FVector& InRotation, const FVector& InScale) const
{
	if (ConstantBufferModels)
	{
		D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

		GetDeviceContext()->Map(ConstantBufferModels, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
		// update constant buffer every frame
		FMatrix* constants = (FMatrix*)constantbufferMSR.pData;
		{
			*constants = FMatrix::GetModelMatrix(InPosition, FVector::GetDegreeToRadian(InRotation), InScale);
		}
		GetDeviceContext()->Unmap(ConstantBufferModels, 0);
	}
}

void URenderer::UpdateConstant(const FViewProjConstants& InViewProjConstants) const
{
	Pipeline->SetConstantBuffer(1, true, ConstantBufferViewProj);

	if (ConstantBufferViewProj)
	{
		D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR = {};

		GetDeviceContext()->Map(ConstantBufferViewProj, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR);
		// update constant buffer every frame
		FViewProjConstants* ViewProjectionConstants = (FViewProjConstants*)ConstantBufferMSR.pData;
		{
			ViewProjectionConstants->View = InViewProjConstants.View;
			ViewProjectionConstants->Projection = InViewProjConstants.Projection;
		}
		GetDeviceContext()->Unmap(ConstantBufferViewProj, 0);
	}
}

void URenderer::UpdateConstant(const FVector4& Color) const
{
	Pipeline->SetConstantBuffer(2, false, ConstantBufferColor);

	if (ConstantBufferColor)
	{
		D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR = {};

		GetDeviceContext()->Map(ConstantBufferColor, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR);
		// update constant buffer every frame
		FVector4* ColorConstants = (FVector4*)ConstantBufferMSR.pData;
		{
			ColorConstants->X = Color.X;
			ColorConstants->Y = Color.Y;
			ColorConstants->Z = Color.Z;
			ColorConstants->W = Color.W;
		}
		GetDeviceContext()->Unmap(ConstantBufferColor, 0);
	}
}

bool URenderer::UpdateVertexBuffer(ID3D11Buffer* vertexBuffer, const std::vector<FVector>& vertices)
{
	if (!GetDeviceContext() || !vertexBuffer || vertices.empty())
		return false;

	D3D11_MAPPED_SUBRESOURCE mappedResource = {};
	HRESULT hr = GetDeviceContext()->Map(
		vertexBuffer,
		0,                          // 서브리소스 인덱스 (버퍼는 0)
		D3D11_MAP_WRITE_DISCARD,    // 전체 갱신
		0,                          // 플래그 없음
		&mappedResource
	);

	if (FAILED(hr))
		return false;

	// GPU 메모리에 새 데이터 복사
	// to do: 어쩔 때 한번 read access violation 걸림
	memcpy(mappedResource.pData, vertices.data(), sizeof(FVector) * vertices.size());

	// GPU 접근 재허용
	GetDeviceContext()->Unmap(vertexBuffer, 0);

	return true;
}

//void URenderer::UpdateAndSetBatchLineConstant(const BatchLineContants& inBatchLineConstant) const
//{
//	Pipeline->SetConstantBuffer(3, true, ConstantBufferBatchLine);
//
//	if (ConstantBufferBatchLine)
//	{
//		D3D11_MAPPED_SUBRESOURCE ConstantBufferMSR = {};
//
//		GetDeviceContext()->Map(ConstantBufferBatchLine, 0, D3D11_MAP_WRITE_DISCARD, 0, &ConstantBufferMSR);
//		// update constant buffer every frame
//		BatchLineContants* batchLineConstant = (BatchLineContants*)ConstantBufferMSR.pData;
//		{
//			*batchLineConstant = inBatchLineConstant;
//		}
//		GetDeviceContext()->Unmap(ConstantBufferBatchLine, 0);
//	}
//}

//void URenderer::UpdateBatchLineConstant(const UPrimitiveComponent* Primitive, const BatchLineContants& batchLineConstant) const
//{
//	if (ConstantBufferBatchLine)
//	{
//		D3D11_MAPPED_SUBRESOURCE constantbufferMSR;
//
//		GetDeviceContext()->Map(ConstantBufferBatchLine, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
//		// update constant buffer every frame
//		FMatrix* constants = (FMatrix*)constantbufferMSR.pData;
//		{
//			*constants = FMatrix::GetModelMatrix(Primitive->GetRelativeLocation(), FVector::GetDegreeToRadian(Primitive->GetRelativeRotation()), Primitive->GetRelativeScale3D());
//		}
//		GetDeviceContext()->Unmap(ConstantBufferBatchLine, 0);
//	}
//}

ID3D11RasterizerState* URenderer::GetRasterizerState(const FRenderState& InRenderState)
{
	D3D11_FILL_MODE FillMode = ToD3D11(InRenderState.FillMode);
	D3D11_CULL_MODE CillMode = ToD3D11(InRenderState.CullMode);

	const FRasterKey Key{ FillMode, CillMode };
	if (auto It = RasterCache.find(Key); It != RasterCache.end())
		return It->second;

	ID3D11RasterizerState* RasterizerState = nullptr;
	D3D11_RASTERIZER_DESC RasterizerDesc = {};
	RasterizerDesc.FillMode = FillMode;
	RasterizerDesc.CullMode = CillMode;
	RasterizerDesc.DepthClipEnable = TRUE; // ✅ 근/원거리 평면 클리핑 활성화 (핵심)

	HRESULT Hr = GetDevice()->CreateRasterizerState(&RasterizerDesc, &RasterizerState);

	if (FAILED(Hr)) { return nullptr; }

	RasterCache.emplace(Key, RasterizerState);
	return RasterizerState;
}
