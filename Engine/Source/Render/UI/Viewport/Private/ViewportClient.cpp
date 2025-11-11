#include "pch.h"

#include "Render/UI/Viewport/Public/Viewport.h"
#include "Render/UI/Viewport/Public/ViewportClient.h"
#include "Manager/UI/Public/ViewportManager.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Level/Public/Level.h"
#include "Level/Public/World.h"
#include "Manager/Input/Public/InputManager.h"

FViewportClient::FViewportClient() = default;

FViewportClient::~FViewportClient() = default;

void FViewportClient::SetViewType(EViewType InType)
{
    const EViewType OldType = ViewType;
    ViewType = InType;

    // Set transform based on view type
    switch (InType)
    {
    case EViewType::Perspective:
        // Restore saved perspective location, rotation and far clip
        ViewLocation = SavedPerspectiveLocation;
        ViewRotation = SavedPerspectiveRotation;
        FarZ = SavedPerspectiveFarZ;
        break;

    case EViewType::OrthoTop:
        // Save current state if coming from perspective
        if (OldType == EViewType::Perspective)
        {
            SavedPerspectiveLocation = ViewLocation;
            SavedPerspectiveRotation = ViewRotation;
            SavedPerspectiveFarZ = FarZ;
        }

        FarZ = 5000.0f;
        OrthoZoom = UViewportManager::GetInstance().GetSharedOrthoZoom();
        // Top view: Looking down (-Z direction) with Pitch=-90
        ViewLocation = FVector(0.0f, 0.0f, 100.0f);
        ViewRotation = FVector(-90.0f, 0.0f, 0.0f);  // Pitch=-90, looking down
        break;

    case EViewType::OrthoBottom:
        if (OldType == EViewType::Perspective)
        {
            SavedPerspectiveLocation = ViewLocation;
            SavedPerspectiveRotation = ViewRotation;
            SavedPerspectiveFarZ = FarZ;
        }

        FarZ = 5000.0f;
        OrthoZoom = UViewportManager::GetInstance().GetSharedOrthoZoom();
        // Bottom view: Looking up (+Z direction) with Pitch=90
        ViewLocation = FVector(0.0f, 0.0f, -100.0f);
        ViewRotation = FVector(90.0f, 0.0f, 0.0f);  // Pitch=90, looking up
        break;

    case EViewType::OrthoFront:
        if (OldType == EViewType::Perspective)
        {
            SavedPerspectiveLocation = ViewLocation;
            SavedPerspectiveRotation = ViewRotation;
            SavedPerspectiveFarZ = FarZ;
        }

        FarZ = 5000.0f;
        OrthoZoom = UViewportManager::GetInstance().GetSharedOrthoZoom();
        // Front view: Looking forward (+X direction)
        ViewLocation = FVector(-100.0f, 0.0f, 0.0f);
        ViewRotation = FVector(0.0f, 0.0f, 0.0f);  // Neutral, looking forward
        break;

    case EViewType::OrthoBack:
        if (OldType == EViewType::Perspective)
        {
            SavedPerspectiveLocation = ViewLocation;
            SavedPerspectiveRotation = ViewRotation;
            SavedPerspectiveFarZ = FarZ;
        }

        FarZ = 5000.0f;
        OrthoZoom = UViewportManager::GetInstance().GetSharedOrthoZoom();
        // Back view: Looking backward (-X direction) with Yaw=180
        ViewLocation = FVector(100.0f, 0.0f, 0.0f);
        ViewRotation = FVector(0.0f, 180.0f, 0.0f);  // Yaw=180, looking backward
        break;

    case EViewType::OrthoRight:
        if (OldType == EViewType::Perspective)
        {
            SavedPerspectiveLocation = ViewLocation;
            SavedPerspectiveRotation = ViewRotation;
            SavedPerspectiveFarZ = FarZ;
        }

        FarZ = 5000.0f;
        OrthoZoom = UViewportManager::GetInstance().GetSharedOrthoZoom();
        // Right view: Looking right (+Y direction) with Yaw=90
        ViewLocation = FVector(0.0f, -100.0f, 0.0f);
        ViewRotation = FVector(0.0f, 90.0f, 0.0f);  // Yaw=90, looking right
        break;

    case EViewType::OrthoLeft:
        if (OldType == EViewType::Perspective)
        {
            SavedPerspectiveLocation = ViewLocation;
            SavedPerspectiveRotation = ViewRotation;
            SavedPerspectiveFarZ = FarZ;
        }

        FarZ = 5000.0f;
        OrthoZoom = UViewportManager::GetInstance().GetSharedOrthoZoom();
        // Left view: Looking left (-Y direction) with Yaw=-90
        ViewLocation = FVector(0.0f, 100.0f, 0.0f);
        ViewRotation = FVector(0.0f, -90.0f, 0.0f);  // Yaw=-90, looking left
        break;
    }
}

