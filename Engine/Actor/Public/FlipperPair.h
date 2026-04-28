#pragma once
#include "Flipper.h"
#include "Global/Enum.h"

struct FPadPairConfig
{
	// 형상
	float Base = 0.1f;
	float Height = 0.5f;

	// 쌍 중심 (월드 좌표 인센터 기준) – 전체 이동용
	float CenterX = 0.0f;
	float CenterY = 0.0f;

	// 개별 패드 상대 위치 (Center 기준)
	float XOffset = 0.60f;   // 좌우 반거리 (Left = CenterX - XOffset, Right = CenterX + XOffset)
	float YOffset = -0.45f;  // 두 패드 공통 Y (CenterY + YOffset)

	// 회전 (도 단위)
	float MidAngleDeg = 90.0f;
	float SweepHalfDeg = 30.0f;
	float RotationSpeedDeg = 360.0f;

	// 입력 키
	EKeyInput KeyLeft = EKeyInput::A;
	EKeyInput KeyRight = EKeyInput::D;
};

class UFlipperPair
{
public:
	UFlipperPair() = default;

	void Init(const FPadPairConfig& InConfig);
	void Reconfigure(const FPadPairConfig& InConfig) { Init(InConfig); }

	void Update(FInputManager* Input, float DeltaTime);
	void Render(const URenderer& Renderer);

	const FPadPairConfig& GetConfig() const { return Config; }

	UFlipper& Left() { return LeftPad; }
	UFlipper& Right() { return RightPad; }
	const UFlipper& Left() const { return LeftPad; }
	const UFlipper& Right() const { return RightPad; }

private:
	UFlipper LeftPad;
	UFlipper RightPad;
	FPadPairConfig Config;

	void ApplyConfigToPad(UFlipper& Pad, UFlipper::ESide Side,
		float MinRot, float MaxRot, float InitRot,
		float X, float Y, EKeyInput Key, float RotSpeedRad);

	static float DegToRad(float d) { return d * 3.14159265358979323846f / 180.0f; }
	static void  RecalcIsoscelesInRadius(UTriangle* T);
};
