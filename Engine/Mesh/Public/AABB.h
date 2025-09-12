#pragma once
#include "Mesh/Public/BoundingVolume.h"
#include "Global/Vector.h"

struct FAABB : public IBoundingVolume
{
	FVector Min;
	FVector Max;

	FAABB(const FVector& InMin, const FVector& InMax) : Min(InMin), Max(InMax) {}

	bool RaycastHit() const override;
	EBoundingVolumeType GetType() const override { return EBoundingVolumeType::AABB; }
};
