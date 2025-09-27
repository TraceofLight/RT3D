#pragma once
#include "SWindow.h"

class FViewport;
class ACameraActor;
class UWorld;

/**
 * @brief Viewport 단위 Rendering State를 관리하는 View Class
 *
 * @param Camera 뷰의 위치, 회전, FOV 등의 기본 정보를 제공하는 Camera Actor
 * @param Viewport 뷰가 렌더링될 대상 뷰포트 (화면 영역)
 * @param World 뷰가 속한 게임 월드 또는 레벨
 * @param ViewMatrix 월드 공간을 뷰 공간으로 변환하는 행렬
 * @param ProjectionMatrix 뷰 공간을 클립 공간으로 변환하는 행렬
 * @param ViewProjectionMatrix ViewMatrix와 ProjectionMatrix를 결합한 행렬 (최종 월드-클립 공간 변환)
 * @param ViewLocation 뷰의 월드 공간 위치
 * @param ViewRotation 뷰의 월드 공간 회전 (쿼터니언)
 * @param FOV 시야각 (Field of View)
 * @param NearClip 근접 클리핑 평면 (Near Clipping Plane)
 * @param FarClip 원거리 클리핑 평면 (Far Clipping Plane)
 * @param AspectRatio 뷰포트의 종횡비 (가로/세로 비율)
 * @param ViewModeIndex 렌더링할 뷰 모드 (예: Lit, Unlit, Wireframe 등)
 * @param ViewportSize 뷰포트의 픽셀 크기 (FVector2D)
 * @param ViewRect 뷰가 렌더링될 실제 사각형 영역
 */
class FSceneView
{
public:
    FSceneView();
    ~FSceneView();

    void Initialize(ACameraActor* InCamera, FViewport* InViewport, UWorld* InWorld);
    void UpdateViewMatrices();

    // Getter & Setter
    const FMatrix& GetViewMatrix() const { return ViewMatrix; }
    const FMatrix& GetProjectionMatrix() const { return ProjectionMatrix; }
    const FMatrix& GetViewProjectionMatrix() const { return ViewProjectionMatrix; }

    FVector GetViewLocation() const { return ViewLocation; }
    FQuat GetViewRotation() const { return ViewRotation; }

    FViewport* GetViewport() const { return Viewport; }
    ACameraActor* GetCamera() const { return Camera; }
    UWorld* GetWorld() const { return World; }

    EViewModeIndex GetViewModeIndex() const { return ViewModeIndex; }
    void SetViewModeIndex(EViewModeIndex InViewMode) { ViewModeIndex = InViewMode; }

    // Viewport Info
    FVector2D GetViewportSize() const { return ViewportSize; }
    FRect GetViewRect() const { return ViewRect; }

    // Frustum
    float GetFOV() const { return FOV; }
    float GetNearClippingPlane() const { return NearClip; }
    float GetFarClippingPlane() const { return FarClip; }

    bool IsLocationInFrustum(const FVector& InLocation) const;

private:
    ACameraActor* Camera = nullptr;
    FViewport* Viewport = nullptr;
    UWorld* World = nullptr;

    FMatrix ViewMatrix;
    FMatrix ProjectionMatrix;
    FMatrix ViewProjectionMatrix;

    FVector ViewLocation;
    FQuat ViewRotation;

    float FOV = 90.0f;
    float NearClip = 1.0f;
    float FarClip = 10000.0f;
    float AspectRatio = 1.777f;

    EViewModeIndex ViewModeIndex = EViewModeIndex::VMI_Lit;
    FVector2D ViewportSize;
    FRect ViewRect;
};
