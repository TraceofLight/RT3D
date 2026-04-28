#include "pch.h"

#include "RenderingStats.h"
#include "UI/StatsOverlayD2D.h"

struct FConstants
{
    FVector WorldPosition;
    float Scale;
};
// b0 in VS
struct ModelBufferType
{
    FMatrix Model;
};

// b0 in PS
struct FMaterialInPs
{
    FVector DiffuseColor; // Kd
    float OpticalDensity; // Ni

    FVector AmbientColor; // Ka
    float Transparency; // Tr Or d

    FVector SpecularColor; // Ks
    float SpecularExponent; // Ns

    FVector EmissiveColor; // Ke
    uint32 IlluminationModel; // illum. Default illumination model to Phong for non-Pbr materials

    FVector TransmissionFilter; // Tf
    float dummy; // 4 bytes padding
};

struct FPixelConstBufferType
{
    FMaterialInPs Material;
    bool bHasMaterial; // 1 bytes
    bool Dummy[3]; // 3 bytes padding
    bool bHasTexture; // 1 bytes
    bool Dummy2[11]; // 11 bytes padding
};

static_assert(sizeof(FPixelConstBufferType) % 16 == 0, "PixelConstData size mismatch!");

// b1
struct ViewProjBufferType
{
    FMatrix View;
    FMatrix Proj;
};

// b2
struct HighLightBufferType
{
    uint32 Picked;
    FVector Color;
    uint32 X;
    uint32 Y;
    uint32 Z;
    uint32 Gizmo;
};

struct ColorBufferType
{
    FVector4 Color;
};


struct BillboardBufferType
{
    FVector pos;
    FMatrix View;
    FMatrix Proj;
    FMatrix InverseViewMat;
    /*FVector cameraRight;
    FVector cameraUp;*/
};

void D3D11RHI::Initialize(HWND hWindow)
{
    // 이곳에서 Device, DeviceContext, viewport, swapchain를 초기화한다
    CreateDeviceAndSwapChain(hWindow);
    CreateFrameBuffer();
    CreateRasterizerState();
    CreateBlendState();
    CreateConstantBuffer();
    CreateDepthStencilState();
    CreateSamplerState();
    UResourceManager::GetInstance().Initialize(Device, DeviceContext);

    // Initialize Direct2D overlay after device/swapchain ready
    UStatsOverlayD2D::Get().Initialize(Device, DeviceContext, SwapChain);
}

void D3D11RHI::Release()
{
    if (DeviceContext)
    {
        // 파이프라인에서 바인딩된 상태/리소스를 명시적으로 해제
        DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
        DeviceContext->OMSetDepthStencilState(nullptr, 0);
        DeviceContext->RSSetState(nullptr);
        DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

        DeviceContext->ClearState();
        DeviceContext->Flush();
    }

    ReleaseSamplerState();

    // 상수버퍼
    if (HighLightCB)
    {
        HighLightCB->Release();
        HighLightCB = nullptr;
    }
    if (ModelCB)
    {
        ModelCB->Release();
        ModelCB = nullptr;
    }
    if (ColorCB)
    {
        ColorCB->Release();
        ColorCB = nullptr;
    }
    if (ViewProjCB)
    {
        ViewProjCB->Release();
        ViewProjCB = nullptr;
    }
    if (BillboardCB)
    {
        BillboardCB->Release();
        BillboardCB = nullptr;
    }
    if (PixelConstCB)
    {
        PixelConstCB->Release();
        PixelConstCB = nullptr;
    }
    if (UVScrollCB)
    {
        UVScrollCB->Release();
        UVScrollCB = nullptr;
    }
    if (ConstantBuffer)
    {
        ConstantBuffer->Release();
        ConstantBuffer = nullptr;
    }

    // 상태 객체
    if (DepthStencilState)
    {
        DepthStencilState->Release();
        DepthStencilState = nullptr;
    }
    if (DepthStencilStateLessEqualWrite)
    {
        DepthStencilStateLessEqualWrite->Release();
        DepthStencilStateLessEqualWrite = nullptr;
    }
    if (DepthStencilStateLessEqualReadOnly)
    {
        DepthStencilStateLessEqualReadOnly->Release();
        DepthStencilStateLessEqualReadOnly = nullptr;
    }
    if (DepthStencilStateAlwaysNoWrite)
    {
        DepthStencilStateAlwaysNoWrite->Release();
        DepthStencilStateAlwaysNoWrite = nullptr;
    }
    if (DepthStencilStateDisable)
    {
        DepthStencilStateDisable->Release();
        DepthStencilStateDisable = nullptr;
    }
    if (DepthStencilStateGreaterEqualWrite)
    {
        DepthStencilStateGreaterEqualWrite->Release();
        DepthStencilStateGreaterEqualWrite = nullptr;
    }

    if (DefaultRasterizerState)
    {
        DefaultRasterizerState->Release();
        DefaultRasterizerState = nullptr;
    }
    if (WireFrameRasterizerState)
    {
        WireFrameRasterizerState->Release();
        WireFrameRasterizerState = nullptr;
    }
    if (BlendState)
    {
        BlendState->Release();
        BlendState = nullptr;
    }

    // RTV/DSV/FrameBuffer
    ReleaseFrameBuffer();

    // Device + SwapChain
    ReleaseDeviceAndSwapChain();
}

void D3D11RHI::BeginFrame()
{
    // 프레임 시작 시 수행할 작업들
    // 렌더 타겟 설정
    DeviceContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);

    // 백버퍼/깊이버퍼를 클리어
    ClearBackBuffer(); // 배경색
    ClearDepthBuffer(1.0f, 0); // 깊이값 초기화
    CreateBlendState();
    IASetPrimitiveTopology();

    // RS
    RSSetViewport();

    //OM
    OMSetRenderTargets();
}

void D3D11RHI::EndFrame()
{
    // 렌더링 통계 수집 종료
    URenderingStatsCollector& StatsCollector = URenderingStatsCollector::GetInstance();
    StatsCollector.EndFrame();
    
    // 현재 프레임 통계를 업데이트
    const FRenderingStats& CurrentStats = StatsCollector.GetCurrentFrameStats();
    StatsCollector.UpdateFrameStats(CurrentStats);
    
    // 평균 통계를 얻어서 오버레이에 업데이트
    const FRenderingStats& AvgStats = StatsCollector.GetAverageStats();
    UStatsOverlayD2D::Get().UpdateRenderingStats(
        AvgStats.TotalDrawCalls,
        AvgStats.MaterialChanges,
        AvgStats.TextureChanges,
        AvgStats.ShaderChanges
    );
    
    Present();
}

