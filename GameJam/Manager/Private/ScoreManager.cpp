#include "pch.h"
#include "Manager/Public/ScoreManager.h"

FScoreManager* FScoreManager::Instance = nullptr;
int FScoreManager::CurrentScore = 0;

FScoreManager::FScoreManager()
{
	Initialize();
}

FScoreManager::~FScoreManager()
{
	Instance = nullptr;
}

FScoreManager* FScoreManager::GetInstance()
{
	if (!Instance)
	{
		Instance = new FScoreManager();
	}
	return Instance;
}

void FScoreManager::Initialize()
{
	TimeSinceLastScore = 0.0f;
}

void FScoreManager::Update()
{
	TimeSinceLastScore += TimeManager->GetDeltaTime();
}

void FScoreManager::AddScore(int InScore)
{
	if (TimeSinceLastScore < TimeBuffer) return;
	CurrentScore += InScore;
	TimeSinceLastScore = 0.0f;
}

int FScoreManager::GetCurrentScore()
{
	return CurrentScore;
}