void FViewportClient::Tick() const
{
    // No longer needed - transform is managed directly
}

void FViewportClient::Draw(const FViewport* InViewport) const
{
    // No longer needed - rendering uses GetViewMatrix/GetProjectionMatrix directly
}

void FViewportClient::SetPlayerCameraManager(APlayerCameraManager* Manager)
{
    PlayerCameraManager = Manager;
}

/**
 * @brief Forward/Right/Up 벡터 계산 (Camera.cpp와 동일한 로직)
 * Perspective: ViewRotation으로부터 계산
 * Ortho: SetViewType에서 설정된 고정 방향 사용
 */
FVector FViewportClient::GetForward() const
{
    if (ViewType == EViewType::Perspective)
    {
        // ViewRotation (Pitch, Yaw, Roll)을 Quaternion으로 변환
        const FRotator Rotator(ViewRotation.X, ViewRotation.Y, ViewRotation.Z);
        const FQuat RotationQuat = Rotator.Quaternion();
        const FMatrix RotationMatrix = RotationQuat.ToRotationMatrix();
        const FVector4 Forward4 = FVector4::ForwardVector() * RotationMatrix;

        FVector Forward(Forward4.X, Forward4.Y, Forward4.Z);
        Forward.Normalize();
        return Forward;
    }

    // Ortho: ViewType별 고정 Forward 방향
    switch (ViewType)
    {
    case EViewType::OrthoTop:    return FVector(0, 0, -1);  // -Z (Looking down)
    case EViewType::OrthoBottom: return FVector(0, 0, 1);   // +Z (Looking up)
    case EViewType::OrthoFront:  return FVector(1, 0, 0);   // +X (Looking forward)
    case EViewType::OrthoBack:   return FVector(-1, 0, 0);  // -X (Looking backward)
    case EViewType::OrthoRight:  return FVector(0, 1, 0);   // +Y (Looking right)
    case EViewType::OrthoLeft:   return FVector(0, -1, 0);  // -Y (Looking left)
    default:                     return FVector(1, 0, 0);
    }
}

FVector FViewportClient::GetRight() const
{
    if (ViewType == EViewType::Perspective)
    {
        const FVector Forward = GetForward();
        const FVector WorldUp = FVector(0, 0, 1);
        FVector Right = WorldUp.Cross(Forward);

        if (Right.LengthSquared() < MATH_EPSILON)
        {
            Right = FVector(0, 1, 0);
        }

        Right.Normalize();
        return Right;
    }

    // Ortho: ViewType별 고정 Right 방향
    switch (ViewType)
    {
    case EViewType::OrthoTop:    return FVector(0, 1, 0);   // +Y (Right)
    case EViewType::OrthoBottom: return FVector(0, -1, 0);  // -Y (Right for bottom view)
    case EViewType::OrthoFront:  return FVector(0, 1, 0);   // +Y (Right)
    case EViewType::OrthoBack:   return FVector(0, -1, 0);  // -Y (Right for back view)
    case EViewType::OrthoRight:  return FVector(-1, 0, 0);  // -X (Right for right view)
    case EViewType::OrthoLeft:   return FVector(1, 0, 0);   // +X (Right for left view)
    default:                     return FVector(0, 1, 0);
    }
}