void D3D11RHI::ClearBackBuffer()
{
    float ClearColor[4] = {0.025f, 0.025f, 0.025f, 1.0f};
    DeviceContext->ClearRenderTargetView(RenderTargetView, ClearColor);
}

void D3D11RHI::ClearDepthBuffer(float Depth, UINT Stencil)
{
    DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                                         Depth, Stencil);
}

void D3D11RHI::CreateBlendState()
{
    // Create once; reuse every frame
    if (BlendState)
        return;

    D3D11_BLEND_DESC bd = {};
    auto& rt = bd.RenderTarget[0];
    rt.BlendEnable = TRUE;
    rt.SrcBlend = D3D11_BLEND_SRC_ALPHA; // 스트레이트 알파
    rt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA; // (프리멀티면 ONE / INV_SRC_ALPHA)
    rt.BlendOp = D3D11_BLEND_OP_ADD;
    rt.SrcBlendAlpha = D3D11_BLEND_ONE;
    rt.DestBlendAlpha = D3D11_BLEND_ZERO;
    rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    Device->CreateBlendState(&bd, &BlendState);
}

void D3D11RHI::CreateDepthStencilState()
{
    D3D11_DEPTH_STENCIL_DESC desc = {};
    desc.StencilEnable = FALSE;

    // 1) 기본: LessEqual + Write ALL
    desc.DepthEnable = TRUE;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    Device->CreateDepthStencilState(&desc, &DepthStencilStateLessEqualWrite);

    // 2) ReadOnly: LessEqual + Write ZERO
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    Device->CreateDepthStencilState(&desc, &DepthStencilStateLessEqualReadOnly);

    // 3) AlwaysNoWrite: Always + Write ZERO (기즈모/오버레이 용)
    desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    // DepthEnable은 TRUE 유지 (읽기 의미는 없지만 상태 일관성을 위해)
    Device->CreateDepthStencilState(&desc, &DepthStencilStateAlwaysNoWrite);

    // 4) Disable: DepthEnable FALSE (테스트/쓰기 모두 무시)
    desc.DepthEnable = FALSE;
    // DepthWriteMask/Func는 무시되지만 값은 그대로 둬도 됨
    Device->CreateDepthStencilState(&desc, &DepthStencilStateDisable);

    // 5) (선택) GreaterEqual + Write ALL
    desc.DepthEnable = TRUE;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
    Device->CreateDepthStencilState(&desc, &DepthStencilStateGreaterEqualWrite);
}

void D3D11RHI::CreateSamplerState()
{
    D3D11_SAMPLER_DESC SampleDesc = {};
    SampleDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    SampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    SampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    SampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    SampleDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    SampleDesc.MinLOD = 0;
    SampleDesc.MaxLOD = D3D11_FLOAT32_MAX;

    HRESULT HR = Device->CreateSamplerState(&SampleDesc, &DefaultSamplerState);
}

HRESULT D3D11RHI::CreateIndexBuffer(ID3D11Device* device, const FMeshData* meshData,
                                    ID3D11Buffer** outBuffer)
{
    if (!meshData || meshData->Indices.empty())
        return E_FAIL;

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = static_cast<UINT>(sizeof(uint32) * meshData->Indices.size());
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA iinitData = {};
    iinitData.pSysMem = meshData->Indices.data();

    return device->CreateBuffer(&ibd, &iinitData, outBuffer);
}

HRESULT D3D11RHI::CreateIndexBuffer(ID3D11Device* device, const FStaticMesh* mesh,
                                    ID3D11Buffer** outBuffer)
{
    if (!mesh || mesh->Indices.empty())
        return E_FAIL;

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = static_cast<UINT>(sizeof(uint32) * mesh->Indices.size());
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA iinitData = {};
    iinitData.pSysMem = mesh->Indices.data();

    return device->CreateBuffer(&ibd, &iinitData, outBuffer);
}

//이거 두개를 나눔
void D3D11RHI::UpdateConstantBuffers(const FMatrix& ModelMatrix, const FMatrix& ViewMatrix,
                                     const FMatrix& ProjMatrix)
{
    UpdateModelConstantBuffers(ModelMatrix);

    UpdateViewConstantBuffers(ViewMatrix, ProjMatrix);
}

