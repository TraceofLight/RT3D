#include "pch.h"
#include "Actor/Public/PadPair.h"
#include "Mesh/Public/UTriangle.h"
#include "Render/Public/Renderer.h"
#include "Manager/Public/InputManager.h"

void UPadPair::RecalcIsoscelesInRadius(UTriangle* T)
{
	float halfB = T->Base * 0.5f;
	float side = sqrtf(halfB * halfB + T->Height * T->Height);
	T->Radius = (T->Base * T->Height) / (T->Base + 2.f * side);
}

void UPadPair::ApplyConfigToPad(UPad& Pad,
	UPad::ESide Side,
	float MinRot,
	float MaxRot,
	float InitRot,
	float X,
	float Y,
	EKeyInput Key,
	float RotSpeedRad)
{
	if (!Pad.Shape)
		Pad.Shape = new UTriangle();

	Pad.Side = Side;
	Pad.Shape->Base = Config.Base;
	Pad.Shape->Height = Config.Height;
	RecalcIsoscelesInRadius(Pad.Shape);

	Pad.Shape->Location = FVector3(X, Y, 0.f);

	Pad.MinRotation = MinRot;
	Pad.MaxRotation = MaxRot;
	Pad.Shape->Rotation = InitRot;
	Pad.RotationSpeed = RotSpeedRad;
	Pad.bUseRotationLimit = true;
	Pad.RotationKey = Key;
}

void UPadPair::Init(const FPadPairConfig& InConfig)
{
	Config = InConfig;

	float midRad = DegToRad(Config.MidAngleDeg);
	float sweepRad = DegToRad(Config.SweepHalfDeg);
	float speedRad = DegToRad(Config.RotationSpeedDeg);

	// Left (음수 영역)
	float leftMin = -(midRad + sweepRad);
	float leftMax = -(midRad - sweepRad);
	float leftInit = leftMin; // 아래(휴식) 상태

	// Right (양수 영역)
	float rightMin = (midRad - sweepRad);
	float rightMax = (midRad + sweepRad);
	float rightInit = rightMax; // 아래(휴식) 상태(키 누르면 감소)

	ApplyConfigToPad(LeftPad, UPad::ESide::Left,
		leftMin, leftMax, leftInit,
		-Config.XOffset, Config.YOffset,
		Config.KeyLeft, speedRad);

	ApplyConfigToPad(RightPad, UPad::ESide::Right,
		rightMin, rightMax, rightInit,
		Config.XOffset, Config.YOffset,
		Config.KeyRight, speedRad);
}

void UPadPair::Update(FInputManager* Input, float DeltaTime)
{
	LeftPad.HandleInput(Input, DeltaTime);
	RightPad.HandleInput(Input, DeltaTime);
}

void UPadPair::Render(const URenderer& Renderer)
{
	LeftPad.Render(Renderer);
	RightPad.Render(Renderer);
}
