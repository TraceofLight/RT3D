#include "pch.h"
#include "Editor/Public/ObjectPicker.h"

#include "Editor/Public/Editor.h"
#include "Editor/Public/Gizmo.h"
#include "Manager/UI/Public/ViewportManager.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Render/UI/Viewport/Public/Viewport.h"
#include "Render/UI/Viewport/Public/ViewportClient.h"

IMPLEMENT_CLASS(UObjectPicker, UObject)

UObjectPicker::~UObjectPicker()
{
	SafeRelease(HitProxyStagingTexture);
}

void UObjectPicker::PickGizmo(FViewportClient* InClient, const FRay& WorldRay, UGizmo& Gizmo, FVector& CollisionPoint)
{
	//Forward, Right, Up순으로 테스트할거임.
	//원기둥 위의 한 점 P, 축 위의 임의의 점 A에(기즈모 포지션) 대해, AP벡터와 축 벡터 V와 피타고라스 정리를 적용해서 점 P의 축부터의 거리 r을 구할 수 있음.
	//r이 원기둥의 반지름과 같다고 방정식을 세운 후 근의공식을 적용해서 충돌여부 파악하고 distance를 구할 수 있음.

	//FVector4 PointOnCylinder = WorldRay.Origin + WorldRay.Direction * X;
	//dot(PointOnCylinder - GizmoLocation)*Dot(PointOnCylinder - GizmoLocation) - Dot(PointOnCylinder - GizmoLocation, GizmoAxis)^2 = r^2 = radiusOfGizmo
	//이 t에 대한 방정식을 풀어서 근의공식 적용하면 됨.

	// 멀티 뷰포트 대응: 현재 뷰포트의 ViewportClient 기준으로 Viewport 정보 가져오기
	auto& ViewportManager = UViewportManager::GetInstance();
	const auto& Viewports = ViewportManager.GetViewports();
	const auto& Clients = ViewportManager.GetClients();

	int32 CurrentViewportIndex = -1;
	for (int32 i = 0; i < Clients.Num(); ++i)
	{
		if (Clients[i] == InClient)
		{
			CurrentViewportIndex = i;
			break;
		}
	}

	// 현재 뷰포트 기준으로 Scale 재계산 (멀티 뷰포트에서 각 뷰포트마다 다른 Scale 사용)
	if (CurrentViewportIndex != -1 && Gizmo.GetTargetComponent())
	{
		const D3D11_VIEWPORT& CurrentViewportInfo = Viewports[CurrentViewportIndex]->GetRenderRect();
		Gizmo.UpdateScale(InClient, CurrentViewportInfo);
	}

	FVector GizmoLocation = Gizmo.GetGizmoLocation();
	FVector GizmoAxises[3] = { FVector{1, 0, 0}, FVector{0, 1, 0}, FVector{0, 0, 1} };

	if (Gizmo.GetGizmoMode() == EGizmoMode::Scale || !Gizmo.IsWorldMode())
	{
		FQuaternion q = Gizmo.GetTargetComponent()->GetWorldRotationAsQuaternion();
		for (int i = 0; i < 3; i++)
		{
			// 쿼터니언을 사용해 기본 축을 회전시킵니다.
			GizmoAxises[i] = q.RotateVector(GizmoAxises[i]);
		}
	}

	FVector WorldRayOrigin{ WorldRay.Origin.X,WorldRay.Origin.Y ,WorldRay.Origin.Z };
	FVector WorldRayDirection(WorldRay.Direction.X, WorldRay.Direction.Y, WorldRay.Direction.Z);
	WorldRayDirection.Normalize();

	switch (Gizmo.GetGizmoMode())
	{
	case EGizmoMode::Translate:
	case EGizmoMode::Scale:
	{
		// 먼저 평면 충돌 검사 (우선순위 높음)
		const float GizmoScale = Gizmo.GetTranslateScale();
		const float PlaneSize = PLANE_GIZMO_SIZE * GizmoScale;
		const float PlaneOffset = PLANE_GIZMO_OFFSET * GizmoScale;

		// 평면 정보: {방향, 탄젠트1, 탄젠트2, 법선}
		struct FPlaneTestInfo
		{
			EGizmoDirection Direction;
			FVector Tangent1;
			FVector Tangent2;
			FVector Normal;
		};

		FPlaneTestInfo Planes[3] = {
			{EGizmoDirection::XY_Plane, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}},  // XY 평면
			{EGizmoDirection::XZ_Plane, {1, 0, 0}, {0, 0, 1}, {0, 1, 0}},  // XZ 평면
			{EGizmoDirection::YZ_Plane, {0, 1, 0}, {0, 0, 1}, {1, 0, 0}}   // YZ 평면
		};

		for (const FPlaneTestInfo& PlaneInfo : Planes)
		{
			// 로컬 좌표계 벡터
			FVector T1 = PlaneInfo.Tangent1;
			FVector T2 = PlaneInfo.Tangent2;
			FVector Normal = PlaneInfo.Normal;

			// World/Local 모드에 따라 회전 적용
			FQuaternion q = FQuaternion::Identity();
			bool bNeedRotation = (Gizmo.GetGizmoMode() == EGizmoMode::Scale || !Gizmo.IsWorldMode());
			if (bNeedRotation)
			{
				q = Gizmo.GetTargetComponent()->GetWorldRotationAsQuaternion();
				T1 = q.RotateVector(T1);
				T2 = q.RotateVector(T2);
				Normal = q.RotateVector(Normal);
			}

			// 레이와 평면 교차 테스트
			FVector HitPoint;
			if (IsRayCollideWithPlane(WorldRay, GizmoLocation, Normal, HitPoint))
			{
				// 교차점을 평면 로컬 좌표로 변환
				FVector WorldHit = HitPoint - GizmoLocation;

				// 회전이 적용된 경우, 역회전하여 로컬 좌표로 변환
				FVector LocalHit = WorldHit;
				if (bNeedRotation)
				{
					LocalHit = q.Inverse().RotateVector(WorldHit);
				}

				// 로컬 좌표계에서 U, V 계산
				float U = LocalHit.Dot(PlaneInfo.Tangent1);
				float V = LocalHit.Dot(PlaneInfo.Tangent2);

				// 평면 사각형 내부에 있는지 확인
				if (U >= PlaneOffset && U <= PlaneOffset + PlaneSize &&
				    V >= PlaneOffset && V <= PlaneOffset + PlaneSize)
				{
					CollisionPoint = HitPoint;
					Gizmo.SetGizmoDirection(PlaneInfo.Direction);
					return;
				}
			}
		}

		// 중심 구체 충돌 검사
		{
			const float SphereRadius = Gizmo.GetTranslateRadius() * CENTER_SPHERE_RADIUS_SCALE;
			if (CheckRaySphereCollision(WorldRayOrigin, WorldRayDirection, GizmoLocation, SphereRadius, CollisionPoint))
			{
				Gizmo.SetGizmoDirection(EGizmoDirection::Center);
				return;
			}
		}

		// 평면과 중심 구체와 충돌하지 않았으면 축 충돌 검사
		const float GizmoRadius = Gizmo.GetTranslateRadius();
		const float GizmoHeight = Gizmo.GetTranslateHeight();

		for (int a = 0; a < 3; a++)
		{
			bool bCollided = CheckRayCylinderCollision(WorldRayOrigin, WorldRayDirection, GizmoLocation, GizmoAxises[a],
			                              GizmoRadius, GizmoHeight, CollisionPoint);

			if (bCollided)
			{
				switch (a)
				{
				case 0:	Gizmo.SetGizmoDirection(EGizmoDirection::Forward);	return;
				case 1:	Gizmo.SetGizmoDirection(EGizmoDirection::Right);	return;
				case 2:	Gizmo.SetGizmoDirection(EGizmoDirection::Up);		return;
				}
			}
		}
	} break;
	case EGizmoMode::Rotate:
	{
		EGizmoDirection Dirs[3] = { EGizmoDirection::Forward, EGizmoDirection::Right, EGizmoDirection::Up };

		// 오쏘 뷰 World 모드: 카메라 방향에 따라 피킹할 축 결정
		const bool bIsOrtho = InClient->IsOrtho();
		const bool bIsWorld = Gizmo.IsWorldMode();
		int OrthoAxisIndex = -1;

		if (bIsOrtho && bIsWorld && !Gizmo.IsDragging())
		{
			// ViewportClient로부터 직접 Forward 방향 가져오기
			const FVector CamForward = InClient->GetForward();
			const float AbsX = abs(CamForward.X);
			const float AbsY = abs(CamForward.Y);
			const float AbsZ = abs(CamForward.Z);

			if (AbsZ > AbsX && AbsZ > AbsY)
			{
				OrthoAxisIndex = 2;  // Z축만 피킹
			}
			else if (AbsY > AbsX && AbsY > AbsZ)
			{
				OrthoAxisIndex = 1;  // Y축만 피킹
			}
			else
			{
				OrthoAxisIndex = 0;  // X축만 피킹
			}
		}

		for (int a = 0; a < 3; a++)
		{
			// 오쏘 뷰 World 모드: 해당 축만 피킹
			if (OrthoAxisIndex != -1 && a != OrthoAxisIndex)
			{
				continue;
			}

			if (IsRayCollideWithPlane(WorldRay, GizmoLocation, GizmoAxises[a], CollisionPoint))
			{
				FVector RadiusVector = CollisionPoint - GizmoLocation;
				if (Gizmo.IsInRadius(RadiusVector.Length()))
				{
					// 오쏘 뷰 World 모드: Full Ring 피킹
					const bool bShouldCheckQuarterRingAngle = !bIsOrtho || !bIsWorld;

					// Quarter ring 각도 범위 체크
					if (!Gizmo.IsDragging() && bShouldCheckQuarterRingAngle)
					{
						if (!IsCollisionPointInQuarterRing(CollisionPoint, GizmoLocation, GizmoAxises[a], a, Gizmo, InClient))
						{
							continue;
						}
					}
					switch (a)
					{
					case 0:	Gizmo.SetGizmoDirection(EGizmoDirection::Forward);	return;
					case 1:	Gizmo.SetGizmoDirection(EGizmoDirection::Right);	return;
					case 2:	Gizmo.SetGizmoDirection(EGizmoDirection::Up);		return;
					}
				}
			}
		}
	} break;
	default: break;
	}

	Gizmo.SetGizmoDirection(EGizmoDirection::None);
}

