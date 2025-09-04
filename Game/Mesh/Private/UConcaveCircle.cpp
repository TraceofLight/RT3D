#include "pch.h"
#include "Mesh/Public/UConcaveCircle.h"
#include <cmath>

UConcaveCircle::UConcaveCircle(const FVector3& InCenter, float InRadius)
    : m_Center(InCenter)
    , m_Radius(InRadius)
    , m_InnerRadius(0.0f)
    , m_ArcAngleStart(0.0f)
    , m_ArcAngleEnd(2.0f * 3.14159265359f)
    , m_bIsFullCircle(true)
{
}

UConcaveCircle::UConcaveCircle(const FVector3& InCenter, float InRadius, float InStartAngle, float InEndAngle)
    : m_Center(InCenter)
    , m_Radius(InRadius)
    , m_InnerRadius(0.0f)
    , m_ArcAngleStart(InStartAngle)
    , m_ArcAngleEnd(InEndAngle)
    , m_bIsFullCircle(false)
{
}

void UConcaveCircle::SetArcAngles(float InStartAngle, float InEndAngle)
{
    m_ArcAngleStart = InStartAngle;
    m_ArcAngleEnd = InEndAngle;
    m_bIsFullCircle = false;
}

bool UConcaveCircle::IsPointInside(const FVector3& Point) const
{
    FVector3 diff = Point - m_Center;
    float distanceSquared = diff.LengthSquare();
    
    // 외부 반지름 안에 있는지 확인
    if (distanceSquared > m_Radius * m_Radius)
        return false;
    
    // 내부 반지름이 있다면 그 밖에 있는지 확인 (도넛 형태)
    if (m_InnerRadius > 0.0f && distanceSquared < m_InnerRadius * m_InnerRadius)
        return false;
    
    // 호(arc) 형태라면 각도 범위 안에 있는지 확인
    if (!m_bIsFullCircle)
    {
        return IsPointInsideArc(Point);
    }
    
    return true;
}

bool UConcaveCircle::IsPointInsideArc(const FVector3& Point) const
{
    FVector3 diff = Point - m_Center;
    float angle = atan2f(diff.y, diff.x);
    return IsAngleInArc(angle);
}

float UConcaveCircle::GetDistanceToPoint(const FVector3& Point) const
{
    FVector3 diff = Point - m_Center;
    float distance = diff.Length();
    
    if (m_bIsFullCircle)
    {
        // 전체 원의 경우
        if (m_InnerRadius > 0.0f)
        {
            // 도넛 형태
            if (distance < m_InnerRadius)
                return m_InnerRadius - distance;
            else if (distance > m_Radius)
                return distance - m_Radius;
            else
                return 0.0f; // 내부에 있음
        }
        else
        {
            // 일반 원
            return m_Radius - distance;
        }
    }
    else
    {
        // 호(arc) 형태의 경우 더 복잡한 계산 필요
        float angle = atan2f(diff.y, diff.x);
        if (IsAngleInArc(angle))
        {
            return m_Radius - distance;
        }
        else
        {
            // 호의 끝점들과의 거리를 계산
            FVector3 startPoint(m_Center.x + m_Radius * cosf(m_ArcAngleStart),
                              m_Center.y + m_Radius * sinf(m_ArcAngleStart),
                              m_Center.z);
            FVector3 endPoint(m_Center.x + m_Radius * cosf(m_ArcAngleEnd),
                            m_Center.y + m_Radius * sinf(m_ArcAngleEnd),
                            m_Center.z);
            
            float distToStart = (Point - startPoint).Length();
            float distToEnd = (Point - endPoint).Length();
            
            return fminf(distToStart, distToEnd);
        }
    }
}

float UConcaveCircle::NormalizeAngle(float Angle) const
{
    const float TWO_PI = 2.0f * 3.14159265359f;
    while (Angle < 0.0f) Angle += TWO_PI;
    while (Angle >= TWO_PI) Angle -= TWO_PI;
    return Angle;
}

bool UConcaveCircle::IsAngleInArc(float Angle) const
{
    float normalizedAngle = NormalizeAngle(Angle);
    float normalizedStart = NormalizeAngle(m_ArcAngleStart);
    float normalizedEnd = NormalizeAngle(m_ArcAngleEnd);
    
    if (normalizedStart <= normalizedEnd)
    {
        return normalizedAngle >= normalizedStart && normalizedAngle <= normalizedEnd;
    }
    else
    {
        // 호가 0도를 넘나드는 경우
        return normalizedAngle >= normalizedStart || normalizedAngle <= normalizedEnd;
    }
}
