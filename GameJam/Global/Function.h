#pragma once

/**
* @brief 벡터의 내적 함수
 * @param InLeft Vector 1
 * @param InRight Vector 2
 * @return 내적 연산 값
 */
static float Dot(const FVector3& InLeft, const FVector3& InRight)
{
    return InLeft.x * InRight.x + InLeft.y * InRight.y + InLeft.z * InRight.z;
}

/**
 * @brief 값의 범위를 지정된 최소값과 최대값 사이로 제한하는 함수
 * @param InValue 제한할 값
 * @param InMin 최소값
 * @param InMax 최대값
 * @return 제한된 값
 */
static inline float Clamp(float InValue, float InMin, float InMax)
{
	if (InValue < InMin)
	{
		return InMin;
	}
	if (InValue > InMax)
	{
		return InMax;
	}
	return InValue;
}

template<typename T>
static void SafeDelete(T& InDynamicObject)
{
	delete InDynamicObject;
	InDynamicObject = nullptr;
}

/** ==============
 *  각도 변환 헬퍼
 *  ============== */
static constexpr float PI_F = 3.14159265358979323846f;
static constexpr float DEG_TO_RAD = PI_F / 180.0f;
static constexpr float RAD_TO_DEG = 180.0f / PI_F;

static inline float DegToRad(float InDegrees) { return InDegrees * DEG_TO_RAD; }
static inline float RadToDeg(float InRadians) { return InRadians * RAD_TO_DEG; }
