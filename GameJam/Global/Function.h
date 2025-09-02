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

struct FConstants
{
    FVector3 Offset;
    float Scale;
};
