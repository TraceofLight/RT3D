#include "pch.h"
#include "FViewportClient.h"

#include "CameraActor.h"
#include "CameraComponent.h"
#include "FViewport.h"
#include "Picking.h"
#include "SelectionManager.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/SceneView/Public/SceneView.h"
#include "Renderer/SceneViewFamily/Public/SceneViewFamily.h"

FVector FViewportClient::CameraAddPosition{};

FViewportClient::FViewportClient()
{
    ViewportType = EViewportType::Perspective;
    // 직교 뷰별 기본 카메라 설정
    Camera = NewObject<ACameraActor>();
    ViewPortCamera = Camera;
    SetupCameraMode();
}

FViewportClient::~FViewportClient()
{
}

void FViewportClient::Tick(float DeltaTime)
{
    if (PerspectiveCameraInput)
    {
        Camera->ProcessEditorCameraInput(DeltaTime);
    }
    MouseWheel(DeltaTime);
}

void FViewportClient::Draw(FViewport* Viewport)
{
    // TODO(KHJ): SceneRenderer 사용 전 코드. 필요 없으면 제거할 것
    // if (!Viewport || !World) return;
    //
    // // 뷰포트의 실제 크기로 aspect ratio 계산
    // float ViewportAspectRatio = static_cast<float>(Viewport->GetSizeX()) / static_cast<float>(Viewport->GetSizeY());
    // if (Viewport->GetSizeY() == 0) ViewportAspectRatio = 1.0f; // 0으로 나누기 방지
    //
    // switch (ViewportType)
    // {
    // case EViewportType::Perspective:
    // {
    //     ACameraActor* MainCamera = World->GetCameraActor();
    //     MainCamera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Perspective);
    //     if (Viewport->GetMainViewport()) {
    //         Camera = MainCamera;
    //     }
    //     Camera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Perspective);
    //     PerspectiveCameraPosition = Camera->GetActorLocation();
    //     PerspectiveCameraRotation = Camera->GetActorRotation();
    //     PerspectiveCameraFov = Camera->GetCameraComponent()->GetFOV();
    //       if (World)
    //       {
    //           World->SetViewModeIndex(ViewModeIndex);
    //           World->RenderViewports(Camera, Viewport);
    //           World->GetGizmoActor()->Render(Camera, Viewport);
    //       }
    //     break;
    // }
    // case EViewportType::Orthographic_Top:
    // case EViewportType::Orthographic_Front:
    // case EViewportType::Orthographic_Left:
    // case EViewportType::Orthographic_Back:
    // case EViewportType::Orthographic_Bottom:
    // case EViewportType::Orthographic_Right:
    // {
    //     Camera = ViewPortCamera;
    //     Camera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Orthographic);
    //     SetupCameraMode();
    //     // 월드의 모든 액터들을 렌더링
    //     if (World)
    //     {
    //         World->SetViewModeIndex(ViewModeIndex);
    //         World->RenderViewports(Camera, Viewport);
    //         
    //         World->GetGizmoActor()->Render(Camera, Viewport);
    //     }
    //     break;
    // }
    // }

    // RHI에서 렌더링 시작
    URHIDevice* RHI = FSceneRenderer::GetGlobalRHI();
    if (!RHI) return;

    RHI->BeginRender();

    try
    {
        // 카메라 설정 - 뷰포트 타입에 따라
        SetupCameraForViewportType();

        if (!Camera)
        {
            RHI->EndRender();
            return;
        }

        // SceneViewFamily 생성
        FSceneViewFamily* ViewFamily = new FSceneViewFamily;

        // ViewFamily 설정
        ViewFamily->SetRenderTarget(Viewport);
        ViewFamily->SetFeatureLevel(ERenderingFeatureLevel::SM5);
        ViewFamily->SetSceneRenderTargetsMode(ESceneRenderTargetsMode::SetTextures);
        ViewFamily->SetRealtimeUpdate(true);

        // SceneView 생성 및 ViewFamily에 추가
        FSceneView* SceneView = new FSceneView;
        SceneView->Initialize(Camera, Viewport, World);
        SceneView->SetViewModeIndex(ViewModeIndex);

        ViewFamily->AddView(SceneView);

        // SceneRenderer 임시 생성 및 렌더링
        FSceneRenderer* SceneRenderer = FSceneRenderer::CreateSceneRenderer(*ViewFamily);

        // 메인 렌더링 실행
        SceneRenderer->Render();

        // Gizmo 렌더링 (기존 방식 유지, Debug Pass 등으로 도입하면 분리할 것)
        if (World && World->GetGizmoActor())
        {
            World->GetGizmoActor()->Render(Camera, Viewport);
        }

        // 리소스 정리
        delete SceneRenderer;
        delete SceneView;
        delete ViewFamily;
    }
    catch (...)
    {
        // 예외 발생 시도 리소스 정리 보장
    }

    // RHI에서 렌더링 종료
    RHI->EndRender();
}

