#pragma once

// TODO Singleton 패턴화

struct FScoreEntry
{
	int Score;
	string PlayerName;

	// 기본 생성자 추가
	FScoreEntry() : Score(0), PlayerName("") {}
	
	FScoreEntry(int InScore, const string& InName) : Score(InScore), PlayerName(InName) {}
};

class FScoreManager
{
private:
	static FScoreManager* Instance;
	const float TimeBuffer = 0.1f;
	const int MaxLeaderboardEntries = 10;
	
	// 점수 시스템 상수들
	const int TIME_SCORE_PER_SECOND = 100;  // 1초마다 100점
	const int COLLISION_SCORE = 10;         // 충돌 시 10점

	static int CurrentScore;
	float TimeSinceLastScore;
	float TimeSinceLastTimeScore;          // 시간 기반 점수용 타이머
	static vector<FScoreEntry> Leaderboard;

	FTimeManager* TimeManager = FTimeManager::GetInstance();

	void Initialize();
	void LoadLeaderboard();
	void SaveLeaderboard();
	void ProcessTimeBasedScore();           // 시간 기반 점수 처리

public:
	FScoreManager();
	int GetCurrentScore();
	void Update();
	void AddScore(int InScore);
	void AddCollisionScore();               // 충돌 시 점수 추가 (10점)
	void SubmitScore(const string& playerName);
	void ResetCurrentScore();
	const vector<FScoreEntry>& GetLeaderboard() const;

	static FScoreManager* GetInstance();
	~FScoreManager();
};
