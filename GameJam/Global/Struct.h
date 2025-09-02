/**
 * @brief 전역 struct 모음 File
 */

#include <cmath>

struct FVertexSimple
{
    float x, y, z; // Position
    float r, g, b, a; // Color
};

struct FVector3
{
    float x, y, z;

    FVector3(float InX = 0, float InY = 0, float InZ = 0) : x(InX), y(InY), z(InZ)
    {
    }

    // Vector operations
    FVector3 operator+(const FVector3& InOther) const
    {
        return FVector3(x + InOther.x, y + InOther.y, z + InOther.z);
    }

    FVector3& operator+=(const FVector3& InOther)
    {
        x += InOther.x;
        y += InOther.y;
        z += InOther.z;
        return *this;
    }

    FVector3 operator-(const FVector3& InOther) const
    {
        return FVector3(x - InOther.x, y - InOther.y, z - InOther.z);
    }

    FVector3& operator-=(const FVector3& InOther)
    {
        x -= InOther.x;
        y -= InOther.y;
        z -= InOther.z;
        return *this;
    }

    FVector3 operator*(float InScalar) const
    {
        return FVector3(x * InScalar, y * InScalar, z * InScalar);
    }

    FVector3& operator*=(float InScalar)
    {
        x *= InScalar;
        y *= InScalar;
        z *= InScalar;
        return *this;
    }

    FVector3 operator/(float InScalar) const
    {
        return FVector3(x / InScalar, y / InScalar, z / InScalar);
    }

    float LengthSquare() const { return x * x + y * y + z * z; }
    float Length() const { return std::sqrtf(LengthSquare()); }

    FVector3& Normalize()
    {
        float len = Length();
        if (len > 0.0f)
        {
            x /= len;
            y /= len;
            z /= len;
        }
        return *this;
    }
};

struct FConstants
{
	FVector3 Offset;
	float ScaleX;
	float ScaleY;
	float Pad[3];
};
