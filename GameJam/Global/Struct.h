#pragma once

/**
 * @brief 전역 struct 모음 File
 */

struct FVertexSimple
{
    float x, y, z; // Position
    float r, g, b, a; // Color
};

struct FVector2
{
    float x, y;

    FVector2(float InX = 0, float InY = 0) : x(InX), y(InY)
    {
    }

    // Vector operations
    FVector2 operator+(const FVector2& InOther) const
    {
        return FVector2(x + InOther.x, y + InOther.y);
    }

    FVector2& operator+=(const FVector2& InOther)
    {
        x += InOther.x;
        y += InOther.y;
        return *this;
    }

    FVector2 operator-(const FVector2& InOther) const
    {
        return FVector2(x - InOther.x, y - InOther.y);
    }

    FVector2& operator-=(const FVector2& InOther)
    {
        x -= InOther.x;
        y -= InOther.y;
        return *this;
    }

    FVector2 operator*(float InScalar) const
    {
        return FVector2(x * InScalar, y * InScalar);
    }

    FVector2& operator*=(float InScalar)
    {
        x *= InScalar;
        y *= InScalar;
        return *this;
    }

    FVector2 operator/(float InScalar) const
    {
        return FVector2(x / InScalar, y / InScalar);
    }

    float LengthSquare() const { return x * x + y * y; }
    float Length() const { return std::sqrtf(LengthSquare()); }

    FVector2& Normalize()
    {
        float len = Length();
        if (len > 0.0f)
        {
            x /= len;
            y /= len;
        }
        return *this;
    }
};

struct FVector3
{
    float x, y, z;

    FVector3(float InX = 0, float InY = 0, float InZ = 0) : x(InX), y(InY), z(InZ)
    {
    }

	FVector3 operator-() const
    {
    	return FVector3(-x, -y, -z);
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
	float Rotation;
	float Radius; // Triangle일 때 내접원 반지름, 다른 도형은 0
	float Pad[5]; // 16바이트 정렬(총 48바이트)
};
