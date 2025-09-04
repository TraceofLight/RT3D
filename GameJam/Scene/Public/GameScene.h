#pragma once
#include "Scene/Public/Scene.h"

class UShooter;
class UPinBall;
class URectangle;
class UTriangle;
class UPrimitive;
class UPadPair;

class GameScene : public Scene
{
public:
	GameScene();
	~GameScene() override;

	void Init() override;
	void Update(float deltaTime) override;
	void Render() override;
	void Cleanup() override;

private:
	void InputProcess();
	void RenderProcess();

	void ResolveBallRectangle(UPinBall* Ball, const URectangle* Rect);
	void ResolveBallTriangle(UPinBall* Ball, const UTriangle* Triangle);
	void HandleBallPadPairCollisions();
	void HandleBallRectangleCollisions();
	bool isGameOver();
	void CheckBallTriggers();

	void AddNewBall();
	void AddNewRectangle(FVector3 location, float rotation, float width, float height);
	void AddNewTriangle(FVector3 location, float rotation, float base, float height);

private:
	int m_TotalPrimitives = 0;
	vector<UPrimitive*>* m_PrimitiveList;
	URectangle* m_Rectangle = nullptr;
	UPinBall* m_GravityCenterBall = nullptr;

	// Delayed Deletion System
	vector<UPinBall*> m_BallsToDelete;
	void ProcessDelayedDeletions();
	bool IsMarkedForDeletion(UPinBall* InBall) const;

	UPadPair* m_PadPair = nullptr;
	UPinBall* m_ActorBall = nullptr;
	bool m_Shooted = false;
	UShooter* Shooter;

	bool bIsBallSpawned = false;
};
