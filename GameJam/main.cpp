#include "pch.h"
#include <windows.h>

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

#include "Sphere.h"
#include "Core/Public/Primitive.h"
#include "Manager/Public/ImGuiManager.h"
#include "Render/Public/Renderer.h"

class UBall;

static void HandleMouseClick(int InX, int InY, bool InIsLeftClick);
static void RemoveSpecificBall(int IndexToRemove);
static void SetGravityCenter(int IndexToSet);
static void HandleCollisions();

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static void AddNewBall();
static void RemoveRandomBall();

// Static
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static bool bPinballGravity = true;

static HWND GlobalWindowHandle = nullptr;
static UBall* GravityCenterBall = nullptr;

static int TotalPrimitives = 0;
static UPrimitive** PrimitiveList = nullptr;

/////////////////////////////////////////////////////////////////////////////////////////////////////


// [Fixed]
class UBall :
    public UPrimitive
{
public:
    FVector3 Location; // [Fixed]
    FVector3 Velocity; // [Fixed]
    float Radius; // [Fixed]
    float Mass; // [Fixed]
    static int TotalNumBalls; // [Fixed]

    UBall()
    {
        // Position Setting
        Location.x = -0.8f + (rand() / (float)RAND_MAX) * 1.6f;
        Location.y = -0.8f + (rand() / (float)RAND_MAX) * 1.6f;
        Location.z = 0.0f;

        // Velocity Setting
        Velocity.x = (-0.2f + (rand() / (float)RAND_MAX) * 0.4f);
        Velocity.y = (-0.2f + (rand() / (float)RAND_MAX) * 0.4f);
        Velocity.z = 0.0f;

        // Radius Setting
        Radius = 0.1f + (rand() / (float)RAND_MAX) * 0.05f;

        // 질량은 면적에 비례하도록 설정
        Mass = Radius * Radius;
    }

    // TODO(KHJ): DT
    void Move()
    {
        const float FixedDeltaTime = 1.0f / 30.0f;

        // Make Gravity Center Stop
        if (GravityCenterBall == this)
        {
            Velocity = FVector3(0.f, 0.f, 0.f);
            return;
        }

        // 중력 중심 존재 여부에 따라, 중력 적용
        if (GravityCenterBall != nullptr)
        {
            FVector3 Direction = GravityCenterBall->Location - this->Location;
            float DistanceSq = Direction.LengthSquare();

            // 힘의 최대값 조정
            if (DistanceSq > (GravityCenterBall->Radius + this->Radius) * (GravityCenterBall->Radius
                + this->Radius))
            {
                Direction.Normalize();
                float GravityStrength = 2.0f;
                Velocity += Direction * GravityStrength * FixedDeltaTime / (DistanceSq + 0.1f);
            }
        }
        else if (bPinballGravity) // 기존 핀볼 중력
        {
            Velocity.y -= (1.0f * FixedDeltaTime);
        }

        Location += Velocity * FixedDeltaTime;

        // 벽 충돌 처리
        if ((Location.x > 1.0f - Radius && Velocity.x > 0) || (Location.x < -1.0f + Radius &&
            Velocity.x < 0))
        {
            Velocity.x *= -1.0f;
        }
        if ((Location.y > 1.0f - Radius && Velocity.y > 0) || (Location.y < -1.0f + Radius &&
            Velocity.y < 0))
        {
            Velocity.y *= -1.0f;
        }
    }
};

int UBall::TotalNumBalls = 0;

/**
 * @brief 매 프레임 반복되는 Logic을 처리하는 함수
 */