void D3D11RHI::UpdateViewConstantBuffers(const FMatrix& ViewMatrix, const FMatrix& ProjMatrix)
{
    static FMatrix LastViewMatrix;
    static FMatrix LastProjectionMatrix;
    if (LastViewMatrix != ViewMatrix || LastProjectionMatrix != ProjMatrix)
    {
        LastViewMatrix = ViewMatrix;
        LastProjectionMatrix = ProjMatrix;
        D3D11_MAPPED_SUBRESOURCE mapped;
        DeviceContext->Map(ViewProjCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        auto* dataPtr = reinterpret_cast<ViewProjBufferType*>(mapped.pData);

        dataPtr->View = ViewMatrix;
        dataPtr->Proj = ProjMatrix;

        DeviceContext->Unmap(ViewProjCB, 0);
        DeviceContext->VSSetConstantBuffers(1, 1, &ViewProjCB); // b1 슬롯
    }
}

void D3D11RHI::UpdateModelConstantBuffers(const FMatrix& ModelMatrix)
{
    // b0 : 모델 행렬
    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        DeviceContext->Map(ModelCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        auto* dataPtr = reinterpret_cast<ModelBufferType*>(mapped.pData);

        // HLSL 기본 row-major와 맞추기 위해 전치
        dataPtr->Model = ModelMatrix;

        DeviceContext->Unmap(ModelCB, 0);
        DeviceContext->VSSetConstantBuffers(0, 1, &ModelCB); // b0 슬롯
    }
}

void D3D11RHI::UpdateBillboardConstantBuffers(const FVector& pos, const FMatrix& ViewMatrix,
                                              const FMatrix& ProjMatrix,
                                              const FVector& CameraRight, const FVector& CameraUp)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    DeviceContext->Map(BillboardCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    auto* dataPtr = reinterpret_cast<BillboardBufferType*>(mapped.pData);

    // HLSL 기본 row-major와 맞추기 위해 전치
    dataPtr->pos = pos;
    dataPtr->View = ViewMatrix;
    dataPtr->Proj = ProjMatrix;
    dataPtr->InverseViewMat = ViewMatrix.InverseAffine();
    //dataPtr->cameraRight = CameraRight;
    //dataPtr->cameraUp = CameraUp;

    DeviceContext->Unmap(BillboardCB, 0);
    DeviceContext->VSSetConstantBuffers(0, 1, &BillboardCB); // b0 슬롯
}

void D3D11RHI::UpdatePixelConstantBuffers(const FObjMaterialInfo& InMaterialInfo, bool bHasMaterial,
                                          bool bHasTexture)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    DeviceContext->Map(PixelConstCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    FPixelConstBufferType* dataPtr = reinterpret_cast<FPixelConstBufferType*>(mapped.pData);

    // 이후 다양한 material들이 맵핑될 수도 있음.
    dataPtr->bHasMaterial = bHasMaterial;
    dataPtr->bHasTexture = bHasTexture;
    dataPtr->Material.DiffuseColor = InMaterialInfo.DiffuseColor;
    dataPtr->Material.AmbientColor = InMaterialInfo.AmbientColor;

    DeviceContext->Unmap(PixelConstCB, 0);
    DeviceContext->PSSetConstantBuffers(4, 1, &PixelConstCB); // b4 슬롯
}

void D3D11RHI::UpdateHighLightConstantBuffers(const uint32 InPicked, const FVector& InColor,
                                              const uint32 X, const uint32 Y, const uint32 Z,
                                              const uint32 Gizmo)
{
    // b2 : 색 강조
    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        DeviceContext->Map(HighLightCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        auto* dataPtr = reinterpret_cast<HighLightBufferType*>(mapped.pData);

        dataPtr->Picked = InPicked;
        dataPtr->Color = InColor;
        dataPtr->X = X;
        dataPtr->Y = Y;
        dataPtr->Z = Z;
        dataPtr->Gizmo = Gizmo;
        DeviceContext->Unmap(HighLightCB, 0);
        DeviceContext->VSSetConstantBuffers(2, 1, &HighLightCB); // b2 슬롯
    }
}

void D3D11RHI::UpdateColorConstantBuffers(const FVector4& InColor)
{
    // b3 : 색 설정
    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        DeviceContext->Map(ColorCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        auto* dataPtr = reinterpret_cast<ColorBufferType*>(mapped.pData);
        {
            dataPtr->Color = InColor;
        }
        DeviceContext->Unmap(ColorCB, 0);
        DeviceContext->PSSetConstantBuffers(3, 1, &ColorCB); // b3 슬롯
    }
}

void D3D11RHI::IASetPrimitiveTopology()
{
    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void D3D11RHI::RSSetState(EViewModeIndex ViewModeIndex)
{
    if (ViewModeIndex == EViewModeIndex::VMI_Wireframe)
    {
        DeviceContext->RSSetState(WireFrameRasterizerState);
    }
    else
    {
        DeviceContext->RSSetState(DefaultRasterizerState);
    }
}

void D3D11RHI::RSSetViewport()
{
    DeviceContext->RSSetViewports(1, &ViewportInfo);
}

void D3D11RHI::OMSetRenderTargets()
{
    DeviceContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);
}

void D3D11RHI::OMSetBlendState(bool bIsBlendMode)
{
    if (bIsBlendMode == true)
    {
        float blendFactor[4] = {0, 0, 0, 0};
        DeviceContext->OMSetBlendState(BlendState, blendFactor, 0xffffffff);
    }
    else
    {
        DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    }
}

void D3D11RHI::Present()
{
    // Draw any Direct2D overlays before present
    UStatsOverlayD2D::Get().Draw();
    SwapChain->Present(0, 0); // vsync on
}

void D3D11RHI::CreateDeviceAndSwapChain(HWND hWindow)
{
    // 지원하는 Direct3D 기능 레벨을 정의
    D3D_FEATURE_LEVEL featurelevels[] = {D3D_FEATURE_LEVEL_11_0};

    // 스왑 체인 설정 구조체 초기화
    DXGI_SWAP_CHAIN_DESC swapchaindesc = {};
    swapchaindesc.BufferDesc.Width = 0; // 창 크기에 맞게 자동으로 설정
    swapchaindesc.BufferDesc.Height = 0; // 창 크기에 맞게 자동으로 설정
    swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // 색상 포맷
    swapchaindesc.SampleDesc.Count = 1; // 멀티 샘플링 비활성화
    swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 렌더 타겟으로 사용
    swapchaindesc.BufferCount = 2; // 더블 버퍼링
    swapchaindesc.OutputWindow = hWindow; // 렌더링할 창 핸들
    swapchaindesc.Windowed = TRUE; // 창 모드
    swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 스왑 방식

    // Direct3D 장치와 스왑 체인을 생성
    UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                               createDeviceFlags,
                                               featurelevels, ARRAYSIZE(featurelevels),
                                               D3D11_SDK_VERSION,
                                               &swapchaindesc, &SwapChain, &Device, nullptr,
                                               &DeviceContext);
    // 생성된 스왑 체인의 정보 가져오기
    SwapChain->GetDesc(&swapchaindesc);

    // 뷰포트 정보 설정
    ViewportInfo = {
        0.0f, 0.0f, (float)swapchaindesc.BufferDesc.Width, (float)swapchaindesc.BufferDesc.Height,
        0.0f, 1.0f
    };
}

void D3D11RHI::CreateFrameBuffer()
{
    // 백 버퍼 가져오기
    SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

    // 렌더 타겟 뷰 생성
    D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
    framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

    Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &RenderTargetView);

    // =====================================
    // 깊이/스텐실 버퍼 생성
    // =====================================
    DXGI_SWAP_CHAIN_DESC swapDesc;
    SwapChain->GetDesc(&swapDesc);

    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = swapDesc.BufferDesc.Width;
    depthDesc.Height = swapDesc.BufferDesc.Height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 깊이 24비트 + 스텐실 8비트
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* depthBuffer = nullptr;
    Device->CreateTexture2D(&depthDesc, nullptr, &depthBuffer);

    // DepthStencilView 생성
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = depthDesc.Format;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    Device->CreateDepthStencilView(depthBuffer, &dsvDesc, &DepthStencilView);

    depthBuffer->Release(); // 뷰만 참조 유지
}

