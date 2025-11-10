#pragma once
#include "Render/HitProxy/Public/HitProxy.h"

class FOctree;
class UGizmo;
class FViewportClient;

class UObjectPicker :
	public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UObjectPicker, UObject)

public:
	UObjectPicker() = default;
	~UObjectPicker() override;

	void PickGizmo(FViewportClient* InClient, const FRay& WorldRay, UGizmo& Gizmo, FVector& CollisionPoint);
	UPrimitiveComponent* PickPrimitive(FViewportClient* InClient, int32 MouseX, int32 MouseY);
	bool IsRayCollideWithPlane(const FRay& WorldRay, FVector PlanePoint, FVector Normal, FVector& PointOnPlane);

private:
	FHitProxyId ReadHitProxyAtLocation(int32 X, int32 Y, const D3D11_VIEWPORT& Viewport);
	void CreateStagingTextureIfNeeded();

	// Gizmo picking helper functions
	bool CheckRaySphereCollision(const FVector& RayOrigin, const FVector& RayDirection,
	                             const FVector& SphereCenter, float SphereRadius,
	                             FVector& OutCollisionPoint) const;
	bool CheckRayCylinderCollision(const FVector& RayOrigin, const FVector& RayDirection,
	                               const FVector& CylinderBase, const FVector& CylinderAxis,
	                               float CylinderRadius, float CylinderHeight,
	                               FVector& OutCollisionPoint) const;
	bool IsCollisionPointInQuarterRing(const FVector& CollisionPoint, const FVector& GizmoLocation,
	                                   const FVector& GizmoAxis, int AxisIndex,
	                                   const UGizmo& Gizmo, const FViewportClient* InClient) const;

	// Gizmo picking constants
	static constexpr float CENTER_SPHERE_RADIUS_SCALE = 2.5f;   // Center 구체 반지름 배율 (렌더링 0.1 * Scale과 일치: 0.04 * 2.5 = 0.1)
	static constexpr float QUARTER_RING_MIN_PROJECTION = 0.001f; // Quarter Ring 최소 투영 길이
	static constexpr float RAY_PLANE_PARALLEL_THRESHOLD = 0.01f; // Ray-Plane 평행 판정 임계값

	ID3D11Texture2D* HitProxyStagingTexture = nullptr;
};
