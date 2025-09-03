#include "Scene.h"
#include "Mesh/Public/URectangle.h"

class UPinBall;

class GameScene : public Scene
{
public:
	GameScene();
	virtual ~GameScene();

	// IScene 인터페이스 함수들
	virtual void Init() override;
	virtual void Update(float deltaTime) override;
	virtual void Render() override; // Renderer를 파라미터로 받도록 수정
	virtual void Cleanup() override;

private:
	void HandleCollisions();
	void HandleBallRectangleCollisions();
	void ResolveBallRectangle(UPinBall* Ball, const URectangle* Rect);

private:
	std::vector<UPinBall*> PinBalls;
	URectangle GRectangle; // 사각형 객체
	vector<UPrimitive*> ScenePrimitives;
};