bool UObjectPicker::IsRayCollideWithPlane(const FRay& WorldRay, FVector PlanePoint, FVector Normal, FVector& PointOnPlane)
{
	FVector WorldRayOrigin{ WorldRay.Origin.X, WorldRay.Origin.Y ,WorldRay.Origin.Z };
	FVector WorldRayDirection{ WorldRay.Direction.X, WorldRay.Direction.Y, WorldRay.Direction.Z };

	if (std::fabsf(WorldRayDirection.Dot(Normal)) < RAY_PLANE_PARALLEL_THRESHOLD)
	{
		return false;
	}

	float Distance = (PlanePoint - WorldRayOrigin).Dot(Normal) / WorldRayDirection.Dot(Normal);

	if (Distance < 0)
	{
		return false;
	}

	PointOnPlane = WorldRayOrigin + WorldRayDirection * Distance;

	return true;
}

bool UObjectPicker::CheckRaySphereCollision(const FVector& RayOrigin, const FVector& RayDirection,
                                            const FVector& SphereCenter, float SphereRadius,
                                            FVector& OutCollisionPoint) const
{
	const FVector ToSphere = SphereCenter - RayOrigin;
	const float ProjectionLength = ToSphere.Dot(RayDirection);

	if (ProjectionLength <= 0.0f)
	{
		return false;
	}

	const FVector ClosestPoint = RayOrigin + RayDirection * ProjectionLength;
	const float DistanceToRay = (ClosestPoint - SphereCenter).Length();

	if (DistanceToRay <= SphereRadius)
	{
		OutCollisionPoint = ClosestPoint;
		return true;
	}

	return false;
}

