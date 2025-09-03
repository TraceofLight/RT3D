#pragma once
#include "Scene/Public/Scene.h"

class UPinBall;
class URectangle;
class UTriangle;
class UPrimitive;

class GameScene : public Scene
{
public:
	GameScene();
	virtual ~GameScene() override;

	virtual void Init() override;
	virtual void Update(float deltaTime) override;
	virtual void Render() override;
	virtual void Cleanup() override;

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
};
