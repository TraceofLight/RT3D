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

	// void HandleBallRectangleCollisions();
	void ResolveBallRectangle(UPinBall* Ball, const URectangle* Rect);
	void ResolveBallTriangle(UPinBall* Ball, const UTriangle* Triangle);
	void HandleBallPadPairCollisions();
	bool IsGameOver();
	void CheckBallTriggers();

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
	UShooter* Shooter;
};
