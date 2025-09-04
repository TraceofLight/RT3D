#include "pch.h"
#include "Actor/Public/FlipperPair.h"
#include "Mesh/Public/UTriangle.h"
#include "Render/Public/Renderer.h"
#include "Manager/Public/InputManager.h"

void UFlipperPair::RecalcIsoscelesInRadius(UTriangle* T)
{
	float halfB = T->Base * 0.5f;
	float side = sqrtf(halfB * halfB + T->Height * T->Height);
	T->Radius = (T->Base * T->Height) / (T->Base + 2.f * side);
}

void UFlipperPair::ApplyConfigToPad(UFlipper& Pad,
	UFlipper::ESide Side,
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
	Pad.Shape->Restitution = PAD_REST_RESTITUTION;
}

void UFlipperPair::Init(const FPadPairConfig& InConfig)
{
	Config = InConfig;

	float midRad = DegToRad(Config.MidAngleDeg);
	float sweepRad = DegToRad(Config.SweepHalfDeg);
	float speedRad = DegToRad(Config.RotationSpeedDeg);

	// Left (음수 영역)
	float leftMin = -(midRad + sweepRad);
	float leftMax = -(midRad - sweepRad);
	float leftInit = leftMin;

	// Right (양수 영역)
	float rightMin = (midRad - sweepRad);
	float rightMax = (midRad + sweepRad);
	float rightInit = rightMax;

	const float leftX = Config.CenterX - Config.XOffset;
	const float rightX = Config.CenterX + Config.XOffset;
	const float bothY = Config.CenterY + Config.YOffset;

	ApplyConfigToPad(LeftPad, UFlipper::ESide::Left,
		leftMin, leftMax, leftInit,
		leftX, bothY,
		Config.KeyLeft, speedRad);

	ApplyConfigToPad(RightPad, UFlipper::ESide::Right,
		rightMin, rightMax, rightInit,
		rightX, bothY,
		Config.KeyRight, speedRad);
}

void UFlipperPair::Update(FInputManager* Input, float DeltaTime)
{
	LeftPad.HandleInput(Input, DeltaTime);
	RightPad.HandleInput(Input, DeltaTime);
}

void UFlipperPair::Render(const URenderer& Renderer)
{
	LeftPad.Render(Renderer);
	RightPad.Render(Renderer);
}