void D3D11RHI::CreateRasterizerState()
{
    D3D11_RASTERIZER_DESC deafultrasterizerdesc = {};
    deafultrasterizerdesc.FillMode = D3D11_FILL_SOLID; // 채우기 모드
    deafultrasterizerdesc.CullMode = D3D11_CULL_BACK; // 백 페이스 컬링
    deafultrasterizerdesc.DepthClipEnable = TRUE; // 근/원거리 평면 클리핑

    Device->CreateRasterizerState(&deafultrasterizerdesc, &DefaultRasterizerState);

    D3D11_RASTERIZER_DESC wireframerasterizerdesc = {};
    wireframerasterizerdesc.FillMode = D3D11_FILL_WIREFRAME; // 채우기 모드
    wireframerasterizerdesc.CullMode = D3D11_CULL_BACK; // 백 페이스 컬링
    wireframerasterizerdesc.DepthClipEnable = TRUE; // 근/원거리 평면 클리핑

    Device->CreateRasterizerState(&wireframerasterizerdesc, &WireFrameRasterizerState);
}

void D3D11RHI::CreateConstantBuffer()
{
    D3D11_BUFFER_DESC modelDesc = {};
    modelDesc.Usage = D3D11_USAGE_DYNAMIC;
    modelDesc.ByteWidth = sizeof(ModelBufferType);
    modelDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    modelDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Device->CreateBuffer(&modelDesc, nullptr, &ModelCB);

    // b0 in StaticMeshPS
    D3D11_BUFFER_DESC pixelConstDesc = {};
    pixelConstDesc.Usage = D3D11_USAGE_DYNAMIC;
    pixelConstDesc.ByteWidth = sizeof(FPixelConstBufferType);
    pixelConstDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    pixelConstDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    HRESULT hr = Device->CreateBuffer(&pixelConstDesc, nullptr, &PixelConstCB);
    if (FAILED(hr))
    {
        assert(FAILED(hr));
    }

    // b1 : ViewProjBuffer
    D3D11_BUFFER_DESC vpDesc = {};
    vpDesc.Usage = D3D11_USAGE_DYNAMIC;
    vpDesc.ByteWidth = sizeof(ViewProjBufferType);
    vpDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    vpDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Device->CreateBuffer(&vpDesc, nullptr, &ViewProjCB);

    // b2 : HighLightBuffer  (← 기존 코드에서 vpDesc를 다시 써서 버그났던 부분)
    D3D11_BUFFER_DESC hlDesc = {};
    hlDesc.Usage = D3D11_USAGE_DYNAMIC;
    hlDesc.ByteWidth = sizeof(HighLightBufferType);
    hlDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    hlDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Device->CreateBuffer(&hlDesc, nullptr, &HighLightCB);

    D3D11_BUFFER_DESC billboardDesc = {};
    billboardDesc.Usage = D3D11_USAGE_DYNAMIC;
    billboardDesc.ByteWidth = sizeof(BillboardBufferType);
    billboardDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    billboardDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Device->CreateBuffer(&billboardDesc, nullptr, &BillboardCB);

    D3D11_BUFFER_DESC ColorDesc = {};
    ColorDesc.Usage = D3D11_USAGE_DYNAMIC;
    ColorDesc.ByteWidth = sizeof(ColorBufferType);
    ColorDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    ColorDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Device->CreateBuffer(&ColorDesc, nullptr, &ColorCB);

    D3D11_BUFFER_DESC uvScrollDesc = {};
    uvScrollDesc.Usage = D3D11_USAGE_DYNAMIC;
    uvScrollDesc.ByteWidth = sizeof(float) * 4; // float2 speed + float time + pad
    uvScrollDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    uvScrollDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Device->CreateBuffer(&uvScrollDesc, nullptr, &UVScrollCB);
    if (UVScrollCB)
    {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(DeviceContext->Map(UVScrollCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            float init[4] = {0, 0, 0, 0};
            memcpy(mapped.pData, init, sizeof(init));
            DeviceContext->Unmap(UVScrollCB, 0);
        }
        DeviceContext->PSSetConstantBuffers(5, 1, &UVScrollCB);
    }
}

void D3D11RHI::UpdateUVScrollConstantBuffers(const FVector2D& Speed, float TimeSec)
{
    if (!UVScrollCB) return;

    struct
    {
        float x;
        float y;
        float t;
        float pad;
    } data{Speed.X, Speed.Y, TimeSec, 0.0f};

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(DeviceContext->Map(UVScrollCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        memcpy(mapped.pData, &data, sizeof(data));
        DeviceContext->Unmap(UVScrollCB, 0);
        DeviceContext->PSSetConstantBuffers(5, 1, &UVScrollCB);
    }
}


void D3D11RHI::ReleaseSamplerState()
{
    if (DefaultSamplerState)
    {
        DefaultSamplerState->Release();
        DefaultSamplerState = nullptr;
    }
}

void D3D11RHI::ReleaseBlendState()
{
    if (BlendState)
    {
        BlendState->Release();
        BlendState = nullptr;
    }
}

void D3D11RHI::ReleaseRasterizerState()
{
    if (DefaultRasterizerState)
    {
        DefaultRasterizerState->Release();
        DefaultRasterizerState = nullptr;
    }
    if (WireFrameRasterizerState)
    {
        WireFrameRasterizerState->Release();
        WireFrameRasterizerState = nullptr;
    }
    DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void D3D11RHI::ReleaseFrameBuffer()
{
    if (FrameBuffer)
    {
        FrameBuffer->Release();
        FrameBuffer = nullptr;
    }
    if (RenderTargetView)
    {
        RenderTargetView->Release();
        RenderTargetView = nullptr;
    }

    if (DepthStencilView)
    {
        DepthStencilView->Release();
        DepthStencilView = nullptr;
    }
}

void D3D11RHI::ReleaseDeviceAndSwapChain()
{
    if (SwapChain)
    {
        SwapChain->Release();
        SwapChain = nullptr;
    }

    if (DeviceContext)
    {
        DeviceContext->Release();
        DeviceContext = nullptr;
    }

    if (Device)
    {
        Device->Release();
        Device = nullptr;
    }
}

void D3D11RHI::OmSetDepthStencilState(EComparisonFunc Func)
{
    switch (Func)
    {
    case EComparisonFunc::Always:
        DeviceContext->OMSetDepthStencilState(DepthStencilStateAlwaysNoWrite, 0);
        break;
    case EComparisonFunc::LessEqual:
        DeviceContext->OMSetDepthStencilState(DepthStencilStateLessEqualWrite, 0);
        break;
    case EComparisonFunc::GreaterEqual:
        DeviceContext->OMSetDepthStencilState(DepthStencilStateGreaterEqualWrite, 0);
        break;
    }
}

void D3D11RHI::CreateShader(ID3D11InputLayout** SimpleInputLayout,
                            ID3D11VertexShader** SimpleVertexShader,
                            ID3D11PixelShader** SimplePixelShader)
{
    ID3DBlob* vertexshaderCSO;
    ID3DBlob* pixelshaderCSO;

    D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0,
                       &vertexshaderCSO, nullptr);

    Device->CreateVertexShader(vertexshaderCSO->GetBufferPointer(),
                               vertexshaderCSO->GetBufferSize(), nullptr, SimpleVertexShader);

    D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0,
                       &pixelshaderCSO, nullptr);

    Device->CreatePixelShader(pixelshaderCSO->GetBufferPointer(), pixelshaderCSO->GetBufferSize(),
                              nullptr, SimplePixelShader);

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    Device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexshaderCSO->GetBufferPointer(),
                              vertexshaderCSO->GetBufferSize(), SimpleInputLayout);

    vertexshaderCSO->Release();
    pixelshaderCSO->Release();
}

