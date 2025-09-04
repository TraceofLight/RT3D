#include "pch.h"
#include "Manager/Public/ScoreManager.h"

FScoreManager* FScoreManager::Instance = nullptr;
int FScoreManager::CurrentScore = 0;
vector<FScoreEntry> FScoreManager::Leaderboard;

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
	LoadLeaderboard();
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

void FScoreManager::ResetCurrentScore()
{
	CurrentScore = 0;
}

void FScoreManager::SubmitScore(const string& playerName)
{
	Leaderboard.emplace_back(CurrentScore, playerName);
	
	// 점수 기준 내림차순 정렬
	sort(Leaderboard.begin(), Leaderboard.end(), [](const FScoreEntry& a, const FScoreEntry& b) {
		return a.Score > b.Score;
	});
	
	// 상위 10개만 유지
	if (Leaderboard.size() > MaxLeaderboardEntries)
	{
		Leaderboard.resize(MaxLeaderboardEntries);
	}
	
	SaveLeaderboard();
}

const vector<FScoreEntry>& FScoreManager::GetLeaderboard() const
{
	return Leaderboard;
}

void FScoreManager::LoadLeaderboard()
{
	// 실제 파일에서 로드하는 대신 빈 리더보드로 시작
	// Leaderboard를 비워둡니다
}

void FScoreManager::SaveLeaderboard()
{
	// TODO: 실제 파일 저장 구현
	// 현재는 메모리에만 보관
}
