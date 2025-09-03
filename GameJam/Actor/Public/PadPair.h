#pragma once
#include "Pad.h"
#include "Global/Enum.h"

struct FPadPairConfig
{
	// 형상
	float Base = 0.1f;
	float Height = 0.5f;

	// 위치
	float XOffset = 0.60f;   // 인센터 기준 좌우 거리
	float YOffset = -0.45f;

	// 회전 (도 단위)
	float MidAngleDeg = 90.0f;  // 중심각
	float SweepHalfDeg = 30.0f;  // 중심에서 위/아래 절반
	float RotationSpeedDeg = 360.0f;

	// 입력 키
	EKeyInput KeyLeft = EKeyInput::A;
	EKeyInput KeyRight = EKeyInput::D;
};

class UPadPair
{
public:
	UPadPair() = default;

	void Init(const FPadPairConfig& InConfig);
	void Reconfigure(const FPadPairConfig& InConfig) { Init(InConfig); }

	void Update(FInputManager* Input, float DeltaTime);
	void Render(const URenderer& Renderer);

	UPad& Left() { return LeftPad; }
	UPad& Right() { return RightPad; }
	const UPad& Left() const { return LeftPad; }
	const UPad& Right() const { return RightPad; }

private:
	UPad LeftPad;
	UPad RightPad;
	FPadPairConfig Config;

	void ApplyConfigToPad(UPad& Pad, UPad::ESide Side, float MinRot, float MaxRot, float InitRot, float X, float Y, EKeyInput Key, float RotSpeedRad);

	static float DegToRad(float d) { return d * 3.14159265358979323846f / 180.0f; }
	static void  RecalcIsoscelesInRadius(UTriangle* T);
};

extern UPadPair GPadPair;
