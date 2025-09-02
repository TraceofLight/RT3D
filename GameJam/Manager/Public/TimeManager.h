#pragma once

using std::chrono::high_resolution_clock;

constexpr int FPS_SAMPLE_COUNT = 60;

// FIXME(KHJ): Singleton 패턴화해서 구현되면 적용할 것
class FTimeManager
{
private:
	static FTimeManager* Instance;

	high_resolution_clock::time_point PrevTime;
	high_resolution_clock::time_point CurrentTime;
	float GameTime;
	float DeltaTime;

	float FPS;
	int FrameCount;
	float FrameSpeedSamples[FPS_SAMPLE_COUNT];
	int FrameSpeedSampleIndex;

	bool bIsPaused;

private:
	FTimeManager();
	void Initialize();
	void CalculateFPS();

public:
	void Update();

	// Getter & Setter
	float GetFPS() const { return FPS; }
	float GetDeltaTime() const { return DeltaTime; }
	float GetGameTime() const { return GameTime; }
	bool IsPaused() const { return bIsPaused; }

	void PauseGame() { bIsPaused = true; }
	void ResumeGame() { bIsPaused = false; }

	static FTimeManager* GetInstance();
	~FTimeManager();
};
