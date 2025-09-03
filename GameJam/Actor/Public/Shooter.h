#pragma once
#include "Mesh/Public/URectangle.h"

class FShooter
{
private:
	URectangle* Shape;
	float ChargingTime;

public:
	void Init();
	void Destroy();
	void Charging();
	void Shoot();

	FShooter();
	~FShooter();
};