FVector FViewportClient::GetUp() const
{
    if (ViewType == EViewType::Perspective)
    {
        const FVector Forward = GetForward();
        const FVector Right = GetRight();
        FVector Up = Forward.Cross(Right);
        Up.Normalize();
        return Up;
    }

    // Ortho: ViewType별 고정 Up 방향
    switch (ViewType)
    {
    case EViewType::OrthoTop:    return FVector(1, 0, 0);   // +X (Up for top view)
    case EViewType::OrthoBottom: return FVector(1, 0, 0);   // +X (Up for bottom view)
    case EViewType::OrthoFront:  return FVector(0, 0, 1);   // +Z (Up)
    case EViewType::OrthoBack:   return FVector(0, 0, 1);   // +Z (Up)
    case EViewType::OrthoRight:  return FVector(0, 0, 1);   // +Z (Up)
    case EViewType::OrthoLeft:   return FVector(0, 0, 1);   // +Z (Up)
    default:                     return FVector(0, 0, 1);
    }
}

void FViewportClient::PrepareCamera(const D3D11_VIEWPORT& InViewport)
{
    if (PlayerCameraManager)  // PIE/Game mode
    {
        // Update aspect ratio from viewport (marks camera as dirty)
        if (InViewport.Width > 0.f && InViewport.Height > 0.f)
        {
            PlayerCameraManager->SetDefaultAspectRatio(InViewport.Width / InViewport.Height);
        }
        // Camera will be updated automatically in GetCameraCachePOV() if dirty
    }
    // Editor mode: No need to update anything, transform is managed directly
}

const FCameraConstants& FViewportClient::GetCameraConstants() const
{
    // PIE/Game mode: Use player camera manager
    if (PlayerCameraManager)
    {
        return PlayerCameraManager->GetCameraCachePOV().CameraConstants;
    }

    // Editor mode: Build FCameraConstants from own transform
    if (!OwningViewport)
    {
        return CachedCameraConstants;
    }

    const FRect& ViewportRect = OwningViewport->GetRect();
    const float AspectRatio = static_cast<float>(ViewportRect.Width) / static_cast<float>(ViewportRect.Height);

    CachedCameraConstants.View = GetViewMatrix();
    CachedCameraConstants.Projection = GetProjectionMatrix(AspectRatio);
    CachedCameraConstants.ViewWorldLocation = ViewLocation;
    CachedCameraConstants.NearClip = NearZ;
    CachedCameraConstants.FarClip = FarZ;

    return CachedCameraConstants;
}

FMinimalViewInfo FViewportClient::GetViewInfo() const
{
    // PIE/Game mode: Use player camera manager
    if (PlayerCameraManager)
    {
        return PlayerCameraManager->GetCameraCachePOV();
    }

    // Editor mode: Build FMinimalViewInfo from own transform
    FMinimalViewInfo ViewInfo;
    ViewInfo.Location = ViewLocation;

    // Convert ViewRotation (Pitch, Yaw, Roll) to Quaternion
    FVector Radians = FVector::GetDegreeToRadian(ViewRotation);
    FMatrix RotationMatrix = FMatrix::CreateFromYawPitchRoll(Radians.Y, Radians.X, Radians.Z);
    ViewInfo.Rotation = FQuat::FromRotationMatrix(RotationMatrix);

    ViewInfo.FOV = FOV;
    ViewInfo.AspectRatio = OwningViewport ? (static_cast<float>(OwningViewport->GetRect().Width) / static_cast<float>(OwningViewport->GetRect().Height)) : 1.777f;
    ViewInfo.NearClipPlane = NearZ;
    ViewInfo.FarClipPlane = FarZ;
    ViewInfo.ProjectionMode = IsOrtho() ? ECameraProjectionMode::Orthographic : ECameraProjectionMode::Perspective;
    ViewInfo.OrthoWidth = 0.0f;  // TODO: Calculate from OrthoZoom if needed
    ViewInfo.CameraConstants = GetCameraConstants();

    return ViewInfo;
}

const TArray<UPrimitiveComponent*>& FViewportClient::GetVisiblePrimitives() const
{
    return ViewFrustumCuller.GetRenderableObjects();
}

