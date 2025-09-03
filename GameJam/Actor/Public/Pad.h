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
	bool bUseRotationLimit = true;

public:
	void Init() override;
	void Destroy() override;

	void Render(const URenderer& InRenderer);
	void HandleInput(FInputManager* InInput, float InDeltaTime);

	UPad();
	~UPad() override;
};