void FViewportClient::SetupCameraMode()
{
    Camera = ViewPortCamera;
    switch (ViewportType)
    {
    case EViewportType::Perspective:

        Camera->SetActorLocation(PerspectiveCameraPosition);
        Camera->SetActorRotation(PerspectiveCameraRotation);
        Camera->GetCameraComponent()->SetFOV(PerspectiveCameraFov);
        break;
    case EViewportType::Orthographic_Top:

        Camera->SetActorLocation({CameraAddPosition.X, CameraAddPosition.Y, 1000});
        Camera->SetActorRotation(FQuat::MakeFromEuler({0, 90, 0}));
        Camera->GetCameraComponent()->SetFOV(100);
        break;
    case EViewportType::Orthographic_Bottom:

        Camera->SetActorLocation({CameraAddPosition.X, CameraAddPosition.Y, -1000});
        Camera->SetActorRotation(FQuat::MakeFromEuler({0, -90, 0}));
        Camera->GetCameraComponent()->SetFOV(100);
        break;
    case EViewportType::Orthographic_Left:
        Camera->SetActorLocation({CameraAddPosition.X, 1000, CameraAddPosition.Z});
        Camera->SetActorRotation(FQuat::MakeFromEuler({0, 0, -90}));
        Camera->GetCameraComponent()->SetFOV(100);
        break;
    case EViewportType::Orthographic_Right:
        Camera->SetActorLocation({CameraAddPosition.X, -1000, CameraAddPosition.Z});
        Camera->SetActorRotation(FQuat::MakeFromEuler({0, 0, 90}));
        Camera->GetCameraComponent()->SetFOV(100);
        break;

    case EViewportType::Orthographic_Front:
        Camera->SetActorLocation({-1000, CameraAddPosition.Y, CameraAddPosition.Z});
        Camera->SetActorRotation(FQuat::MakeFromEuler({0, 0, 0}));
        Camera->GetCameraComponent()->SetFOV(100);
        break;
    case EViewportType::Orthographic_Back:
        Camera->SetActorLocation({1000, CameraAddPosition.Y, CameraAddPosition.Z});
        Camera->SetActorRotation(FQuat::MakeFromEuler({0, 0, 180}));
        Camera->GetCameraComponent()->SetFOV(100);
        break;
    }
}

void FViewportClient::SetupCameraForViewportType()
{
    if (!Camera || !ViewPortCamera) return;

    // 뷰포트 카메라를 현재 카메라로 설정
    Camera = ViewPortCamera;

    switch (ViewportType)
    {
    case EViewportType::Perspective:
        // 원근 투영 - 저장된 원근 카메라 설정 사용
        Camera->SetActorLocation(PerspectiveCameraPosition);
        Camera->SetActorRotation(PerspectiveCameraRotation);
        Camera->GetCameraComponent()->SetFOV(PerspectiveCameraFov);
        Camera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Perspective);
        break;

    case EViewportType::Orthographic_Top:
        // 상단 직교 뷰
        Camera->SetActorLocation({CameraAddPosition.X, CameraAddPosition.Y, 1000});
        Camera->SetActorRotation(FQuat::MakeFromEuler({0, 90, 0}));
        Camera->GetCameraComponent()->SetFOV(OrthographicZoom);
        Camera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Orthographic);
        break;

    case EViewportType::Orthographic_Bottom:
        // 하단 직교 뷰
        Camera->SetActorLocation({CameraAddPosition.X, CameraAddPosition.Y, -1000});
        Camera->SetActorRotation(FQuat::MakeFromEuler({0, -90, 0}));
        Camera->GetCameraComponent()->SetFOV(OrthographicZoom);
        Camera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Orthographic);
        break;

    case EViewportType::Orthographic_Left:
        // 왼쪽면 직교 뷰
        Camera->SetActorLocation({CameraAddPosition.X, 1000, CameraAddPosition.Z});
        Camera->SetActorRotation(FQuat::MakeFromEuler({0, 0, -90}));
        Camera->GetCameraComponent()->SetFOV(OrthographicZoom);
        Camera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Orthographic);
        break;

    case EViewportType::Orthographic_Right:
        // 오른쪽면 직교 뷰
        Camera->SetActorLocation({CameraAddPosition.X, -1000, CameraAddPosition.Z});
        Camera->SetActorRotation(FQuat::MakeFromEuler({0, 0, 90}));
        Camera->GetCameraComponent()->SetFOV(OrthographicZoom);
        Camera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Orthographic);
        break;

    case EViewportType::Orthographic_Front:
        // 정면 직교 뷰
        Camera->SetActorLocation({-1000, CameraAddPosition.Y, CameraAddPosition.Z});
        Camera->SetActorRotation(FQuat::MakeFromEuler({0, 0, 0}));
        Camera->GetCameraComponent()->SetFOV(OrthographicZoom);
        Camera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Orthographic);
        break;

    case EViewportType::Orthographic_Back:
        // 후면 직교 뷰
        Camera->SetActorLocation({1000, CameraAddPosition.Y, CameraAddPosition.Z});
        Camera->SetActorRotation(FQuat::MakeFromEuler({0, 0, 180}));
        Camera->GetCameraComponent()->SetFOV(OrthographicZoom);
        Camera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Orthographic);
        break;
    }
}

