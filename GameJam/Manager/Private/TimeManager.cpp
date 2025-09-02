#include "pch.h"
#include "Manager/Public/TimeManager.h"

FTimeManager* FTimeManager::Instance = nullptr;

FTimeManager::FTimeManager() = default;

FTimeManager::~FTimeManager()
{
	Instance = nullptr;
}

FTimeManager* FTimeManager::GetInstance()
{
	if (!Instance)
	{
		Instance = new FTimeManager();
	}

	return Instance;
}

float FTimeManager::GetDeltaTime()
{
}
