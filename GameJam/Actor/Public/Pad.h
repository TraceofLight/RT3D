#pragma once
#include "Core/Public/Primitive.h"

class UTriangle;
class URenderer;
class FInputManager;

class UPad
	: public UPrimitive
{
private:
	UTriangle* Shape = nullptr;
	EKeyInput RotationKey = EKeyInput::End;
	float RotationSpeed = DegToRad(360.0f);
	bool bUseRotationLimit = true;
	float MinRotation = DegToRad(-120.0f);
	float MaxRotation = DegToRad(-60.0f);

public:
	void Init();
	void Destroy();

	UPad();
	~UPad() override;

	void Render(const URenderer& InRenderer);
	void HandleInput(FInputManager* InInput, float InDeltaTime);

	void SetRotationKey(EKeyInput InKey);
	UTriangle* GetShape() const;
};

extern UPad GLeftPad;
