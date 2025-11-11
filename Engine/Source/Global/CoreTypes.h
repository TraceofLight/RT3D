#pragma once
#include "Vector.h"
#include "Matrix.h"
#include "Quaternion.h"
#include "Rotator.h"
#include "Runtime/CoreUObject/Public/Archive.h"

#define HAS_DIFFUSE_MAP	 (1 << 0)
#define HAS_AMBIENT_MAP	 (1 << 1)
#define HAS_SPECULAR_MAP (1 << 2)
#define HAS_NORMAL_MAP	 (1 << 3)
#define HAS_ALPHA_MAP	 (1 << 4)
#define HAS_BUMP_MAP	 (1 << 5)

struct FMaterialConstants
{
	FVector4 Ka;
	FVector4 Kd;
	FVector4 Ks;
	float Ns;
	float Ni;
	float D;
	uint32 MaterialFlags;
	float Time; // Time in seconds
};

struct FVertex
{
	FVector Position;
	FVector4 Color;
};

struct FNormalVertex
{
	FVector Position;
	FVector Normal;
	FVector4 Color;
	FVector2 TexCoord;
	FVector4 Tangent;  // XYZ: Tangent, W: Handedness(+1/-1)
};

inline FArchive& operator<<(FArchive& Ar, FNormalVertex& Vertex)
{
	Ar << Vertex.Position;
	Ar << Vertex.Normal;
	Ar << Vertex.Color;
	Ar << Vertex.TexCoord;
	Ar << Vertex.Tangent;
	return Ar;
}

struct FRay
{
	FVector4 Origin;
	FVector4 Direction;
};

/**
 * @brief Render State Settings for Actor's Component
 */
struct FRenderState
{
	ECullMode CullMode = ECullMode::None;
	EFillMode FillMode = EFillMode::Solid;
};

/**
 * @brief 변환 정보를 담는 구조체
 * Unreal Engine과 동일하게 내부적으로 Quaternion을 사용하여 Rotation 저장
 */
struct FTransform
{
	FVector Location = {};
	FQuat Rotation = {};
	FVector Scale = FVector(1.0f, 1.0f, 1.0f);

	FTransform() = default;

	FTransform(const FVector& InLocation, const FQuat& InRotation = FQuat::Identity(),
		const FVector& InScale = FVector::OneVector())
		: Location(InLocation), Rotation(InRotation), Scale(InScale)
	{
	}

	// FRotator를 받는 편의 생성자
	FTransform(const FVector& InLocation, const FRotator& InRotator, const FVector& InScale = FVector::OneVector())
		: Location(InLocation), Rotation(InRotator.Quaternion()), Scale(InScale)
	{
	}

	// Rotation 접근 편의 메서드
	FRotator GetRotation() const
	{
		return Rotation.ToRotator();
	}

	void SetRotation(const FRotator& InRotator)
	{
		Rotation = InRotator.Quaternion();
	}

	void SetRotation(const FQuat& InQuaternion)
	{
		Rotation = InQuaternion;
	}
};

inline FArchive& operator<<(FArchive& Ar, FTransform& Transform)
{
	Ar << Transform.Location;
	// Quaternion을 직렬화 (X, Y, Z, W)
	Ar << Transform.Rotation.X;
	Ar << Transform.Rotation.Y;
	Ar << Transform.Rotation.Z;
	Ar << Transform.Rotation.W;
	Ar << Transform.Scale;
	return Ar;
}

/**
 * @brief 2차원 좌표의 정보를 담는 구조체
 */
struct FPoint
{
	INT X = 0;
	INT Y = 0;
	constexpr FPoint(LONG InX, LONG InY) : X(InX), Y(InY)
	{
	}
};

/**
 * @brief 윈도우를 비롯한 2D 화면의 정보를 담는 구조체
 */
struct FRect
{
	LONG Left = 0;
	LONG Top = 0;
	LONG Width = 0;
	LONG Height = 0;

	LONG GetRight() const { return Left + Width; }
	LONG GetBottom() const { return Top + Height; }
};

struct FAmbientLightInfo
{
	FVector4 Color;
	float Intensity;
	FVector Padding;
};

struct FDirectionalLightInfo
{
	FVector4 Color;
	FVector Direction;
	float Intensity;

	// Shadow parameters
	// FMatrix LightViewProjection;
	uint32 CastShadow;           // 0 or 1
	uint32 ShadowModeIndex;
	float ShadowBias;
	float ShadowSlopeBias;
	float ShadowSharpen;
	float Resolution;
	FVector2 Padding;
};

//StructuredBuffer: 16-byte alignment required (FVector4 alignment)
struct FPointLightInfo
{
	FVector4 Color;
	FVector Position;
	float Intensity;
	float Range;
	float DistanceFalloffExponent;

	// Shadow parameters
	uint32 CastShadow;
	uint32 ShadowModeIndex;
	float ShadowBias;
	float ShadowSlopeBias;
	float ShadowSharpen;
	float Resolution;
};

//StructuredBuffer padding 없어도됨
struct FSpotLightInfo
{
	// Point Light와 공유하는 속성 (필드 순서 맞춤)
	FVector4 Color;
	FVector Position;
	float Intensity;
	float Range;
	float DistanceFalloffExponent;

	// SpotLight 고유 속성
	float InnerConeAngle;
	float OuterConeAngle;
	float AngleFalloffExponent;
	FVector Direction;

	// Shadow parameters
	FMatrix LightViewProjection;
	uint32 CastShadow;
	uint32 ShadowModeIndex;
	float ShadowBias;
	float ShadowSlopeBias;
	float ShadowSharpen;
	float Resolution;
	FVector2 Padding;
};

struct FGlobalLightConstant
{
	FAmbientLightInfo Ambient;
	FDirectionalLightInfo Directional;
};
