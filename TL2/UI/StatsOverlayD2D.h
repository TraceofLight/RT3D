#pragma once

#include <d3d11.h>
#include <dxgi.h>

class FWindowsPlatformTime
{
public:
    static double GSecondsPerCycle; // 0
    static bool bInitialized; // false

    static void InitTiming()
    {
        if (!bInitialized)
        {
            bInitialized = true;

            double Frequency = (double)GetFrequency();
            if (Frequency <= 0.0)
            {
                Frequency = 1.0;
            }

            GSecondsPerCycle = 1.0 / Frequency;
        }
    }
    static float GetSecondsPerCycle()
    {
        if (!bInitialized)
        {
            InitTiming();
        }
        return (float)GSecondsPerCycle;
    }
    static uint64 GetFrequency()
    {
        LARGE_INTEGER Frequency;
        QueryPerformanceFrequency(&Frequency);
        return Frequency.QuadPart;
    }
    static double ToMilliseconds(uint64 CycleDiff)
    {
        double Ms = static_cast<double>(CycleDiff)
            * GetSecondsPerCycle()
            * 1000.0;

        return Ms;
    }

    static uint64 Cycles64()
    {
        LARGE_INTEGER CycleCount;
        QueryPerformanceCounter(&CycleCount);
        return (uint64)CycleCount.QuadPart;
    }
};

struct TStatId
{
};

typedef FWindowsPlatformTime FPlatformTime;

class FScopeCycleCounter
{
public:
    FScopeCycleCounter(TStatId StatId)
        : StartCycles(FPlatformTime::Cycles64())
        , UsedStatId(StatId)
    {
    }

    ~FScopeCycleCounter()
    {
        Finish();
    }

    uint64 Finish()
    {
        const uint64 EndCycles = FPlatformTime::Cycles64();
        const uint64 CycleDiff = EndCycles - StartCycles;

        // FThreadStats::AddMessage(UsedStatId, EStatOperation::Add, CycleDiff);

        return CycleDiff;
    }

private:
    uint64 StartCycles;
    TStatId UsedStatId;
};

class UStatsOverlayD2D
{
public:
    static UStatsOverlayD2D& Get();

    void Initialize(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapChain);
    void Draw();

    void SetShowFPS(bool b); 
    void SetShowMemory(bool b);
    void SetShowRenderStats(bool b);
    void ToggleFPS();
    void ToggleMemory();
    void ToggleRenderStats();
    bool IsFPSVisible() const { return bShowFPS; }
    bool IsMemoryVisible() const { return bShowMemory; }
    bool IsRenderStatsVisible() const { return bShowRenderStats; }
    
    // 렌더링 통계 업데이트
    void UpdateRenderingStats(uint32 InDrawCalls, uint32 InMaterialChanges,
                              uint32 InTextureChanges, uint32 InShaderChanges);
    
    // 성능 측정 관련 메서드
    void UpdatePickingTime(double PickingTimeMs);
    void IncrementAttempts();
    void UpdateAccumulatedTime(double AccumTimeMs);

private:
    UStatsOverlayD2D() = default;
    ~UStatsOverlayD2D() = default;
    UStatsOverlayD2D(const UStatsOverlayD2D&) = delete;
    UStatsOverlayD2D& operator=(const UStatsOverlayD2D&) = delete;

    void EnsureInitialized();
    void ReleaseD2DTarget();

private:
    bool bInitialized = false;
    bool bShowFPS = true;
    bool bShowMemory = true;
    bool bShowRenderStats = true;
    
    // 렌더링 통계 데이터
    uint32 CurrentDrawCalls = 0;
    uint32 CurrentMaterialChanges = 0;
    uint32 CurrentTextureChanges = 0;
    uint32 CurrentShaderChanges = 0;
    
    // 10프레임 평균
    static const int STATS_HISTORY_SIZE = 10;
    uint32 DrawCallsHistory[STATS_HISTORY_SIZE] = {0};
    uint32 MaterialChangesHistory[STATS_HISTORY_SIZE] = {0};
    uint32 TextureChangesHistory[STATS_HISTORY_SIZE] = {0};
    uint32 ShaderChangesHistory[STATS_HISTORY_SIZE] = {0};
    int StatsHistoryIndex = 0;
    
    // 고성능 FPS 측정을 위한 변수들
    uint64_t LastFrameTime = 0;
    uint64_t CurrentFrameTime = 0;
    double PreciseFPS = 0.0;
    double PreciseFrameTime = 0.0;
    
    // FPS 히스토리 (더 부드러운 표시를 위해)
    static const int FPS_HISTORY_SIZE = 30;
    double FPSHistory[FPS_HISTORY_SIZE] = {0};
    int FPSHistoryIndex = 0;
    
    // 추가 성능 지표들
    double CurrentPickingTime = 0.0;  // ms
    uint32 CurrentAttempts = 0;
    double CurrentAccumulatedTime = 0.0;  // ms
    
    // 성능 지표 히스토리
    double PickingTimeHistory[STATS_HISTORY_SIZE] = {0};
    uint32 AttemptsHistory[STATS_HISTORY_SIZE] = {0};
    double AccumulatedTimeHistory[STATS_HISTORY_SIZE] = {0};

    ID3D11Device* D3DDevice = nullptr;
    ID3D11DeviceContext* D3DContext = nullptr;
    IDXGISwapChain* SwapChain = nullptr;
};
