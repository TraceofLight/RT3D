#include "pch.h"
#include "SceneView/Public/SceneView.h"

#include "CameraActor.h"
#include "CameraComponent.h"
#include "FViewport.h"

/**
 * @brief FSceneView 클래스의 생성자로 모든 변환 행렬을 단위 행렬로 초기화
 */
FSceneView::FSceneView()
{
    ViewMatrix = FMatrix::Identity();
    ProjectionMatrix = FMatrix::Identity();
    ViewProjectionMatrix = FMatrix::Identity();
}

FSceneView::~FSceneView() = default;

/**
 * @brief FSceneView 객체를 초기화하고 뷰포트 정보를 설정하는 함수
 * 입력된 카메라, 뷰포트, 월드 포인터를 저장하고, 뷰포트의 크기와 종횡비를 계산
 * 최종적으로 UpdateViewMatrices()를 호출하여 뷰 행렬들을 계산한다
 * @param InCamera 뷰 정보를 제공할 ACameraActor 포인터
 * @param InViewport 뷰가 렌더링될 대상 FViewport 포인터
 * @param InWorld 뷰가 속한 UWorld 포인터
 */
void FSceneView::Initialize(ACameraActor* InCamera, FViewport* InViewport, UWorld* InWorld)
{
    Camera = InCamera;
    Viewport = InViewport;
    World = InWorld;

    if (Viewport)
    {
        ViewportSize = FVector2D(
            static_cast<float>(Viewport->GetSizeX()),
            static_cast<float>(Viewport->GetSizeY())
        );

        ViewRect = FRect(
            static_cast<float>(Viewport->GetStartX()),
            static_cast<float>(Viewport->GetStartY()),
            static_cast<float>(Viewport->GetStartX() + Viewport->GetSizeX()),
            static_cast<float>(Viewport->GetStartY() + Viewport->GetSizeY())
        );

        // Aspect Ratio 계산
        if (ViewportSize.Y > 0)
        {
            AspectRatio = ViewportSize.X / ViewportSize.Y;
        }
    }

    UpdateViewMatrices();
}

/**
 * @brief 카메라의 현재 위치, 회전 및 설정을 기반으로 뷰 행렬들을 업데이트하는 함수
 * 카메라 컴포넌트로부터 위치, FOV, 뷰 행렬, 투영 행렬을 가져와 계산
 */
void FSceneView::UpdateViewMatrices()
{
    if (!Camera || !Viewport)
    {
        return;
    }

    UCameraComponent* CameraComponent = Camera->GetCameraComponent();
    if (!CameraComponent)
    {
        return;
    }

    // Update View
    ViewLocation = Camera->GetActorLocation();
    ViewRotation = Camera->GetActorRotation();

    // Get Camera FOV
    FOV = CameraComponent->GetFOV();

    // 매트릭스 계산
    ViewMatrix = Camera->GetViewMatrix();
    ProjectionMatrix = Camera->GetProjectionMatrix(AspectRatio, Viewport);
    ViewProjectionMatrix = ViewMatrix * ProjectionMatrix;
}

/**
 * @brief 주어진 월드 공간 위치가 현재 뷰의 절두체 내부에 있는지 판정하는 함수
 * @param InLocation 판정할 월드 공간 위치
 * @return 위치가 절두체 내부에 있으면 true, 아니면 false
 */
bool FSceneView::IsLocationInFrustum(const FVector& InLocation) const
{
    // TODO(KHJ): Frustum Culling 판정 추가하여 클리핑 수행 필요
    return true;
}
