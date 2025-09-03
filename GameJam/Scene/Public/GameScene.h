#pragma once
#include "Scene/Public/Scene.h"

class UShooter;
class UPinBall;
class URectangle;
class UTriangle;
class UPrimitive;

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

	void HandleBallRectangleCollisions();
	void ResolveBallRectangle(UPinBall* Ball, const URectangle* Rect);
	void HandleBallTriangleCollisions();
	void ResolveBallTriangle(UPinBall* Ball, const UTriangle* Triangle);

private:
	int m_TotalPrimitives = 0;
	vector<UPrimitive*> m_PrimitiveList;
	URectangle* m_Rectangle = nullptr;
	UTriangle* m_Triangle = nullptr;
	UPinBall* m_GravityCenterBall = nullptr;

	UShooter* Shooter;
};
