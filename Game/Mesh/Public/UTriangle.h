#pragma once
#include "Core/Public/Primitive.h"

class FInputManager;

class UTriangle
	: public UPrimitive
{
public:
	FVector3 Location; // Incenter (내접원 중심) 월드 좌표
	float Base; // 밑변 길이 (b)
	float Height; // 높이 (h)
	float Radius; // 내접원 반지름 = (B * H) / ( B + 2 * sqrt( (B/2)^2 + H^2 ) )
	float Rotation; // 라디안 (Incenter 기준 회전)
	float RotationSpeed; // 초당 회전 속도 (라디안)

	// 회전 제한 관련
	bool  bUseRotationLimit;
	float MinRotation;
	float MaxRotation;

	// 탄성 계수
	float Restitution = 1.0f;

	UTriangle();

	void UpdateRotation(FInputManager* InInput, float InDeltaTime);
	void SetRotationLimit(float InMinRadians, float InMaxRadians);
	void ClearRotationLimit();
};