static void MainLoop(URenderer& InRenderer)
{
    // Renderer와 Shader 생성 이후에 버텍스 버퍼를 생성합니다.
    UINT numVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);

    InRenderer.vertexBufferSphere = InRenderer.CreateVertexBuffer(
        sphere_vertices, sizeof(sphere_vertices));

    InRenderer.numVerticesSphere = numVerticesSphere;

    const int TargetFPS = 30;
    const double TargetFrameTime = 1000.0 / TargetFPS;

    LARGE_INTEGER Frequency;
    QueryPerformanceFrequency(&Frequency);

    LARGE_INTEGER StartTime, EndTime;
    double ElapsedTime = 0.0;
    bool bIsExit = false;

    while (bIsExit == false)
    {
        QueryPerformanceCounter(&StartTime);

        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
            {
                bIsExit = true;
                break;
            }
        }
        if (bIsExit)
        {
            break;
        }

        while (TotalPrimitives < UBall::TotalNumBalls)
        {
            AddNewBall();
        }
        while (TotalPrimitives > UBall::TotalNumBalls)
        {
            RemoveRandomBall();
        }

        // 물리 업데이트
        for (int i = 0; i < TotalPrimitives; ++i)
        {
            UBall* Ball = static_cast<UBall*>(PrimitiveList[i]);
            Ball->Move();
        }

        // 물리 업데이트 후 충돌 처리
        HandleCollisions();

        // 렌더링
        InRenderer.Prepare();
        InRenderer.PrepareShader();

        for (int i = 0; i < TotalPrimitives; ++i)
        {
            UBall* ball = static_cast<UBall*>(PrimitiveList[i]);
            InRenderer.UpdateConstant(ball->Location, ball->Radius);
            InRenderer.RenderPrimitive();
        }

        FImGuiManager::RenderImGui();

        InRenderer.SwapBuffer();

        // 일정한 프레임 타임을 유지
        do
        {
            Sleep(0);
            QueryPerformanceCounter(&EndTime);
            ElapsedTime = (EndTime.QuadPart - StartTime.QuadPart) * 1000.0 / Frequency.QuadPart;
        }
        while (ElapsedTime < TargetFrameTime);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    // 윈도우 클래스 이름
    WCHAR WindowClass[] = L"JungleWindowClass";

    // 윈도우 타이틀바에 표시될 이름
    WCHAR Title[] = L"Game Tech Lab";

    // 각종 메시지를 처리할 함수인 WndProc의 함수 포인터를 WindowClass 구조체에 넣는다.
    WNDCLASSW wndclass = {0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass};

    // 윈도우 클래스 등록
    RegisterClassW(&wndclass);

    // 1024 x 1024 크기에 윈도우 생성
    HWND WindowHandle = CreateWindowExW(0, WindowClass, Title,
                                        WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
                                        CW_USEDEFAULT, CW_USEDEFAULT, 1024, 1024,
                                        nullptr, nullptr, hInstance, nullptr);

    // Make Window Handle Global
    GlobalWindowHandle = WindowHandle;

    URenderer Renderer;

    Renderer.TotalInit(WindowHandle);

    MainLoop(Renderer);

    Renderer.TotalShutDown();

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Renderer Initializer
 * @param InWindowHandle Window Handle
 */
void URenderer::Create(HWND InWindowHandle)
{
    // Direct3D 장치 및 스왑 체인 생성
    CreateDeviceAndSwapChain(InWindowHandle);

    // 프레임 버퍼 생성
    CreateFrameBuffer();

    // 래스터라이저 상태 생성
    CreateRasterizerState();
}

/**
 * @brief Direct3D 장치 및 스왑 체인을 생성하는 함수
 * @param InWindowHandle
 */
void URenderer::CreateDeviceAndSwapChain(HWND InWindowHandle)
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
    swapchaindesc.OutputWindow = InWindowHandle; // 렌더링할 창 핸들
    swapchaindesc.Windowed = TRUE; // 창 모드
    swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 스왑 방식

    // Direct3D 장치와 스왑 체인을 생성
    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                  D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
                                  featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
                                  &swapchaindesc, &SwapChain, &Device, nullptr, &DeviceContext);

    // 생성된 스왑 체인의 정보 가져오기
    SwapChain->GetDesc(&swapchaindesc);

    // 뷰포트 정보 설정
    ViewportInfo = {
        0.0f, 0.0f, (float)swapchaindesc.BufferDesc.Width,
        (float)swapchaindesc.BufferDesc.Height, 0.0f, 1.0f
    };
}

/**
 * @brief Direct3D 장치 및 스왑 체인을 해제하는 함수
 */