bool UObjectPicker::CheckRayCylinderCollision(const FVector& RayOrigin, const FVector& RayDirection,
                                               const FVector& CylinderBase, const FVector& CylinderAxis,
                                               float CylinderRadius, float CylinderHeight,
                                               FVector& OutCollisionPoint) const
{
	const FVector DistanceVector = RayOrigin - CylinderBase;

	// Ax^2 + Bx + C = 0 (이차 방정식)
	const float A = 1.0f - static_cast<float>(pow(RayDirection.Dot(CylinderAxis), 2));
	const float B = RayDirection.Dot(DistanceVector) - RayDirection.Dot(CylinderAxis) * DistanceVector.Dot(CylinderAxis);
	const float C = static_cast<float>(DistanceVector.Dot(DistanceVector) - pow(DistanceVector.Dot(CylinderAxis), 2)) - CylinderRadius * CylinderRadius;

	// 판별식
	const float Det = B * B - A * C;
	if (Det < 0.0f)
	{
		return false;
	}

	const float SqrtDet = sqrtf(Det);

	// 두 개의 교점 확인 (가까운 것과 먼 것)
	const float X1 = (-B + SqrtDet) / A;
	const FVector Point1 = RayOrigin + RayDirection * X1;
	const float Height1 = (Point1 - CylinderBase).Dot(CylinderAxis);

	if (Height1 >= 0.0f && Height1 <= CylinderHeight)
	{
		OutCollisionPoint = Point1;
		return true;
	}

	const float X2 = (-B - SqrtDet) / A;
	const FVector Point2 = RayOrigin + RayDirection * X2;
	const float Height2 = (Point2 - CylinderBase).Dot(CylinderAxis);

	if (Height2 >= 0.0f && Height2 <= CylinderHeight)
	{
		OutCollisionPoint = Point2;
		return true;
	}

	return false;
}

