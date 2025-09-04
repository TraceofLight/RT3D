#pragma once

// TODO Singleton 패턴화

struct FScoreEntry
{
	int Score;
	string PlayerName;

	// 기본 생성자 추가
	FScoreEntry() : Score(0), PlayerName("")
	{
	}

	FScoreEntry(int InScore, const string& InName) : Score(InScore), PlayerName(InName)
	{
	}
};

class FScoreManager
{
private:
	static FScoreManager* Instance;
	const float TimeBuffer = 0.1f;
	const int MaxLeaderboardEntries = 10;

	static int CurrentScore;
	float TimeSinceLastScore;
	float TimeSinceLastTimeScore;
	static vector<FScoreEntry> Leaderboard;

	FTimeManager* TimeManager = FTimeManager::GetInstance();

	void Initialize();
	void LoadLeaderboard();
	void SaveLeaderboard();
	void ProcessTimeBasedScore();

public:
	FScoreManager();
	int GetCurrentScore();
	void Update();
	void AddScore(int InScore);
	void AddCollisionScore();
	void SubmitScore(const string& InPlayerName);
	void ResetCurrentScore();
	const vector<FScoreEntry>& GetLeaderboard() const;

	static FScoreManager* GetInstance();
	~FScoreManager();
};
