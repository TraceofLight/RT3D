#pragma once
#include "Scene/Public/Scene.h"

class FScoreManager;
class UShooter;
class UPinBall;
class URectangle;
class UTriangle;
class UConcaveCircle;
class UBall;
class UPrimitive;
class UFlipperPair;

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
	void CollisionProcess();
	void GenerateObstacles();

	void ResolveRectangleCollsion(UPinBall* Ball, const URectangle* Rect);
	void ResolveTriangleCollision(UPinBall* Ball, const UTriangle* Triangle);
	void ResolvePadCollision(UPinBall* InBall, const UTriangle* InFilpper, bool bInIsLeftPad);
	void ResolveBallConcaveCircle(UPinBall* Ball, const UConcaveCircle* ConcaveCircle);
	void ResolveBallConvexCircle(UPinBall* Ball, const UBall* ConvexCircle);
	void HandlePadPairCollisions();
	void HandleRectangleCollisions();
	void HandleTriangleCollisions();
	void HandleConcaveCircleCollisions();
	void HandleConvexCircleCollisions();
	bool isGameOver();
	void CheckBallTriggers();

	void AddNewRectangle(FVector3 location, float rotation, float width, float height, bool autoRotation = false,
	                     RectRotType type = RectRotType::RAND);

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

	UFlipperPair* m_PadPair = nullptr;
	UPinBall* m_ActorBall = nullptr;
	bool m_Shooted = false;
	UShooter* Shooter;

	// Score Manager
	FScoreManager* m_ScoreManager = nullptr;

	// Concave Circle Colliders
	// UConcaveCircle* m_MainBoundary = nullptr; // 메인 경계벽
	// UConcaveCircle* m_LeftFlipperGuide = nullptr; // 좌측 플리퍼 가이드
	// UConcaveCircle* m_RightFlipperGuide = nullptr; // 우측 플리퍼 가이드
	// UConcaveCircle* m_LeftSideGuide = nullptr; // 좌측 사이드 가이드
	// UConcaveCircle* m_RightSideGuide = nullptr; // 우측 사이드 가이드

	// Convex & Concave Colliders
	UConcaveCircle* m_TopBumperWall = nullptr;
	UBall* m_CenterBumperBall = nullptr;
	UBall* m_TopBumperBall1 = nullptr;
	UBall* m_TopBumperBall2 = nullptr;
	UBall* m_TopBumperBall3 = nullptr;
};
