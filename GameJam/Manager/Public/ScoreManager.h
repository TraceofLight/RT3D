#pragma once

// TODO Singleton 패턴화

class FScoreManager
{
private:
	static FScoreManager* Instance;
	const float TimeBuffer = 0.1f;

	static int CurrentScore;
	float TimeSinceLastScore;

	FTimeManager* TimeManager = FTimeManager::GetInstance();

	void Initialize();

public:
	FScoreManager();
	int GetCurrentScore();
	void Update();
	void AddScore(int InScore);

	static FScoreManager* GetInstance();
	~FScoreManager();
};
