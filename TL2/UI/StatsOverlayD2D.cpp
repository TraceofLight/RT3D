#include "pch.h"
#include "StatsOverlayD2D.h"

#include <d2d1_1.h>
#include <dwrite.h>
#include <dxgi1_2.h>
#include "UI/UIManager.h"
#include "MemoryManager.h"

// FWindowsPlatformTime static 변수 초기화
double FWindowsPlatformTime::GSecondsPerCycle = 0.0;
bool FWindowsPlatformTime::bInitialized = false;

#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")

static inline void SafeRelease(IUnknown* p) { if (p) p->Release(); }

UStatsOverlayD2D& UStatsOverlayD2D::Get()
{
    static UStatsOverlayD2D Instance;
    return Instance;
}

void UStatsOverlayD2D::Initialize(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapChain)
{
    D3DDevice = device;
    D3DContext = context;
    SwapChain = swapChain;
    bInitialized = (D3DDevice && D3DContext && SwapChain);
}

void UStatsOverlayD2D::EnsureInitialized()
{
}

void UStatsOverlayD2D::ReleaseD2DTarget()
{
}

static void DrawTextBlock(
    ID2D1DeviceContext* d2dCtx,
    IDWriteFactory* dwrite,
    const wchar_t* text,
    const D2D1_RECT_F& rect,
    float fontSize,
    D2D1::ColorF bgColor,
    D2D1::ColorF textColor)
{
    if (!d2dCtx || !dwrite || !text) return;

    ID2D1SolidColorBrush* brushFill = nullptr;
    d2dCtx->CreateSolidColorBrush(bgColor, &brushFill);

    ID2D1SolidColorBrush* brushText = nullptr;
    d2dCtx->CreateSolidColorBrush(textColor, &brushText);

    d2dCtx->FillRectangle(rect, brushFill);

    IDWriteTextFormat* format = nullptr;
    dwrite->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        fontSize,
        L"en-us",
        &format);

    if (format)
    {
        format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        d2dCtx->DrawTextW(
            text,
            static_cast<UINT32>(wcslen(text)),
            format,
            rect,
            brushText);
        format->Release();
    }

    SafeRelease(brushText);
    SafeRelease(brushFill);
}

void UStatsOverlayD2D::UpdateRenderingStats(uint32 InDrawCalls, uint32 InMaterialChanges, uint32 InTextureChanges, uint32 InShaderChanges)
{
    // 현재 데이터 업데이트
    CurrentDrawCalls = InDrawCalls;
    CurrentMaterialChanges = InMaterialChanges;
    CurrentTextureChanges = InTextureChanges;
    CurrentShaderChanges = InShaderChanges;
    
    // 히스토리에 추가
    DrawCallsHistory[StatsHistoryIndex] = InDrawCalls;
    MaterialChangesHistory[StatsHistoryIndex] = InMaterialChanges;
    TextureChangesHistory[StatsHistoryIndex] = InTextureChanges;
    ShaderChangesHistory[StatsHistoryIndex] = InShaderChanges;
    
    // 추가 성능 지표 히스토리 업데이트
    PickingTimeHistory[StatsHistoryIndex] = CurrentPickingTime;
    AttemptsHistory[StatsHistoryIndex] = CurrentAttempts;
    AccumulatedTimeHistory[StatsHistoryIndex] = CurrentAccumulatedTime;
    
    StatsHistoryIndex = (StatsHistoryIndex + 1) % STATS_HISTORY_SIZE;
}

void UStatsOverlayD2D::UpdatePickingTime(double PickingTimeMs)
{
    CurrentPickingTime = PickingTimeMs;
    UpdateAccumulatedTime(PickingTimeMs);
}

void UStatsOverlayD2D::IncrementAttempts()
{
    ++CurrentAttempts;
}

void UStatsOverlayD2D::UpdateAccumulatedTime(double AccumTimeMs)
{
    CurrentAccumulatedTime += AccumTimeMs;
}