void D3D11RHI::OnResize(UINT NewWidth, UINT NewHeight)
{
    if (!Device || !DeviceContext || !SwapChain)
        return;

    // 기존 리소스 해제
    ReleaseFrameBuffer();

    // 스왑체인 버퍼 리사이즈
    HRESULT hr = SwapChain->ResizeBuffers(
        0, // 버퍼 개수 (0 = 기존 유지)
        NewWidth,
        NewHeight,
        DXGI_FORMAT_UNKNOWN, // 기존 포맷 유지
        0
    );
    if (FAILED(hr))
    {
        UE_LOG("SwapChain->ResizeBuffers failed!\n");
        return;
    }

    // 새 프레임버퍼/RTV/DSV 생성
    CreateFrameBuffer();

    // 뷰포트 갱신
    ViewportInfo.TopLeftX = 0.0f;
    ViewportInfo.TopLeftY = 0.0f;
    ViewportInfo.Width = static_cast<float>(NewWidth);
    ViewportInfo.Height = static_cast<float>(NewHeight);
    ViewportInfo.MinDepth = 0.0f;
    ViewportInfo.MaxDepth = 1.0f;

    DeviceContext->RSSetViewports(1, &ViewportInfo);
}

void D3D11RHI::CreateBackBufferAndDepthStencil(UINT width, UINT height)
{
    // 기존 바인딩 해제 후 뷰 해제
    if (RenderTargetView)
    {
        DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
        RenderTargetView->Release();
        RenderTargetView = nullptr;
    }
    if (DepthStencilView)
    {
        DepthStencilView->Release();
        DepthStencilView = nullptr;
    }

    // 1) 백버퍼에서 RTV 생성
    ID3D11Texture2D* backBuffer = nullptr;
    HRESULT hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (FAILED(hr) || !backBuffer)
    {
        UE_LOG("GetBuffer(0) failed.\n");
        return;
    }

    D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
    framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    hr = Device->CreateRenderTargetView(backBuffer, &framebufferRTVdesc, &RenderTargetView);
    backBuffer->Release();
    if (FAILED(hr) || !RenderTargetView)
    {
        UE_LOG("CreateRenderTargetView failed.\n");
        return;
    }

    // 2) DepthStencil 텍스처/뷰 생성
    ID3D11Texture2D* depthTex = nullptr;
    D3D11_TEXTURE2D_DESC depthDesc{};
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1; // 멀티샘플링 끄는 경우
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    hr = Device->CreateTexture2D(&depthDesc, nullptr, &depthTex);
    if (FAILED(hr) || !depthTex)
    {
        UE_LOG("CreateTexture2D(depth) failed.\n");
        return;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = depthDesc.Format;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    hr = Device->CreateDepthStencilView(depthTex, &dsvDesc, &DepthStencilView);
    depthTex->Release();
    if (FAILED(hr) || !DepthStencilView)
    {
        UE_LOG("CreateDepthStencilView failed.\n");
        return;
    }

    // 3) OM 바인딩
    DeviceContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);

    // 4) 뷰포트 갱신
    SetViewport(width, height);
}

// ──────────────────────────────────────────────────────
// Helper: Viewport 갱신
// ──────────────────────────────────────────────────────
void D3D11RHI::SetViewport(UINT width, UINT height)
{
    ViewportInfo.TopLeftX = 0.0f;
    ViewportInfo.TopLeftY = 0.0f;
    ViewportInfo.Width = static_cast<float>(width);
    ViewportInfo.Height = static_cast<float>(height);
    ViewportInfo.MinDepth = 0.0f;
    ViewportInfo.MaxDepth = 1.0f;

    DeviceContext->RSSetViewports(1, &ViewportInfo);
}

// ──────────────────────────────────────────────────────
// 기존 오타 호출 호환용 래퍼 (선택)
// ──────────────────────────────────────────────────────
void D3D11RHI::setviewort(UINT width, UINT height)
{
    SetViewport(width, height);
}

void D3D11RHI::ResizeSwapChain(UINT width, UINT height)
{
    if (!SwapChain) return;

    // 렌더링 완료까지 대기 (중요!)
    if (DeviceContext)
    {
        DeviceContext->Flush();
    }

    // 현재 렌더 타겟 언바인딩
    if (DeviceContext)
    {
        DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    }

    // 기존 뷰 해제
    if (RenderTargetView)
    {
        RenderTargetView->Release();
        RenderTargetView = nullptr;
    }
    if (DepthStencilView)
    {
        DepthStencilView->Release();
        DepthStencilView = nullptr;
    }
    if (FrameBuffer)
    {
        FrameBuffer->Release();
        FrameBuffer = nullptr;
    }

    // 스왑체인 버퍼 리사이즈
    HRESULT hr = SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr))
    {
        UE_LOG("ResizeBuffers failed!\n");
        return;
    }

    // 다시 RTV/DSV 만들기
    CreateBackBufferAndDepthStencil(width, height);

    // 뷰포트도 갱신
    setviewort(width, height);
}

