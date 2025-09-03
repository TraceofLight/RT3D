#pragma once
#include "Core/Public/Primitive.h"

class UPinBall :
	public UPrimitive
{
private:
	UBall* Shape;

public:
	void Init();
	void Destroy();

	// Special Member Function
	UPinBall();
	~UPinBall() override;
};
