#pragma once
#include "Core/Public/Primitive.h"

/**
 * @brief 오목한 원형 충돌체 클래스
 * 핀볼 게임의 경계벽, 범퍼, 가이드 등을 위한 concave circle collision
 */
class UConcaveCircle :
	public UPrimitive
{
private:
	FVector3 m_Center; // 중심점
	float m_Radius; // 외부 반지름
	float m_InnerRadius; // 내부 반지름 (도넛 형태용, 0이면 일반 원)
	float m_ArcAngleStart; // 호의 시작 각도 (라디안)
	float m_ArcAngleEnd; // 호의 끝 각도 (라디안)
	bool m_bIsFullCircle; // 전체 원인지 여부

private:
	// 내부 유틸리티 함수들
	float NormalizeAngle(float Angle) const;
	bool IsAngleInArc(float Angle) const;

public:
	// 전체 원 생성자
	UConcaveCircle(const FVector3& InCenter, float InRadius);

	// 호(arc) 생성자
	UConcaveCircle(const FVector3& InCenter, float InRadius, float InStartAngle, float InEndAngle);

	~UConcaveCircle() = default;

	// Getter 함수들
	const FVector3& GetCenter() const { return m_Center; }
	float GetRadius() const { return m_Radius; }
	float GetInnerRadius() const { return m_InnerRadius; }
	float GetArcAngleStart() const { return m_ArcAngleStart; }
	float GetArcAngleEnd() const { return m_ArcAngleEnd; }
	bool IsFullCircle() const { return m_bIsFullCircle; }

	// Setter 함수들
	void SetCenter(const FVector3& InCenter) { m_Center = InCenter; }
	void SetRadius(float InRadius) { m_Radius = InRadius; }
	void SetInnerRadius(float InInnerRadius) { m_InnerRadius = InInnerRadius; }
	void SetArcAngles(float InStartAngle, float InEndAngle);

	// 충돌 검사 함수들
	bool IsPointInside(const FVector3& Point) const;
	bool IsPointInsideArc(const FVector3& Point) const;
	float GetDistanceToPoint(const FVector3& Point) const;
};
