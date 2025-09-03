#include "Scene.h"
#include "Mesh/Public/URectangle.h"
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

	// 마우스 입력을 처리할 함수
	//void HandleMouseClick(int InX, int InY, bool InIsLeftClick);

private:
	// main.cpp의 static 함수들을 멤버 함수로 가져옵니다.
	void AddNewBall();
	void RemoveRandomBall();
	//void RemoveSpecificBall(int IndexToRemove);
	//void SetGravityCenter(int IndexToSet);

	void HandleCollisions();
	void HandleBallRectangleCollisions();
	void ResolveBallRectangle(UBall* Ball, const URectangle* Rect);

private:
	// main.cpp의 전역 변수들을 멤버 변수로 가져옵니다.
	int TotalPrimitives = 0;
	UPrimitive** PrimitiveList = nullptr;
	URectangle GRectangle; // 사각형 객체
	UBall* GravityCenterBall = nullptr; // 중력 중심 공
};