void URenderer::ReleaseDeviceAndSwapChain()
{
    if (DeviceContext)
    {
        DeviceContext->Flush(); // 남아있는 GPU 명령 실행
    }

    if (SwapChain)
    {
        SwapChain->Release();
        SwapChain = nullptr;
    }

    if (Device)
    {
        Device->Release();
        Device = nullptr;
    }

    if (DeviceContext)
    {
        DeviceContext->Release();
        DeviceContext = nullptr;
    }
}

/**
 * @brief FrameBuffer 생성 함수
 */
void URenderer::CreateFrameBuffer()
{
    // 스왑 체인으로부터 백 버퍼 텍스처 가져오기
    SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

    // 렌더 타겟 뷰 생성
    D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
    framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // 색상 포맷
    framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처

    Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);
}

/**
 * @brief 프레임 버퍼를 해제하는 함수
 */
void URenderer::ReleaseFrameBuffer()
{
    if (FrameBuffer)
    {
        FrameBuffer->Release();
        FrameBuffer = nullptr;
    }

    if (FrameBufferRTV)
    {
        FrameBufferRTV->Release();
        FrameBufferRTV = nullptr;
    }
}

/**
 * @brief 래스터라이저 상태를 생성하는 함수
 */
void URenderer::CreateRasterizerState()
{
    D3D11_RASTERIZER_DESC rasterizerdesc = {};
    rasterizerdesc.FillMode = D3D11_FILL_SOLID; // 채우기 모드
    rasterizerdesc.CullMode = D3D11_CULL_BACK; // 백 페이스 컬링

    Device->CreateRasterizerState(&rasterizerdesc, &RasterizerState);
}

/**
 * @brief 래스터라이저 상태를 해제하는 함수
 */
void URenderer::ReleaseRasterizerState()
{
    if (RasterizerState)
    {
        RasterizerState->Release();
        RasterizerState = nullptr;
    }
}

/**
 * @brief 렌더러에 사용된 모든 리소스를 해제하는 함수
 */
void URenderer::Release()
{
    RasterizerState->Release();

    // 렌더 타겟을 초기화
    DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    ReleaseFrameBuffer();
    ReleaseDeviceAndSwapChain();
}

/**
 * @brief 스왑 체인의 백 버퍼와 프론트 버퍼를 교체하여 화면에 출력
 */
void URenderer::SwapBuffer() const
{
    SwapChain->Present(1, 0); // 1: VSync 활성화
}

/**
 * @brief Shader 기반의 CSO 생성 함수
 */
void URenderer::CreateShader()
{
    ID3DBlob* VertexShaderCSO;
    ID3DBlob* PixelShaderCSO;

    D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0,
                       &VertexShaderCSO, nullptr);

    Device->CreateVertexShader(VertexShaderCSO->GetBufferPointer(),
                               VertexShaderCSO->GetBufferSize(), nullptr, &SimpleVertexShader);

    D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0,
                       &PixelShaderCSO, nullptr);

    Device->CreatePixelShader(PixelShaderCSO->GetBufferPointer(),
                              PixelShaderCSO->GetBufferSize(), nullptr, &SimplePixelShader);

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    Device->CreateInputLayout(layout, ARRAYSIZE(layout), VertexShaderCSO->GetBufferPointer(),
                              VertexShaderCSO->GetBufferSize(), &SimpleInputLayout);

    Stride = sizeof(FVertexSimple);

    VertexShaderCSO->Release();
    PixelShaderCSO->Release();
}

/**
 * @brief Shader Release
 */
void URenderer::ReleaseShader()
{
    if (SimpleInputLayout)
    {
        SimpleInputLayout->Release();
        SimpleInputLayout = nullptr;
    }

    if (SimplePixelShader)
    {
        SimplePixelShader->Release();
        SimplePixelShader = nullptr;
    }

    if (SimpleVertexShader)
    {
        SimpleVertexShader->Release();
        SimpleVertexShader = nullptr;
    }
}

/**
 * @brief Render Prepare Step
 */