void UStatsOverlayD2D::Draw()
{
    if (!bInitialized || (!bShowFPS && !bShowMemory && !bShowRenderStats) || !SwapChain)
        return;
    
    // FWindowsPlatformTime 초기화 및 고성능 FPS 계산
    FWindowsPlatformTime::InitTiming();
    
    CurrentFrameTime = FPlatformTime::Cycles64();
    if (LastFrameTime != 0)
    {
        uint64_t CycleDiff = CurrentFrameTime - LastFrameTime;
        PreciseFrameTime = FPlatformTime::ToMilliseconds(CycleDiff);
        PreciseFPS = PreciseFrameTime > 0.0 ? (1000.0 / PreciseFrameTime) : 0.0;

        FPSHistory[FPSHistoryIndex] = PreciseFPS;
        FPSHistoryIndex = (FPSHistoryIndex + 1) % FPS_HISTORY_SIZE;
    }
    LastFrameTime = CurrentFrameTime;

    ID2D1Factory1* d2dFactory = nullptr;
    D2D1_FACTORY_OPTIONS opts{};
#ifdef _DEBUG
    opts.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &opts, (void**)&d2dFactory)))
        return;

    IDXGISurface* surface = nullptr;
    if (FAILED(SwapChain->GetBuffer(0, __uuidof(IDXGISurface), (void**)&surface)))
    {
        SafeRelease(d2dFactory);
        return;
    }

    IDXGIDevice* dxgiDevice = nullptr;
    if (FAILED(D3DDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice)))
    {
        SafeRelease(surface);
        SafeRelease(d2dFactory);
        return;
    }

    ID2D1Device* d2dDevice = nullptr;
    if (FAILED(d2dFactory->CreateDevice(dxgiDevice, &d2dDevice)))
    {
        SafeRelease(dxgiDevice);
        SafeRelease(surface);
        SafeRelease(d2dFactory);
        return;
    }

    ID2D1DeviceContext* d2dCtx = nullptr;
    if (FAILED(d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2dCtx)))
    {
        SafeRelease(d2dDevice);
        SafeRelease(dxgiDevice);
        SafeRelease(surface);
        SafeRelease(d2dFactory);
        return;
    }

    IDWriteFactory* dwrite = nullptr;
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&dwrite)))
    {
        SafeRelease(d2dCtx);
        SafeRelease(d2dDevice);
        SafeRelease(dxgiDevice);
        SafeRelease(surface);
        SafeRelease(d2dFactory);
        return;
    }

    D2D1_BITMAP_PROPERTIES1 bmpProps = {};
    bmpProps.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bmpProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    bmpProps.dpiX = 96.0f;
    bmpProps.dpiY = 96.0f;
    bmpProps.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

    ID2D1Bitmap1* targetBmp = nullptr;
    if (FAILED(d2dCtx->CreateBitmapFromDxgiSurface(surface, &bmpProps, &targetBmp)))
    {
        SafeRelease(dwrite);
        SafeRelease(d2dCtx);
        SafeRelease(d2dDevice);
        SafeRelease(dxgiDevice);
        SafeRelease(surface);
        SafeRelease(d2dFactory);
        return;
    }

    d2dCtx->SetTarget(targetBmp);

    d2dCtx->BeginDraw();
    const float margin = 12.0f;
    const float panelWidth = 200.0f;
    const float panelHeight = 48.0f;
    float nextY = margin;

    if (bShowFPS)
    {
        // FPS 히스토리에서 평균 계산 (더 안정적인 표시)
        double AvgFPS = 0.0;
        int ValidSamples = 0;
        for (int i = 0; i < FPS_HISTORY_SIZE; ++i)
        {
            if (FPSHistory[i] > 0.0)
            {
                AvgFPS += FPSHistory[i];
                ValidSamples++;
            }
        }
        if (ValidSamples > 0)
        {
            AvgFPS /= ValidSamples;
        }
        else
        {
            AvgFPS = PreciseFPS;
        }
        
        // 성능 지표 평균값 계산
        double AvgPickingTime = 0.0;
        uint32 AvgAttempts = 0;
        double AvgAccumulatedTime = 0.0;
        
        for (int i = 0; i < STATS_HISTORY_SIZE; ++i)
        {
            AvgPickingTime += PickingTimeHistory[i];
            AvgAttempts += AttemptsHistory[i];
            AvgAccumulatedTime += AccumulatedTimeHistory[i];
        }
        AvgPickingTime /= STATS_HISTORY_SIZE;
        AvgAttempts /= STATS_HISTORY_SIZE;
        AvgAccumulatedTime /= STATS_HISTORY_SIZE;

        // 스크린샷과 같은 형식으로 표시
        wchar_t buf[512];
        swprintf_s(buf, L"FPS : %.0f (%.0f ms)\nPicking Time %.0f ms : Num Attempts %u : Accumulated Time %.0f ms",
                  AvgFPS, PreciseFrameTime, AvgPickingTime, AvgAttempts, AvgAccumulatedTime);

        // 더 큰 패널 크기 (더 많은 정보 표시)
        const float extendedPanelWidth = 600.0f;
        const float extendedPanelHeight = 45.0f;
        
        D2D1_RECT_F rc = D2D1::RectF(margin, nextY, margin + extendedPanelWidth, nextY + extendedPanelHeight);
        DrawTextBlock(
            d2dCtx, dwrite, buf, rc, 16.0f,
            D2D1::ColorF(0, 0, 0, 0.7f),    // 약간 더 진한 배경
            D2D1::ColorF(0.0f, 1.0f, 0.0f, 1.0f));  // 밝은 녹색 텍스트 (스크린샷과 동일)

        nextY += extendedPanelHeight + 8.0f;
    }

    if (bShowMemory)
    {
        double mb = static_cast<double>(CMemoryManager::TotalAllocationBytes) / (1024.0 * 1024.0);

        wchar_t buf[128];
        swprintf_s(buf, L"Memory: %.1f MB\nAllocs: %u", mb, CMemoryManager::TotalAllocationCount);

        D2D1_RECT_F rc = D2D1::RectF(margin, nextY, margin + panelWidth, nextY + panelHeight);
        DrawTextBlock(
            d2dCtx, dwrite, buf, rc, 16.0f,
            D2D1::ColorF(0, 0, 0, 0.6f),
            D2D1::ColorF(D2D1::ColorF::LightGreen));
            
        nextY += panelHeight + 8.0f;
    }
    
    if (bShowRenderStats)
    {
        uint32 AvgDrawCalls = 0, AvgMaterialChanges = 0, AvgTextureChanges = 0, AvgShaderChanges = 0;
        
        for (int i = 0; i < STATS_HISTORY_SIZE; ++i)
        {
            AvgDrawCalls += DrawCallsHistory[i];
            AvgMaterialChanges += MaterialChangesHistory[i];
            AvgTextureChanges += TextureChangesHistory[i];
            AvgShaderChanges += ShaderChangesHistory[i];
        }
        AvgDrawCalls /= STATS_HISTORY_SIZE;
        AvgMaterialChanges /= STATS_HISTORY_SIZE;
        AvgTextureChanges /= STATS_HISTORY_SIZE;
        AvgShaderChanges /= STATS_HISTORY_SIZE;
        
        wchar_t Buffer[256];
        swprintf_s(Buffer, L"DrawCalls: %u\nMaterials: %u\nTextures: %u\nShaders: %u", 
                  AvgDrawCalls, AvgMaterialChanges, AvgTextureChanges, AvgShaderChanges);

        D2D1_RECT_F rc = D2D1::RectF(margin, nextY, margin + panelWidth, nextY + panelHeight * 1.5f);
        DrawTextBlock(
            d2dCtx, dwrite, Buffer, rc, 14.0f,
            D2D1::ColorF(0, 0, 0, 0.6f),
            D2D1::ColorF(D2D1::ColorF::Cyan));
    }

    d2dCtx->EndDraw();
    d2dCtx->SetTarget(nullptr);

    SafeRelease(targetBmp);
    SafeRelease(dwrite);
    SafeRelease(d2dCtx);
    SafeRelease(d2dDevice);
    SafeRelease(dxgiDevice);
    SafeRelease(surface);
    SafeRelease(d2dFactory);
}

void UStatsOverlayD2D::SetShowFPS(bool b)
{
    bShowFPS = b;
}

void UStatsOverlayD2D::SetShowMemory(bool b)
{
    bShowMemory = b;
}

void UStatsOverlayD2D::SetShowRenderStats(bool b)
{
    bShowRenderStats = b;
}

void UStatsOverlayD2D::ToggleFPS()
{
    bShowFPS = !bShowFPS;
}

void UStatsOverlayD2D::ToggleMemory()
{
    bShowMemory = !bShowMemory;
}

void UStatsOverlayD2D::ToggleRenderStats()
{
    bShowRenderStats = !bShowRenderStats;
}
