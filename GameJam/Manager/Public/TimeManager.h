#pragma once

// FIXME(KHJ): Singleton 패턴화해서 구현되면 적용할 것
class FTimeManager
{
private:
	static FTimeManager* Instance;

private:
	FTimeManager();

public:
	float GetFPS();
	float GetDeltaTime();

	static FTimeManager* GetInstance();
	~FTimeManager();
};