void FViewportClient::MouseMove(FViewport* Viewport, int32 X, int32 Y)
{
    World->GetGizmoActor()->ProcessGizmoInteraction(Camera, Viewport, static_cast<float>(X),
                                                    static_cast<float>(Y));

    if (!bIsMouseButtonDown && !World->GetGizmoActor()->GetbIsHovering() && bIsMouseRightButtonDown)
    // 직교투영이고 마우스 버튼이 눌려있을 때
    {
        if (ViewportType != EViewportType::Perspective)
        {
            int32 deltaX = X - MouseLastX;
            int32 deltaY = Y - MouseLastY;

            if (Camera && (deltaX != 0 || deltaY != 0))
            {
                // 기준 픽셀→월드 스케일
                const float basePixelToWorld = 0.05f;

                // 줌인(값↑)일수록 더 천천히 움직이도록 역수 적용
                float zoom = Camera->GetCameraComponent()->GetZoomFactor();
                zoom = (zoom <= 0.f) ? 1.f : zoom; // 안전장치
                const float pixelToWorld = basePixelToWorld * zoom;

                const FVector right = Camera->GetRight();
                const FVector up = Camera->GetUp();

                CameraAddPosition = CameraAddPosition
                    - right * (deltaX * pixelToWorld)
                    + up * (deltaY * pixelToWorld);

                SetupCameraMode();
            }

            MouseLastX = X;
            MouseLastY = Y;
        }
        else if (ViewportType == EViewportType::Perspective)
        {
            PerspectiveCameraInput = true;
        }
    }
}

void FViewportClient::MouseButtonDown(FViewport* Viewport, int32 X, int32 Y, int32 Button)
{
    if (!Viewport || !World) // Only handle left mouse button
        return;

    // Get viewport size
    FVector2D ViewportSize(static_cast<float>(Viewport->GetSizeX()),
                           static_cast<float>(Viewport->GetSizeY()));
    FVector2D ViewportOffset(static_cast<float>(Viewport->GetStartX()),
                             static_cast<float>(Viewport->GetStartY()));

    // X, Y are already local coordinates within the viewport, convert to global coordinates for picking
    FVector2D ViewportMousePos(static_cast<float>(X) + ViewportOffset.X,
                               static_cast<float>(Y) + ViewportOffset.Y);

    AActor* PickedActor;
    TArray<AActor*> AllActors = World->GetActors();

    if (Button == 0)
    {
        bIsMouseButtonDown = true;
        // 뷰포트의 실제 aspect ratio 계산
        float PickingAspectRatio = ViewportSize.X / ViewportSize.Y;
        if (ViewportSize.Y == 0) PickingAspectRatio = 1.0f; // 0으로 나누기 방지
        if (World->GetGizmoActor()->GetbIsHovering())
        {
            return;
        }
        PickedActor = CPickingSystem::PerformViewportPicking(
            AllActors, Camera, ViewportMousePos, ViewportSize, ViewportOffset, PickingAspectRatio,
            Viewport);
        
        if (PickedActor)
        {
            USelectionManager::GetInstance().SelectActor(PickedActor);
            UUIManager::GetInstance().SetPickedActor(PickedActor);
            if (World->GetGizmoActor())
            {
                World->GetGizmoActor()->SetTargetActor(PickedActor);
                World->GetGizmoActor()->SetActorLocation(PickedActor->GetActorLocation());
            }
        }
        else
        {
            UUIManager::GetInstance().ResetPickedActor();
            // Clear selection if nothing was picked
            USelectionManager::GetInstance().ClearSelection();
        }
    }
    else if (Button == 1)
    {
        //우클릭시 
        bIsMouseRightButtonDown = true;
        MouseLastX = X;
        MouseLastY = Y;
    }
}

void FViewportClient::MouseButtonUp(FViewport* Viewport, int32 X, int32 Y, int32 Button)
{
    if (Button == 0) // Left mouse button
    {
        bIsMouseButtonDown = false;
    }
    else
    {
        bIsMouseRightButtonDown = false;
        PerspectiveCameraInput = false;
    }
}

void FViewportClient::MouseWheel(float DeltaSeconds)
{
    if (!Camera) return;

    UCameraComponent* CameraComponent = Camera->GetCameraComponent();
    if (!CameraComponent) return;
    float WheelDelta = UInputManager::GetInstance().GetMouseWheelDelta();

    float zoomFactor = CameraComponent->GetZoomFactor();
    zoomFactor *= (1.0f - WheelDelta * DeltaSeconds * 3.0f);

    CameraComponent->SetZoomFactor(zoomFactor);
}
