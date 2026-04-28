#pragma once
#include "Core/Public/Primitive.h"

class UTriangle;
class URenderer;
class FInputManager;

class UFlipper :
	public UPrimitive
{
public:
	enum class ESide { Left, Right };

private:
	UTriangle* Shape;
	EKeyInput RotationKey = EKeyInput::End;
	float RotationSpeed = 0.f;
	bool bUseRotationLimit = false;
	float MinRotation = 0.f;
	float MaxRotation = 0.f;
	ESide Side = ESide::Left;

public:
	friend class UFlipperPair; // PadPair가 대칭 설정을 직접 주입할 수 있게 함

	void Init();
	void Destroy();

	UFlipper();
	~UFlipper() override;

	void Render(const URenderer& InRenderer);
	void HandleInput(FInputManager* InInput, float InDeltaTime);

	void SetRotationKey(EKeyInput InKey);
	UTriangle* GetShape() const;

	void ConfigueLeftPad();
	void ConfigueRightPad();
};
