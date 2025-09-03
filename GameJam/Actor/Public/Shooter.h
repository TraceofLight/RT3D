#pragma once
#include "Mesh/Public/URectangle.h"

class UShooter :
	public UPrimitive
{
private:
	URectangle* Shape;
	float ChargingTime;
	FVector3 Location;

public:
	void Init();
	void Destroy();
	void Charging();
	void Shoot();

	FVector3 GetLocation() const { return Location; }
	void SetLocation(const FVector3& InLocation) { Location = InLocation; }

	UShooter();
	~UShooter() override;
};
