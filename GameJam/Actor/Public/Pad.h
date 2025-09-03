#pragma once
#include "Core/Public/Primitive.h"

class UTriangle;
class URenderer;
class FInputManager;

class UPad
	: public UPrimitive
{
public:
	enum class ESide
	{
		Left,
		Right
	};

private:
	UTriangle* Shape;
	EKeyInput RotationKey;
	float RotationSpeed;
	bool bUseRotationLimit;
	float MinRotation;
	float MaxRotation;
	ESide Side;

public:
	void Init();
	void Destroy();

	UPad();
	~UPad() override;

	void Render(const URenderer& InRenderer);
	void HandleInput(FInputManager* InInput, float InDeltaTime);

	void SetRotationKey(EKeyInput InKey);
	UTriangle* GetShape() const;

	void ConfigueLeftPad();
	void ConfigueRightPad();
};

extern UPad GLeftPad;
extern UPad GRightPad;