bool UObjectPicker::IsCollisionPointInQuarterRing(const FVector& CollisionPoint, const FVector& GizmoLocation,
                                                   const FVector& GizmoAxis, int AxisIndex,
                                                   const UGizmo& Gizmo, const FViewportClient* InClient) const
{
	// 충돌점을 축 평면에 투영
	const FVector ToHit = CollisionPoint - GizmoLocation;
	FVector Projected = ToHit - (GizmoAxis * ToHit.Dot(GizmoAxis));
	const float ProjLen = Projected.Length();

	if (ProjLen < QUARTER_RING_MIN_PROJECTION)
	{
		return false;
	}

	Projected = Projected * (1.0f / ProjLen);

	// BaseAxis 계산 (각 축의 Quarter Ring이 놓인 평면의 기저 벡터)
	// AxisIndex=0 (X축/Forward): YZ 평면, AxisIndex=1 (Y축/Right): XZ 평면, AxisIndex=2 (Z축/Up): XY 평면
	static const FVector BaseAxisPairs[3][2] = {
		{ FVector(0, 0, 1), FVector(0, 1, 0) },  // X축: Z, Y
		{ FVector(1, 0, 0), FVector(0, 0, 1) },  // Y축: X, Z
		{ FVector(1, 0, 0), FVector(0, 1, 0) }   // Z축: X, Y
	};
	FVector BaseAxis0 = BaseAxisPairs[AxisIndex][0];
	FVector BaseAxis1 = BaseAxisPairs[AxisIndex][1];

	// Local 모드면 회전 적용
	if (!Gizmo.IsWorldMode())
	{
		const FQuaternion q = Gizmo.GetTargetComponent()->GetWorldRotationAsQuaternion();
		BaseAxis0 = q.RotateVector(BaseAxis0);
		BaseAxis1 = q.RotateVector(BaseAxis1);
	}

	// 플립 판정
	const FVector CameraLoc = InClient->GetViewLocation();
	const FVector DirectionToWidget = (GizmoLocation - CameraLoc).GetSafeNormal();
	const bool bMirrorAxis0 = (BaseAxis0.Dot(DirectionToWidget) <= 0.0f);
	const bool bMirrorAxis1 = (BaseAxis1.Dot(DirectionToWidget) <= 0.0f);
	const FVector StartDir = bMirrorAxis0 ? BaseAxis0 : -BaseAxis0;
	const FVector EndDir = bMirrorAxis1 ? BaseAxis1 : -BaseAxis1;

	// 충돌점이 StartDir와 EndDir 사이에 있는지 확인
	const float DotStart = Projected.Dot(StartDir);
	const float DotEnd = Projected.Dot(EndDir);

	return (DotStart >= 0.0f && DotEnd >= 0.0f);
}