void FViewportClient::UpdateVisiblePrimitives(UWorld* InWorld)
{
    if (!InWorld || !InWorld->GetLevel())
    {
        return;
    }

    // Get camera constants from the appropriate source (PIE or Editor)
    const FCameraConstants& CameraConst = GetCameraConstants();

    // Perform frustum culling
    TArray<UPrimitiveComponent*> DynamicPrimitives = InWorld->GetLevel()->GetDynamicPrimitives();
    ViewFrustumCuller.Cull(
        InWorld->GetLevel()->GetStaticOctree(),
        DynamicPrimitives,
        CameraConst
    );
}

void FViewportClient::UpdateEditorCamera(float DeltaTime)
{
	if (!bEditorCameraEnabled || PlayerCameraManager) return;
	if (!bInputEnabled) return; // 프리뷰 창 hover 아닐 때 무시 (선택)

	auto& Input = UInputManager::GetInstance();

	// --- 회전 (RMB 드래그) ---
	if (Input.IsKeyDown(EKeyInput::MouseRight))
	{
		const FVector md = Input.GetMouseDelta();
		ViewRotation.Y += md.X * MouseSensitivityDegPerPixel;      // Yaw
		ViewRotation.X += -md.Y * MouseSensitivityDegPerPixel;     // Pitch
		ViewRotation.X = clamp(ViewRotation.X, -89.9f, 89.9f);
		ViewRotation.Z = 0.0f;
	}

	// --- 이동 (WASD + QE) ---
	if (!IsOrtho())
	{
		float Scale = MoveSpeedBase;

		FVector Direction = FVector::Zero();
		const FVector Forward   = GetForward();
		const FVector Right = GetRight();
		const FVector Up    = FVector(0,0,1);

		if (Input.IsKeyDown(EKeyInput::W)) Direction += Forward;
		if (Input.IsKeyDown(EKeyInput::S)) Direction -= Forward;
		if (Input.IsKeyDown(EKeyInput::D)) Direction += Right;
		if (Input.IsKeyDown(EKeyInput::A)) Direction -= Right;
		if (Input.IsKeyDown(EKeyInput::E)) Direction += Up;
		if (Input.IsKeyDown(EKeyInput::Q)) Direction -= Up;

		if (Direction.LengthSquared() > MATH_EPSILON)
		{
			Direction.Normalize();
			ViewLocation += Direction * Scale * DeltaTime;
		}
	}
}

/**
 * @brief View Matrix 계산
 * ViewLocation과 ViewRotation으로부터 View 행렬 생성
 */
FMatrix FViewportClient::GetViewMatrix() const
{
    FMatrix ViewMatrix;

    // ViewRotation을 사용하여 방향 벡터 계산
    FVector Radians = FVector::GetDegreeToRadian(ViewRotation);
    FMatrix RotationMatrix = FMatrix::CreateFromYawPitchRoll(Radians.Y, Radians.X, Radians.Z);

    FVector Forward = FMatrix::VectorMultiply(FVector::ForwardVector(), RotationMatrix);
    Forward.Normalize();

    const FVector WorldUp = FVector(0, 0, 1);
    FVector Right = WorldUp.Cross(Forward);

    // Forward가 거의 Z축과 평행한 경우 (Pitch ±90° 근처)
    if (Right.LengthSquared() < MATH_EPSILON)
    {
        // Y축을 대체 Right로 사용
        Right = FVector(0, 1, 0);
    }

    Right.Normalize();
    FVector Up = Forward.Cross(Right);
    Up.Normalize();

    // DirectX View Matrix 구성
    ViewMatrix.Data[0][0] = Right.X;
    ViewMatrix.Data[0][1] = Up.X;
    ViewMatrix.Data[0][2] = Forward.X;
    ViewMatrix.Data[0][3] = 0.0f;

    ViewMatrix.Data[1][0] = Right.Y;
    ViewMatrix.Data[1][1] = Up.Y;
    ViewMatrix.Data[1][2] = Forward.Y;
    ViewMatrix.Data[1][3] = 0.0f;

    ViewMatrix.Data[2][0] = Right.Z;
    ViewMatrix.Data[2][1] = Up.Z;
    ViewMatrix.Data[2][2] = Forward.Z;
    ViewMatrix.Data[2][3] = 0.0f;

    ViewMatrix.Data[3][0] = -Right.Dot(ViewLocation);
    ViewMatrix.Data[3][1] = -Up.Dot(ViewLocation);
    ViewMatrix.Data[3][2] = -Forward.Dot(ViewLocation);
    ViewMatrix.Data[3][3] = 1.0f;

    return ViewMatrix;
}

