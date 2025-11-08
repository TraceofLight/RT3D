#pragma once
#include "Global/CameraTypes.h"
#include "Optimization/Public/ViewVolumeCuller.h"

class FViewport;
class APlayerCameraManager;
class UWorld;

class FViewportClient
{
public:
    FViewportClient();
    ~FViewportClient();

    // FutureEngine 철학: ViewportClient는 자신이 속한 Viewport를 알아야 함
    void SetOwningViewport(FViewport* InViewport) { OwningViewport = InViewport; }
    FViewport* GetOwningViewport() const { return OwningViewport; }

public:
    // ---------- 구성/질의 ----------
    void        SetViewType(EViewType InType);
    EViewType   GetViewType() const { return ViewType; }

    void        SetViewMode(EViewModeIndex InMode) { ViewMode = InMode; }
    EViewModeIndex GetViewMode() const { return ViewMode; }

    bool        IsOrtho() const { return ViewType != EViewType::Perspective; }

    // ---------- Transform 접근 (카메라 대신 자체 Transform 사용) ----------
    void SetViewLocation(const FVector& InLocation) { ViewLocation = InLocation; }
    const FVector& GetViewLocation() const { return ViewLocation; }

    void SetViewRotation(const FVector& InRotation) { ViewRotation = InRotation; }
    const FVector& GetViewRotation() const { return ViewRotation; }

    // Forward/Right/Up 벡터 가져오기 (Camera.cpp와 동일)
    FVector GetForward() const;
    FVector GetRight() const;
    FVector GetUp() const;

    // Camera Parameters
    void SetFOV(float InFOV) { FOV = InFOV; }
    float GetFOV() const { return FOV; }

    void SetNearZ(float InNearZ) { NearZ = InNearZ; }
    float GetNearZ() const { return NearZ; }

    void SetFarZ(float InFarZ) { FarZ = InFarZ; }
    float GetFarZ() const { return FarZ; }

    void SetOrthoZoom(float InZoom) { OrthoZoom = InZoom; }
    float GetOrthoZoom() const { return OrthoZoom; }

    // Ortho Units Per Pixel 계산 (Camera.cpp와 동일)
    float GetOrthoUnitsPerPixel(float ViewportWidth) const;

    // Input enable (에디터 카메라 입력 제어용)
    void SetInputEnabled(bool bEnabled) { bInputEnabled = bEnabled; }
    bool GetInputEnabled() const { return bInputEnabled; }

    // View/Projection 행렬 계산
    FMatrix GetViewMatrix() const;
    FMatrix GetProjectionMatrix(float AspectRatio) const;
    FMatrix GetProjectionMatrixInverse(float AspectRatio) const;

public:
    void Tick() const;
    void Draw(const FViewport* InViewport) const;


    static void MouseMove(FViewport* /*Viewport*/, int32 /*X*/, int32 /*Y*/) {}
    void CapturedMouseMove(FViewport* /*Viewport*/, int32 X, int32 Y)
    {
        LastDrag = { X, Y };
    }

    // ---------- 리사이즈/포커스 ----------
    void OnResize(const FPoint& InNewSize) { ViewSize = InNewSize; }
    static void OnGainFocus() {}
    static void OnLoseFocus() {}
    static void OnClose() {}

    /**
     * Set player camera manager for PIE/Game mode
     * @param Manager Player camera manager, or nullptr to use editor transform
     */
    void SetPlayerCameraManager(APlayerCameraManager* Manager);

    /**
     * Prepare camera for rendering (update transform based on viewport)
     * Should be called before GetCameraConstants() each frame
     * @param InViewport D3D11 viewport information for aspect ratio
     */
    void PrepareCamera(const D3D11_VIEWPORT& InViewport);

    /**
     * Get camera constants for rendering
     * Uses player camera manager in PIE/Game mode, otherwise uses editor transform
     */
    const FCameraConstants& GetCameraConstants() const;

    /**
     * Get complete view information for rendering
     * Returns FMinimalViewInfo with all camera parameters needed for rendering
     * @return View info from player camera manager (PIE/Game) or editor transform
     */
    FMinimalViewInfo GetViewInfo() const;

    /**
     * Get visible primitives after frustum culling
     * Returns the cached result of view frustum culling performed in PrepareCamera()
     * @return Array of visible primitive components
     */
    const TArray<UPrimitiveComponent*>& GetVisiblePrimitives() const;

    /**
     * Perform view frustum culling for the current world
     * Should be called before rendering to update visible primitives
     * @param InWorld World to cull primitives from
     */
    void UpdateVisiblePrimitives(UWorld* InWorld);

private:
    // 상태
    EViewType       ViewType = EViewType::Perspective;
    EViewModeIndex  ViewMode = EViewModeIndex::VMI_Gouraud;

    FViewport* OwningViewport = nullptr;  // FutureEngine: 소속 Viewport 참조

    // View Transform (카메라를 소유하지 않고 Transform만 관리)
    FVector ViewLocation = FVector(-15.0f, 0.f, 10.0f);
    FVector ViewRotation = FVector(0, 0, 0);  // (Pitch, Yaw, Roll)
    float FOV = 60.0f;
    float NearZ = 0.1f;
    float FarZ = 4000.0f;
    float OrthoZoom = 1.0f;  // Orthographic zoom level

    // Saved perspective state for restoration
    FVector SavedPerspectiveLocation = FVector(-15.0f, 0.f, 10.0f);
    FVector SavedPerspectiveRotation = FVector(0, 0, 0);
    float SavedPerspectiveFarZ = 1000.0f;

    // PIE/Game 모드용 카메라 매니저 (에디터는 자체 Transform 사용)
    APlayerCameraManager* PlayerCameraManager = nullptr;

    // Cached camera constants (에디터 모드용)
    mutable FCameraConstants CachedCameraConstants;

    // 뷰/입력 상태
    FPoint		ViewSize{ 0, 0 };
    FPoint		LastDrag{ 0, 0 };
    bool        bInputEnabled = false;  // 입력 활성화 여부

    // Orthographic 뷰포트의 기준 높이 (픽셀 밀도 유지용)
    mutable float OrthoReferenceHeight = 0.0f;

    // View frustum culling (독립적으로 관리)
    ViewVolumeCuller ViewFrustumCuller;
};
