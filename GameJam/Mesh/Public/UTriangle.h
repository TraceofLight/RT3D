#pragma once
#include "Core/Public/Primitive.h"

class FInputManager;

class UTriangle
	: public UPrimitive
{
public:
	FVector3 Location; // Incenter (내접원 중심) 월드 좌표
	FVector3 Velocity; // 속도 벡터
	float Base; // 밑변 길이 (b)
	float Height; // 높이 (h)
	float Radius; // 내접원 반지름 (r) = (b*h) / (b+h+sqrt(b^2+h^2))
	float Rotation; // 라디안 (Incenter 기준 회전)
	float RotationSpeed; // 초당 회전 속도 (라디안)

	// 회전 제한 관련
	bool  bUseRotationLimit;
	float MinRotation;
	float MaxRotation;

	UTriangle();

	void UpdateRotation(FInputManager* InInput, float InDeltaTime);
	void SetRotationLimit(float InMinRadians, float InMaxRadians);
	void ClearRotationLimit();
};

extern UTriangle GTriangle;