UPrimitiveComponent* UObjectPicker::PickPrimitive(FViewportClient* InClient, int32 MouseX, int32 MouseY)
{
	if (!InClient)
	{
		return nullptr;
	}

	// HitProxy 패스 실행 (Click On Demand)
	URenderer& Renderer = URenderer::GetInstance();
	UViewportManager& ViewportManager = UViewportManager::GetInstance();

	int32 ActiveViewportIndex = ViewportManager.GetActiveIndex();
	if (ActiveViewportIndex < 0 || ActiveViewportIndex >= ViewportManager.GetViewports().Num())
	{
		return nullptr;
	}

	FViewport* ActiveViewport = ViewportManager.GetViewports()[ActiveViewportIndex];
	D3D11_VIEWPORT DXViewport = ActiveViewport->GetRenderRect();

	// HitProxy 렌더링
	Renderer.RenderHitProxyPass(InClient, DXViewport);

	// HitProxy 텍스처 픽셀 읽기
	FHitProxyId HitProxyId = ReadHitProxyAtLocation(MouseX, MouseY, DXViewport);

	if (!HitProxyId.IsValid())
	{
		return nullptr;
	}

	// HitProxyId로 HitProxy 객체 조회
	FHitProxyManager& HitProxyManager = FHitProxyManager::GetInstance();
	HHitProxy* HitProxy = HitProxyManager.GetHitProxy(HitProxyId);

	if (!HitProxy)
	{
		return nullptr;
	}

	// 기즈모 축인지 확인
	if (HitProxy->IsWidgetAxis())
	{
		HWidgetAxis* WidgetAxis = static_cast<HWidgetAxis*>(HitProxy);

		// 기즈모 축 설정 (Editor에서 드래그 처리)
		UEditor* Editor = GEditor->GetEditorModule();
		if (Editor && Editor->GetGizmo())
		{
			UGizmo* Gizmo = Editor->GetGizmo();

			// EGizmoAxisType -> EGizmoDirection 변환
			EGizmoDirection GizmoDir = EGizmoDirection::None;
			switch (WidgetAxis->Axis)
			{
			case EGizmoAxisType::X:
				GizmoDir = EGizmoDirection::Forward;
				break;
			case EGizmoAxisType::Y:
				GizmoDir = EGizmoDirection::Right;
				break;
			case EGizmoAxisType::Z:
				GizmoDir = EGizmoDirection::Up;
				break;
			case EGizmoAxisType::Center:
				GizmoDir = EGizmoDirection::Center;
				break;
			case EGizmoAxisType::XY:
				GizmoDir = EGizmoDirection::XY_Plane;
				break;
			case EGizmoAxisType::XZ:
				GizmoDir = EGizmoDirection::XZ_Plane;
				break;
			case EGizmoAxisType::YZ:
				GizmoDir = EGizmoDirection::YZ_Plane;
				break;
			default:
				break;
			}
			Gizmo->SetGizmoDirection(GizmoDir);
		}

		return nullptr;
	}

	// 일반 컴포넌트인 경우
	if (HitProxy->IsComponent())
	{
		HComponent* ComponentProxy = static_cast<HComponent*>(HitProxy);
		UPrimitiveComponent* Primitive = ComponentProxy->Component;

		if (Primitive)
		{
			return Primitive;
		}
	}

	return nullptr;
}