void URenderer::Prepare() const
{
    DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);

    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    DeviceContext->RSSetViewports(1, &ViewportInfo);
    DeviceContext->RSSetState(RasterizerState);

    DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, nullptr);
    DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
}

/**
 * @brief Prepare Shader 함수
 */
void URenderer::PrepareShader() const
{
    DeviceContext->VSSetShader(SimpleVertexShader, nullptr, 0);
    DeviceContext->PSSetShader(SimplePixelShader, nullptr, 0);
    DeviceContext->IASetInputLayout(SimpleInputLayout);

    if (ConstantBuffer)
    {
        DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
    }
}

/**
 * @brief Buffer에 작성된 내용 그리는 함수
 */
void URenderer::RenderPrimitive() const
{
    UINT Offset = 0;
    DeviceContext->IASetVertexBuffers(0, 1, &vertexBufferSphere, &Stride, &Offset);
    DeviceContext->Draw(numVerticesSphere, 0);
}

/**
 * @brief 정점 Buffer 생성 함수
 * @param InVertices
 * @param InByteWidth
 * @return
 */
ID3D11Buffer* URenderer::CreateVertexBuffer(FVertexSimple* InVertices, UINT InByteWidth) const
{
    // 2. Create a vertex buffer
    D3D11_BUFFER_DESC VertexBufferDesc = {};
    VertexBufferDesc.ByteWidth = InByteWidth;
    VertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // will never be updated
    VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA VertexBufferSRD = {InVertices};

    ID3D11Buffer* vertexBuffer;

    Device->CreateBuffer(&VertexBufferDesc, &VertexBufferSRD, &vertexBuffer);

    return vertexBuffer;
}

/**
 * @brief Vertex Buffer 소멸 함수
 * @param InVertexBuffer
 */
void URenderer::ReleaseVertexBuffer(ID3D11Buffer* InVertexBuffer)
{
    InVertexBuffer->Release();
}

/**
 * @brief 상수 버퍼 생성 함수
 */
void URenderer::CreateConstantBuffer()
{
    D3D11_BUFFER_DESC constantbufferdesc = {};
    constantbufferdesc.ByteWidth = sizeof(FConstants) + 0xf & 0xfffffff0;
    // ensure constant buffer size is multiple of 16 bytes
    constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC; // will be updated from CPU every frame
    constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    Device->CreateBuffer(&constantbufferdesc, nullptr, &ConstantBuffer);
}

/**
 * @brief 상수 버퍼 소멸 함수
 */
void URenderer::ReleaseConstantBuffer()
{
    if (ConstantBuffer)
    {
        ConstantBuffer->Release();
        ConstantBuffer = nullptr;
    }
}

/**
 * @brief 상수 버퍼 업데이트 함수
 * @param InOffset
 * @param InScale Ball Size
 */
void URenderer::UpdateConstant(FVector3 InOffset, float InScale) const
{
    if (ConstantBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

        DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
        // update constant buffer every frame
        FConstants* constants = (FConstants*)constantbufferMSR.pData;
        {
            constants->Offset = InOffset;
            constants->Scale = InScale;
        }
        DeviceContext->Unmap(ConstantBuffer, 0);
    }
}

void URenderer::TotalInit(HWND InWindowHandle)
{
    Create(InWindowHandle);
    CreateShader();

    CreateConstantBuffer();

    FImGuiManager::InitializeImGui(InWindowHandle, *this);
}