/**
 * Ortho Units Per Pixel 계산 (Camera.cpp와 동일한 UE 방식)
 * Factor = ViewportWidth / 500
 * UnitsPerPixel = (OrthoZoom / (ViewportWidth * 15)) * Factor
 *               = OrthoZoom / 7500  (ViewportWidth 약분됨)
 */
float FViewportClient::GetOrthoUnitsPerPixel(float ViewportWidth) const
{
    constexpr float CAMERA_ZOOM_DIV = 15.0f;
    constexpr float ORTHO_ZOOM_FACTOR_BASE = 500.0f;

    const float ZoomFactor = ViewportWidth / ORTHO_ZOOM_FACTOR_BASE;
    return (OrthoZoom / (ViewportWidth * CAMERA_ZOOM_DIV)) * ZoomFactor;
}

/**
 * @brief Projection Matrix 계산
 * FOV, NearZ, FarZ, AspectRatio로부터 Projection 행렬 생성
 */
FMatrix FViewportClient::GetProjectionMatrix(float AspectRatio) const
{
    FMatrix ProjectionMatrix;

    if (IsOrtho())
    {
        // Orthographic Projection Matrix (Camera.cpp 방식)
        // OrthoZoom 기반으로 뷰포트 크기에 비례하는 OrthoWidth 계산
        // 실제 ViewportWidth는 OwningViewport에서 가져와야 하지만,
        // AspectRatio만 전달되는 인터페이스이므로 가정값 사용
        constexpr float AssumedViewportWidth = 1920.0f;
        const float UnitsPerPixel = GetOrthoUnitsPerPixel(AssumedViewportWidth);
        const float OrthoWidth = UnitsPerPixel * AssumedViewportWidth * 0.5f;
        const float SafeAspect = max(0.1f, AspectRatio);
        const float OrthoHeight = OrthoWidth / SafeAspect;

        const float R = OrthoWidth * 0.5f;
        const float L = -R;
        const float T = OrthoHeight * 0.5f;
        const float B = -T;

        ProjectionMatrix.Data[0][0] = 2.0f / (R - L);
        ProjectionMatrix.Data[0][1] = 0.0f;
        ProjectionMatrix.Data[0][2] = 0.0f;
        ProjectionMatrix.Data[0][3] = 0.0f;

        ProjectionMatrix.Data[1][0] = 0.0f;
        ProjectionMatrix.Data[1][1] = 2.0f / (T - B);
        ProjectionMatrix.Data[1][2] = 0.0f;
        ProjectionMatrix.Data[1][3] = 0.0f;

        ProjectionMatrix.Data[2][0] = 0.0f;
        ProjectionMatrix.Data[2][1] = 0.0f;
        ProjectionMatrix.Data[2][2] = 1.0f / (FarZ - NearZ);
        ProjectionMatrix.Data[2][3] = 0.0f;

        ProjectionMatrix.Data[3][0] = -(R + L) / (R - L);
        ProjectionMatrix.Data[3][1] = -(T + B) / (T - B);
        ProjectionMatrix.Data[3][2] = -NearZ / (FarZ - NearZ);
        ProjectionMatrix.Data[3][3] = 1.0f;
    }
    else
    {
        // Perspective Projection Matrix
        const float FovRadians = FVector::GetDegreeToRadian(FOV);
        const float TanHalfFov = tan(FovRadians / 2.0f);

        ProjectionMatrix.Data[0][0] = 1.0f / (AspectRatio * TanHalfFov);
        ProjectionMatrix.Data[0][1] = 0.0f;
        ProjectionMatrix.Data[0][2] = 0.0f;
        ProjectionMatrix.Data[0][3] = 0.0f;

        ProjectionMatrix.Data[1][0] = 0.0f;
        ProjectionMatrix.Data[1][1] = 1.0f / TanHalfFov;
        ProjectionMatrix.Data[1][2] = 0.0f;
        ProjectionMatrix.Data[1][3] = 0.0f;

        ProjectionMatrix.Data[2][0] = 0.0f;
        ProjectionMatrix.Data[2][1] = 0.0f;
        ProjectionMatrix.Data[2][2] = FarZ / (FarZ - NearZ);
        ProjectionMatrix.Data[2][3] = 1.0f;

        ProjectionMatrix.Data[3][0] = 0.0f;
        ProjectionMatrix.Data[3][1] = 0.0f;
        ProjectionMatrix.Data[3][2] = -(FarZ * NearZ) / (FarZ - NearZ);
        ProjectionMatrix.Data[3][3] = 0.0f;
    }

    return ProjectionMatrix;
}