FHitProxyId UObjectPicker::ReadHitProxyAtLocation(int32 X, int32 Y, const D3D11_VIEWPORT& Viewport)
{
	URenderer& Renderer = URenderer::GetInstance();
	ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();
	UDeviceResources* DeviceResources = Renderer.GetDeviceResources();

	ID3D11Texture2D* HitProxyTexture = DeviceResources->GetHitProxyTexture();
	if (!HitProxyTexture)
	{
		return InvalidHitProxyId;
	}


	// Staging Texture 생성 (한 번만)
	CreateStagingTextureIfNeeded();
	if (!HitProxyStagingTexture)
	{
		return InvalidHitProxyId;
	}

	// HitProxy 텍스처 전체를 Staging으로 복사
	DeviceContext->CopyResource(HitProxyStagingTexture, HitProxyTexture);

	// GPU 커맨드 큐 플러시 (CopyResource가 완료될 때까지 대기)
	DeviceContext->Flush();

	// Staging 텍스처 맵핑 (블로킹, GPU 완료 대기)
	// TODO(KHJ): Deferred 처리하면 성능상의 이점은 당연하나, 현재 전반적으로 Forward 처리함
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	HRESULT hr = DeviceContext->Map(HitProxyStagingTexture, 0, D3D11_MAP_READ, 0, &MappedResource);
	if (FAILED(hr))
	{
		return InvalidHitProxyId;
	}

	// 뷰포트 크기 기준으로 바운드 체크
	int32 ViewportWidth = static_cast<int32>(Viewport.Width);
	int32 ViewportHeight = static_cast<int32>(Viewport.Height);

	if (X < 0 || X >= ViewportWidth || Y < 0 || Y >= ViewportHeight)
	{
		DeviceContext->Unmap(HitProxyStagingTexture, 0);
		return InvalidHitProxyId;
	}

	// 뷰포트 오프셋을 고려한 절대 좌표 계산
	int32 AbsoluteX = static_cast<int32>(Viewport.TopLeftX) + X;
	int32 AbsoluteY = static_cast<int32>(Viewport.TopLeftY) + Y;

	// RGBA8 픽셀 읽기
	uint8* RowStart = static_cast<uint8*>(MappedResource.pData) + AbsoluteY * MappedResource.RowPitch;
	uint8* PixelData = RowStart + AbsoluteX * 4;

	uint8 R = PixelData[0];
	uint8 G = PixelData[1];
	uint8 B = PixelData[2];

	DeviceContext->Unmap(HitProxyStagingTexture, 0);

	// HitProxyId 생성
	FHitProxyId HitProxyId(R, G, B);

	return HitProxyId;
}

void UObjectPicker::CreateStagingTextureIfNeeded()
{
	URenderer& Renderer = URenderer::GetInstance();
	ID3D11Device* Device = Renderer.GetDevice();
	UDeviceResources* DeviceResources = Renderer.GetDeviceResources();

	uint32 Width = DeviceResources->GetWidth();
	uint32 Height = DeviceResources->GetHeight();

	// 기존 Staging Texture가 있고 크기가 같으면 재사용
	if (HitProxyStagingTexture)
	{
		D3D11_TEXTURE2D_DESC ExistingDesc;
		HitProxyStagingTexture->GetDesc(&ExistingDesc);

		if (ExistingDesc.Width == Width && ExistingDesc.Height == Height)
		{
			return;
		}

		// 크기가 다르면 해제 후 재생성
		SafeRelease(HitProxyStagingTexture);
	}

	D3D11_TEXTURE2D_DESC StagingDesc = {};
	StagingDesc.Width = Width;
	StagingDesc.Height = Height;
	StagingDesc.MipLevels = 1;
	StagingDesc.ArraySize = 1;
	StagingDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	StagingDesc.SampleDesc.Count = 1;
	StagingDesc.SampleDesc.Quality = 0;
	StagingDesc.Usage = D3D11_USAGE_STAGING;
	StagingDesc.BindFlags = 0;
	StagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	StagingDesc.MiscFlags = 0;

	HRESULT hr = Device->CreateTexture2D(&StagingDesc, nullptr, &HitProxyStagingTexture);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("ObjectPicker: HitProxy Staging Texture 생성 실패");
	}
	else
	{
		UE_LOG_DEBUG("ObjectPicker: HitProxy Staging Texture 생성 %ux%u", Width, Height);
	}
}