void URenderer::TotalShutDown()
{
    FImGuiManager::ReleaseImGui();

    // Release Balls
    for (int i = 0; i < TotalPrimitives; ++i)
    {
        delete PrimitiveList[i];
    }

    // Release & Remove Dangling Pointer
    if (PrimitiveList)
    {
        delete[] PrimitiveList;
        PrimitiveList = nullptr;
    }

    ReleaseVertexBuffer(this->vertexBufferSphere);
    ReleaseConstantBuffer();
    ReleaseShader();
    Release();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// 각종 메시지를 처리할 함수
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    {
        // ImGui가 마우스 이벤트를 사용했다면, 게임 로직에서는 처리하지 않아야 한다.
        if (ImGui::GetIO().WantCaptureMouse)
        {
            return true;
        }
    }

    switch (message)
    {
    // 마우스 왼쪽 버튼 클릭
    case WM_LBUTTONDOWN:
        {
            HandleMouseClick(LOWORD(lParam), HIWORD(lParam), true);
            return 0;
        }
    // 마우스 오른쪽 버튼 클릭
    case WM_RBUTTONDOWN:
        {
            HandleMouseClick(LOWORD(lParam), HIWORD(lParam), false);
            return 0;
        }
    case WM_DESTROY:
        // Signal that the app should quit
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

/**
 * @brief 새로운 공을 추가하는 함수
 */
void AddNewBall()
{
    // Make New List
    UPrimitive** NewList = new UPrimitive*[TotalPrimitives + 1];

    // Copy
    for (int i = 0; i < TotalPrimitives; ++i)
    {
        NewList[i] = PrimitiveList[i];
    }

    // Add New Ball
    NewList[TotalPrimitives] = new UBall();

    // Release
    delete[] PrimitiveList;

    // Swap List
    PrimitiveList = NewList;

    ++TotalPrimitives;
}

/**
 * @brief 임의의 공을 제거하는 함수
 */
void RemoveRandomBall()
{
    if (TotalPrimitives <= 0)
    {
        return;
    }

    // Select Index
    int IndexToRemove = rand() % TotalPrimitives;

    // If Gravity Center, Make Null First
    if (PrimitiveList[IndexToRemove] == GravityCenterBall)
    {
        GravityCenterBall = nullptr;
    }

    // Remove Object
    delete PrimitiveList[IndexToRemove];

    // Make New List
    UPrimitive** NewList = nullptr;
    if (TotalPrimitives - 1 > 0)
    {
        NewList = new UPrimitive*[TotalPrimitives - 1];
    }

    // Copy
    int NewIndex = 0;
    for (int i = 0; i < TotalPrimitives; ++i)
    {
        if (i == IndexToRemove)
        {
            continue;
        }
        NewList[NewIndex] = PrimitiveList[i];
        NewIndex++;
    }

    // Release
    delete[] PrimitiveList;

    // Swap List
    PrimitiveList = NewList;

    --TotalPrimitives;
}

/**
 * @brief 공들 간의 충돌을 감지하고 처리하는 함수
 */
void HandleCollisions()
{
    for (int i = 0; i < TotalPrimitives; ++i)
    {
        for (int j = i + 1; j < TotalPrimitives; ++j)
        {
            UBall* Ball1 = static_cast<UBall*>(PrimitiveList[i]);
            UBall* Ball2 = static_cast<UBall*>(PrimitiveList[j]);

            FVector3 Delta = Ball1->Location - Ball2->Location;
            float DistanceSq = Delta.LengthSquare();
            float CombinedRadius = Ball1->Radius + Ball2->Radius;

            if (DistanceSq < CombinedRadius * CombinedRadius && DistanceSq > 0.0f)
            {
                // 충돌 발생
                float Distance = sqrtf(DistanceSq);
                FVector3 Normal = Delta / Distance;

                // 겹침 해결
                float Overlap = 0.5f * (CombinedRadius - Distance);
                Ball1->Location += Normal * Overlap;
                Ball2->Location -= Normal * Overlap;

                // 탄성 충돌 계산
                FVector3 relativeVelocity = Ball1->Velocity - Ball2->Velocity;
                float VelocityAlongNormal = Dot(relativeVelocity, Normal);

                if (VelocityAlongNormal < 0)
                {
                    float Restitution = 1.0f; // 완전 탄성 충돌
                    float ImpulseScalar = -(1.0f + Restitution) * VelocityAlongNormal;
                    ImpulseScalar /= (1.0f / Ball1->Mass) + (1.0f / Ball2->Mass);

                    FVector3 impulse = Normal * ImpulseScalar;
                    Ball1->Velocity += impulse * (1.0f / Ball1->Mass);
                    Ball2->Velocity -= impulse * (1.0f / Ball2->Mass);
                }
            }
        }
    }
}

/**
 * @brief 특정 index의 공을 제거하는 함수
 * @param IndexToRemove 제거할 공의 index
 */
void RemoveSpecificBall(int IndexToRemove)
{
    if (TotalPrimitives <= 0 || IndexToRemove < 0 || IndexToRemove >= TotalPrimitives)
    {
        return;
    }

    // Make GravityCenter Null If Selected
    if (PrimitiveList[IndexToRemove] == GravityCenterBall)
    {
        GravityCenterBall = nullptr;
    }

    // Release Object
    delete PrimitiveList[IndexToRemove];

    // Make New List
    UPrimitive** NewList = nullptr;
    if (TotalPrimitives - 1 > 0)
    {
        NewList = new UPrimitive*[TotalPrimitives - 1];
    }

    // Copy
    int NewIndex = 0;
    for (int i = 0; i < TotalPrimitives; ++i)
    {
        if (i == IndexToRemove)
        {
            continue;
        }
        NewList[NewIndex] = PrimitiveList[i];
        NewIndex++;
    }

    // Release Array
    delete[] PrimitiveList;

    // Swap List
    PrimitiveList = NewList;

    // Count Refresh
    --TotalPrimitives;
    --UBall::TotalNumBalls;

    assert(UBall::TotalNumBalls >= 0);
}

/**
 * @brief 특정 공을 중력 중심으로 설정하거나 해제하는 함수
 * @param IndexToSet 중력 중심으로 설정할 공의 인덱스
 */
void SetGravityCenter(int IndexToSet)
{
    if (IndexToSet < 0 || IndexToSet >= TotalPrimitives)
    {
        return;
    }

    UBall* SelectedBall = static_cast<UBall*>(PrimitiveList[IndexToSet]);

    // 이미 중력 중심으로 설정된 공을 다시 클릭하면 중력 효과를 해제
    if (GravityCenterBall == SelectedBall)
    {
        GravityCenterBall = nullptr;
    }
    else
    {
        // 새로운 공을 중력 중심으로 설정, 중력 중심이 된 공은 정지
        GravityCenterBall = SelectedBall;
        GravityCenterBall->Velocity = FVector3(0.f, 0.f, 0.f);
    }
}

/**
 * @brief 마우스 클릭 이벤트를 받아 공 선택 및 관련 로직을 처리하는 함수
 * @param InX 마우스 x 좌표 (스크린 좌표)
 * @param InY 마우스 y 좌표 (스크린 좌표)
 * @param InIsLeftClick 왼쪽 클릭 여부
 */
void HandleMouseClick(int InX, int InY, bool InIsLeftClick)
{
    if (!GlobalWindowHandle)
    {
        return;
    }

    // Get Window Size
    RECT ClientRect;
    GetClientRect(GlobalWindowHandle, &ClientRect);
    float clientWidth = static_cast<float>(ClientRect.right - ClientRect.left);
    float clientHeight = static_cast<float>(ClientRect.bottom - ClientRect.top);

    // NDC Convert
    float ndc_x = (static_cast<float>(InX) / clientWidth) * 2.0f - 1.0f;
    float ndc_y = -((static_cast<float>(InY) / clientHeight) * 2.0f - 1.0f); // Y축은 방향이 반대

    FVector3 ClickPosition(ndc_x, ndc_y, 0.0f);

    // Find Clicked Ball
    int clickedBallIndex = -1;
    for (int i = TotalPrimitives - 1; i >= 0; --i)
    {
        UBall* Ball = static_cast<UBall*>(PrimitiveList[i]);
        FVector3 Delta = ClickPosition - Ball->Location;
        Delta.z = 0;

        if (Delta.LengthSquare() < Ball->Radius * Ball->Radius)
        {
            clickedBallIndex = i;
            break;
        }
    }

    // If Click, Execute Logic
    if (clickedBallIndex != -1)
    {
        if (InIsLeftClick)
        {
            // 왼쪽 클릭: 해당 공 제거
            RemoveSpecificBall(clickedBallIndex);
        }
        else
        {
            // 오른쪽 클릭: 해당 공을 중력 중심으로 설정 / 해제
            SetGravityCenter(clickedBallIndex);
        }
    }
}
