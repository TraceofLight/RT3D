#pragma once
#include "Mesh/Public/URectangle.h"
#include "Actor/Public/PinBall.h"
class UShooter :
	public UPrimitive
{
private:
	URectangle* Shape;
	float ChargingTime;
	FVector3 Location;
	bool bHasBallLoaded;
	class UPinBall* CurrentBall;

public:
	void Init();
	void Destroy();
	void Charging();
	UPinBall* Shoot();
	void LoadBall();
	void OnBallTrigger();

	// Getter & Setter
	bool CanShoot() const { return bHasBallLoaded; }
	FVector3 GetLocation() const { return Location; }
	URectangle* GetShape() const { return Shape; }

	void SetLocation(const FVector3& InLocation) { Location = InLocation; }

	// Special Member Function
	UShooter();
	~UShooter() override;
};