/**
 * @brief Projection Matrix의 역행렬 계산
 * 수치 안정성을 위한 분석적 역행렬
 */
FMatrix FViewportClient::GetProjectionMatrixInverse(float AspectRatio) const
{
    FMatrix InvProjection;

    if (IsOrtho())
    {
        // Orthographic Projection Matrix Inverse (Camera.cpp 방식과 동일)
        constexpr float AssumedViewportWidth = 1920.0f;
        const float UnitsPerPixel = GetOrthoUnitsPerPixel(AssumedViewportWidth);
        const float OrthoWidth = UnitsPerPixel * AssumedViewportWidth * 0.5f;
        const float SafeAspect = max(0.1f, AspectRatio);
        const float OrthoHeight = OrthoWidth / SafeAspect;

        const float R = OrthoWidth * 0.5f;
        const float L = -R;
        const float T = OrthoHeight * 0.5f;
        const float B = -T;

        InvProjection.Data[0][0] = (R - L) / 2.0f;
        InvProjection.Data[0][1] = 0.0f;
        InvProjection.Data[0][2] = 0.0f;
        InvProjection.Data[0][3] = 0.0f;

        InvProjection.Data[1][0] = 0.0f;
        InvProjection.Data[1][1] = (T - B) / 2.0f;
        InvProjection.Data[1][2] = 0.0f;
        InvProjection.Data[1][3] = 0.0f;

        InvProjection.Data[2][0] = 0.0f;
        InvProjection.Data[2][1] = 0.0f;
        InvProjection.Data[2][2] = (FarZ - NearZ);
        InvProjection.Data[2][3] = 0.0f;

        InvProjection.Data[3][0] = (R + L) / 2.0f;
        InvProjection.Data[3][1] = (T + B) / 2.0f;
        InvProjection.Data[3][2] = -NearZ;
        InvProjection.Data[3][3] = 1.0f;
    }
    else
    {
        // Perspective Projection Matrix Inverse
        const float FovRadians = FVector::GetDegreeToRadian(FOV);
        const float TanHalfFov = tan(FovRadians / 2.0f);

        InvProjection.Data[0][0] = AspectRatio * TanHalfFov;
        InvProjection.Data[0][1] = 0.0f;
        InvProjection.Data[0][2] = 0.0f;
        InvProjection.Data[0][3] = 0.0f;

        InvProjection.Data[1][0] = 0.0f;
        InvProjection.Data[1][1] = TanHalfFov;
        InvProjection.Data[1][2] = 0.0f;
        InvProjection.Data[1][3] = 0.0f;

        InvProjection.Data[2][0] = 0.0f;
        InvProjection.Data[2][1] = 0.0f;
        InvProjection.Data[2][2] = 0.0f;
        InvProjection.Data[2][3] = -(FarZ - NearZ) / (FarZ * NearZ);

        InvProjection.Data[3][0] = 0.0f;
        InvProjection.Data[3][1] = 0.0f;
        InvProjection.Data[3][2] = 1.0f;
        InvProjection.Data[3][3] = 1.0f / FarZ;
    }

    return InvProjection;
}
