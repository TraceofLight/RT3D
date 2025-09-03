#pragma once
#include "Mesh/Public/URectangle.h"

class UShooter :
	public UPrimitive
{
private:
	URectangle* Shape;
	float ChargingTime;

public:
	void Init() override;
	void Destroy() override;
	void Charging();
	void Shoot();

	UShooter();
	~UShooter() override;
};
