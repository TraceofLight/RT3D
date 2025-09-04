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
	TimeSinceLastTimeScore = 0.0f;  // 시간 기반 점수 타이머 초기화
	LoadLeaderboard();
}

void FScoreManager::Update()
{
	TimeSinceLastScore += TimeManager->GetDeltaTime();
	ProcessTimeBasedScore();  // 시간 기반 점수 처리
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

void FScoreManager::SubmitScore(const string& InPlayerName)
{
	Leaderboard.emplace_back(CurrentScore, InPlayerName);

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

// 시간 기반 점수 처리 (1초마다 100점)
void FScoreManager::ProcessTimeBasedScore()
{
	TimeSinceLastTimeScore += TimeManager->GetDeltaTime();

	// 1초가 지날 때마다 100점 추가
	if (TimeSinceLastTimeScore >= 1.0f)
	{
		CurrentScore += TIME_SCORE_PER_SECOND;
		TimeSinceLastTimeScore -= 1.0f;  // 1초 차감 (누적 오차 방지)

		DEBUG_PRINT("[ScoreManager] Time bonus: +%d points (Total: %d)\n",
					TIME_SCORE_PER_SECOND, CurrentScore);
	}
}

// 충돌 시 점수 추가 (10점)
void FScoreManager::AddCollisionScore()
{
	// 충돌 점수는 TimeBuffer를 무시하고 즉시 추가
	CurrentScore += COLLISION_SCORE;

	DEBUG_PRINT("[ScoreManager] Collision bonus: +%d points (Total: %d)\n",
				COLLISION_SCORE, CurrentScore);
}
