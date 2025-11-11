#pragma once
#include "Physics/Public/BoundingVolume.h"
#include "Global/Vector.h"

struct FAABB : public IBoundingVolume
{
	FVector Min;
	FVector Max;

	FAABB() : Min(0.f, 0.f, 0.f), Max(0.f, 0.f, 0.f) {}
	FAABB(const FVector& InMin, const FVector& InMax) : Min(InMin), Max(InMax) {}
	FAABB(const TArray<FVector>& Points);

	FVector GetCenter() const { return (Min + Max) * 0.5f; }

	float GetCenterDistanceSquared(const FVector& Point) const;

	bool IsContains(const FAABB& Other) const;

	bool IsIntersected(const FAABB& Other) const;

	float GetSurfaceArea() const;

	bool RaycastHit() const override;

	float GetDistanceSquaredToPoint(const FVector& Point) const;

	EBoundingVolumeType GetType() const override { return EBoundingVolumeType::AABB; }

	FVector GetCorner(const uint32 InIdx) const;

	FAABB GetTransformedAABB(const FMatrix& TransformMatrix) const;
};

bool CheckIntersectionRayBox(const FRay& Ray, const FAABB& Box);

FAABB Union(const FAABB& Box1, const FAABB& Box2);
