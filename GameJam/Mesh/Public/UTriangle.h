#pragma once
#include "Core/Public/Primitive.h"

class UTriangle
	: public UPrimitive
{
public:
	FVector3 Location; // Incenter (내접원 중심) 월드 좌표
	FVector3 Velocity; // 속도 벡터
	float Base; // 밑변 길이 (b)
	float Height; // 높이 (h) : 밑변에서 꼭짓점까지
	float Rotation; // 라디안 (Incenter 기준 회전)

	UTriangle();
};

extern UTriangle GTriangle;