void D3D11RHI::PSSetDefaultSampler(UINT StartSlot)
{
    DeviceContext->PSSetSamplers(StartSlot, 1, &DefaultSamplerState);
}

//
// URenderer::URenderer(URHIDevice* InDevice) : RHIDevice(InDevice)
// {
//     InitializeLineBatch();
// }
//
// URenderer::~URenderer()
// {
//     if (LineBatchData)
//     {
//         delete LineBatchData;
//     }
// }
//
// void URenderer::BeginFrame()
// {
//     // 렌더링 통계 수집 시작
//     URenderingStatsCollector::GetInstance().BeginFrame();
//
//     // 상태 추적 리셋
//     ResetRenderStateTracking();
//
//     // 백버퍼/깊이버퍼를 클리어
//     RHIDevice->ClearBackBuffer(); // 배경색
//     RHIDevice->ClearDepthBuffer(1.0f, 0); // 깊이값 초기화
//     RHIDevice->CreateBlendState();
//     RHIDevice->IASetPrimitiveTopology();
//     // RS
//     RHIDevice->RSSetViewport();
//
//     //OM
//     //RHIDevice->OMSetBlendState();
//     RHIDevice->OMSetRenderTargets();
// }
//
// void URenderer::PrepareShader(FShader& InShader)
// {
//     RHIDevice->GetDeviceContext()->VSSetShader(InShader.SimpleVertexShader, nullptr, 0);
//     RHIDevice->GetDeviceContext()->PSSetShader(InShader.SimplePixelShader, nullptr, 0);
//     RHIDevice->GetDeviceContext()->IASetInputLayout(InShader.SimpleInputLayout);
// }
//
// void URenderer::PrepareShader(UShader* InShader)
// {
//     // 셰이더 변경 추적
//     if (LastShader != InShader)
//     {
//         URenderingStatsCollector::GetInstance().IncrementShaderChanges();
//         LastShader = InShader;
//     }
//
//     RHIDevice->GetDeviceContext()->VSSetShader(InShader->GetVertexShader(), nullptr, 0);
//     RHIDevice->GetDeviceContext()->PSSetShader(InShader->GetPixelShader(), nullptr, 0);
//     RHIDevice->GetDeviceContext()->IASetInputLayout(InShader->GetInputLayout());
// }
//
// void URenderer::OMSetBlendState(bool bIsChecked)
// {
//     if (bIsChecked == true)
//     {
//         RHIDevice->OMSetBlendState(true);
//     }
//     else
//     {
//         RHIDevice->OMSetBlendState(false);
//     }
// }
//
// void URenderer::RSSetState(EViewModeIndex ViewModeIndex)
// {
//     RHIDevice->RSSetState(ViewModeIndex);
// }
//
// void URenderer::UpdateConstantBuffer(const FMatrix& ModelMatrix, const FMatrix& ViewMatrix,
//                                      const FMatrix& ProjMatrix)
// {
//     RHIDevice->UpdateConstantBuffers(ModelMatrix, ViewMatrix, ProjMatrix);
// }
//
// void URenderer::UpdateHighLightConstantBuffer(const uint32 InPicked, const FVector& InColor,
//                                               const uint32 X, const uint32 Y, const uint32 Z,
//                                               const uint32 Gizmo)
// {
//     RHIDevice->UpdateHighLightConstantBuffers(InPicked, InColor, X, Y, Z, Gizmo);
// }
//
// void URenderer::UpdateBillboardConstantBuffers(const FVector& pos, const FMatrix& ViewMatrix,
//                                                const FMatrix& ProjMatrix,
//                                                const FVector& CameraRight, const FVector& CameraUp)
// {
//     RHIDevice->UpdateBillboardConstantBuffers(pos, ViewMatrix, ProjMatrix, CameraRight, CameraUp);
// }
//
// void URenderer::UpdatePixelConstantBuffers(const FObjMaterialInfo& InMaterialInfo,
//                                            bool bHasMaterial, bool bHasTexture)
// {
//     RHIDevice->UpdatePixelConstantBuffers(InMaterialInfo, bHasMaterial, bHasTexture);
// }
//
// void URenderer::UpdateColorBuffer(const FVector4& Color)
// {
//     RHIDevice->UpdateColorConstantBuffers(Color);
// }
//
// void URenderer::UpdateUVScroll(const FVector2D& Speed, float TimeSec)
// {
//     RHIDevice->UpdateUVScrollConstantBuffers(Speed, TimeSec);
// }
//
// void URenderer::DrawIndexedPrimitiveComponent(UStaticMesh* InMesh,
//                                               D3D11_PRIMITIVE_TOPOLOGY InTopology,
//                                               const TArray<FMaterialSlot>& InComponentMaterialSlots)
// {
//     URenderingStatsCollector& StatsCollector = URenderingStatsCollector::GetInstance();
//
//     // 디버그: StaticMesh 렌더링 통계
//
//     UINT stride = 0;
//     switch (InMesh->GetVertexType())
//     {
//     case EVertexLayoutType::PositionColor:
//         stride = sizeof(FVertexSimple);
//         break;
//     case EVertexLayoutType::PositionColorTexturNormal:
//         stride = sizeof(FVertexDynamic);
//         break;
//     case EVertexLayoutType::PositionBillBoard:
//         stride = sizeof(FBillboardVertexInfo_GPU);
//         break;
//     default:
//         // Handle unknown or unsupported vertex types
//         assert(false && "Unknown vertex type!");
//         return; // or log an error
//     }
//     UINT offset = 0;
//
//     ID3D11Buffer* VertexBuffer = InMesh->GetVertexBuffer();
//     ID3D11Buffer* IndexBuffer = InMesh->GetIndexBuffer();
//     uint32 VertexCount = InMesh->GetVertexCount();
//     uint32 IndexCount = InMesh->GetIndexCount();
//
//     RHIDevice->GetDeviceContext()->IASetVertexBuffers(
//         0, 1, &VertexBuffer, &stride, &offset
//     );
//
//     RHIDevice->GetDeviceContext()->IASetIndexBuffer(
//         IndexBuffer, DXGI_FORMAT_R32_UINT, 0
//     );
//
//     RHIDevice->GetDeviceContext()->IASetPrimitiveTopology(InTopology);
//     RHIDevice->PSSetDefaultSampler(0);
//
//     if (InMesh->HasMaterial())
//     {
//         const TArray<FGroupInfo>& MeshGroupInfos = InMesh->GetMeshGroupInfo();
//         const uint32 NumMeshGroupInfos = static_cast<uint32>(MeshGroupInfos.size());
//         for (uint32 i = 0; i < NumMeshGroupInfos; ++i)
//         {
//             const UMaterial* const Material = UResourceManager::GetInstance().Get<UMaterial>(
//                 InComponentMaterialSlots[i].MaterialName);
//             const FObjMaterialInfo& MaterialInfo = Material->GetMaterialInfo();
//             bool bHasTexture = !(MaterialInfo.DiffuseTextureFileName.empty());
//
//             // 재료 변경 추적
//             if (LastMaterial != Material)
//             {
//                 StatsCollector.IncrementMaterialChanges();
//                 LastMaterial = const_cast<UMaterial*>(Material);
//             }
//
//             FTextureData* TextureData = nullptr;
//             if (bHasTexture)
//             {
//                 FWideString WTextureFileName(MaterialInfo.DiffuseTextureFileName.begin(),
//                                              MaterialInfo.DiffuseTextureFileName.end());
//                 // 단순 ascii라고 가정
//                 TextureData = UResourceManager::GetInstance().CreateOrGetTextureData(
//                     WTextureFileName);
//
//                 // 텍스처 변경 추적 (임시로 FTextureData*를 UTexture*로 캠스트)
//                 UTexture* CurrentTexture = reinterpret_cast<UTexture*>(TextureData);
//                 if (LastTexture != CurrentTexture)
//                 {
//                     StatsCollector.IncrementTextureChanges();
//                     LastTexture = CurrentTexture;
//                 }
//
//                 RHIDevice->GetDeviceContext()->PSSetShaderResources(
//                     0, 1, &(TextureData->TextureSRV));
//             }
//
//             RHIDevice->UpdatePixelConstantBuffers(MaterialInfo, true, bHasTexture); // PSSet도 해줌
//
//             // DrawCall 수실행 및 통계 추가
//             RHIDevice->GetDeviceContext()->DrawIndexed(MeshGroupInfos[i].IndexCount,
//                                                        MeshGroupInfos[i].StartIndex, 0);
//             StatsCollector.IncrementDrawCalls();
//         }
//     }
//     else
//     {
//         FObjMaterialInfo ObjMaterialInfo;
//         RHIDevice->UpdatePixelConstantBuffers(ObjMaterialInfo, false, false); // PSSet도 해줌
//         RHIDevice->GetDeviceContext()->DrawIndexed(IndexCount, 0, 0);
//         StatsCollector.IncrementDrawCalls();
//     }
// }
//
// void URenderer::DrawIndexedPrimitiveComponent(UTextRenderComponent* Comp,
//                                               D3D11_PRIMITIVE_TOPOLOGY InTopology)
// {
//     URenderingStatsCollector& StatsCollector = URenderingStatsCollector::GetInstance();
//
//     // 디버그: TextRenderComponent 렌더링 통계
//
//     UINT Stride = sizeof(FBillboardVertexInfo_GPU);
//     ID3D11Buffer* VertexBuff = Comp->GetStaticMesh()->GetVertexBuffer();
//     ID3D11Buffer* IndexBuff = Comp->GetStaticMesh()->GetIndexBuffer();
//
//     // 매테리얼 변경 추적
//     UMaterial* CompMaterial = Comp->GetMaterial();
//     if (LastMaterial != CompMaterial)
//     {
//         StatsCollector.IncrementMaterialChanges();
//         LastMaterial = CompMaterial;
//     }
//
//     UShader* CompShader = CompMaterial->GetShader();
//     // 셰이더 변경 추적
//     if (LastShader != CompShader)
//     {
//         StatsCollector.IncrementShaderChanges();
//         LastShader = CompShader;
//     }
//
//     RHIDevice->GetDeviceContext()->IASetInputLayout(CompShader->GetInputLayout());
//
//
//     UINT offset = 0;
//     RHIDevice->GetDeviceContext()->IASetVertexBuffers(
//         0, 1, &VertexBuff, &Stride, &offset
//     );
//     RHIDevice->GetDeviceContext()->IASetIndexBuffer(
//         IndexBuff, DXGI_FORMAT_R32_UINT, 0
//     );
//
//     // 텍스처 변경 추적 (텍스처 비교)
//     UTexture* CompTexture = CompMaterial->GetTexture();
//     if (LastTexture != CompTexture)
//     {
//         StatsCollector.IncrementTextureChanges();
//         LastTexture = CompTexture;
//     }
//
//     ID3D11ShaderResourceView* TextureSRV = CompTexture->GetShaderResourceView();
//     RHIDevice->PSSetDefaultSampler(0);
//     RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &TextureSRV);
//     RHIDevice->GetDeviceContext()->IASetPrimitiveTopology(InTopology);
//     RHIDevice->GetDeviceContext()->DrawIndexed(Comp->GetStaticMesh()->GetIndexCount(), 0, 0);
//     StatsCollector.IncrementDrawCalls();
// }
//
// void URenderer::SetViewModeType(EViewModeIndex ViewModeIndex)
// {
//     RHIDevice->RSSetState(ViewModeIndex);
//     if (ViewModeIndex == EViewModeIndex::VMI_Wireframe)
//         RHIDevice->UpdateColorConstantBuffers(FVector4{1.f, 0.f, 0.f, 1.f});
//     else
//         RHIDevice->UpdateColorConstantBuffers(FVector4{1.f, 1.f, 1.f, 0.f});
// }
//
// void URenderer::EndFrame()
// {
//     // 렌더링 통계 수집 종료
//     URenderingStatsCollector& StatsCollector = URenderingStatsCollector::GetInstance();
//     StatsCollector.EndFrame();
//
//     // 현재 프레임 통계를 업데이트
//     const FRenderingStats& CurrentStats = StatsCollector.GetCurrentFrameStats();
//     StatsCollector.UpdateFrameStats(CurrentStats);
//
//     // 평균 통계를 얻어서 오버레이에 업데이트
//     const FRenderingStats& AvgStats = StatsCollector.GetAverageStats();
//     UStatsOverlayD2D::Get().UpdateRenderingStats(
//         AvgStats.TotalDrawCalls,
//         AvgStats.MaterialChanges,
//         AvgStats.TextureChanges,
//         AvgStats.ShaderChanges
//     );
//
//     RHIDevice->Present();
// }
//
// void URenderer::OMSetDepthStencilState(EComparisonFunc Func)
// {
//     RHIDevice->OmSetDepthStencilState(Func);
// }
//
// void URenderer::InitializeLineBatch()
// {
//     // Create UDynamicMesh for efficient line batching
//     DynamicLineMesh = UResourceManager::GetInstance().Load<ULineDynamicMesh>("Line");
//
//     // Initialize with maximum capacity (MAX_LINES * 2 vertices, MAX_LINES * 2 indices)
//     uint32 maxVertices = MAX_LINES * 2;
//     uint32 maxIndices = MAX_LINES * 2;
//     DynamicLineMesh->Load(maxVertices, maxIndices, RHIDevice->GetDevice());
//
//     // Create FMeshData for accumulating line data
//     LineBatchData = new FMeshData();
//
//     // Load line shader
//     LineShader = UResourceManager::GetInstance().Load<UShader>(
//         "ShaderLine.hlsl", EVertexLayoutType::PositionColor);
// }
//
// void URenderer::BeginLineBatch()
// {
//     if (!LineBatchData) return;
//
//     bLineBatchActive = true;
//
//     // Clear previous batch data
//     LineBatchData->Vertices.clear();
//     LineBatchData->Color.clear();
//     LineBatchData->Indices.clear();
// }
//
// void URenderer::AddLine(const FVector& Start, const FVector& End, const FVector4& Color)
// {
//     if (!bLineBatchActive || !LineBatchData) return;
//
//     uint32 startIndex = static_cast<uint32>(LineBatchData->Vertices.size());
//
//     // Add vertices
//     LineBatchData->Vertices.push_back(Start);
//     LineBatchData->Vertices.push_back(End);
//
//     // Add colors
//     LineBatchData->Color.push_back(Color);
//     LineBatchData->Color.push_back(Color);
//
//     // Add indices for line (2 vertices per line)
//     LineBatchData->Indices.push_back(startIndex);
//     LineBatchData->Indices.push_back(startIndex + 1);
// }
//
// void URenderer::AddLines(const TArray<FVector>& StartPoints, const TArray<FVector>& EndPoints,
//                          const TArray<FVector4>& Colors)
// {
//     if (!bLineBatchActive || !LineBatchData) return;
//
//     // Validate input arrays have same size
//     if (StartPoints.size() != EndPoints.size() || StartPoints.size() != Colors.size())
//         return;
//
//     uint32 startIndex = static_cast<uint32>(LineBatchData->Vertices.size());
//
//     // Reserve space for efficiency
//     size_t lineCount = StartPoints.size();
//     LineBatchData->Vertices.reserve(LineBatchData->Vertices.size() + lineCount * 2);
//     LineBatchData->Color.reserve(LineBatchData->Color.size() + lineCount * 2);
//     LineBatchData->Indices.reserve(LineBatchData->Indices.size() + lineCount * 2);
//
//     // Add all lines at once
//     for (size_t i = 0; i < lineCount; ++i)
//     {
//         uint32 currentIndex = startIndex + static_cast<uint32>(i * 2);
//
//         // Add vertices
//         LineBatchData->Vertices.push_back(StartPoints[i]);
//         LineBatchData->Vertices.push_back(EndPoints[i]);
//
//         // Add colors
//         LineBatchData->Color.push_back(Colors[i]);
//         LineBatchData->Color.push_back(Colors[i]);
//
//         // Add indices for line (2 vertices per line)
//         LineBatchData->Indices.push_back(currentIndex);
//         LineBatchData->Indices.push_back(currentIndex + 1);
//     }
// }
//
// void URenderer::EndLineBatch(const FMatrix& ModelMatrix, const FMatrix& ViewMatrix,
//                              const FMatrix& ProjectionMatrix)
// {
//     if (!bLineBatchActive || !LineBatchData || !DynamicLineMesh || LineBatchData->Vertices.empty())
//     {
//         bLineBatchActive = false;
//         return;
//     }
//
//     // Efficiently update dynamic mesh data (no buffer recreation!)
//     if (!DynamicLineMesh->UpdateData(LineBatchData, RHIDevice->GetDeviceContext()))
//     {
//         bLineBatchActive = false;
//         return;
//     }
//
//     // Set up rendering state
//     UpdateConstantBuffer(ModelMatrix, ViewMatrix, ProjectionMatrix);
//     PrepareShader(LineShader);
//
//     // Render using dynamic mesh
//     if (DynamicLineMesh->GetCurrentVertexCount() > 0 && DynamicLineMesh->GetCurrentIndexCount() > 0)
//     {
//         UINT stride = sizeof(FVertexSimple);
//         UINT offset = 0;
//
//         ID3D11Buffer* vertexBuffer = DynamicLineMesh->GetVertexBuffer();
//         ID3D11Buffer* indexBuffer = DynamicLineMesh->GetIndexBuffer();
//
//         RHIDevice->GetDeviceContext()->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
//         RHIDevice->GetDeviceContext()->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
//         RHIDevice->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
//         RHIDevice->GetDeviceContext()->DrawIndexed(DynamicLineMesh->GetCurrentIndexCount(), 0, 0);
//
//         // 라인 렌더링에 대한 DrawCall 통계 추가
//         URenderingStatsCollector::GetInstance().IncrementDrawCalls();
//     }
//
//     bLineBatchActive = false;
// }
//
// void URenderer::ResetRenderStateTracking()
// {
//     LastMaterial = nullptr;
//     LastShader = nullptr;
//     LastTexture = nullptr;
// }
//
// void URenderer::ClearLineBatch()
// {
//     if (!LineBatchData) return;
//
//     LineBatchData->Vertices.clear();
//     LineBatchData->Color.clear();
//     LineBatchData->Indices.clear();
//
//     bLineBatchActive = false;
// }
