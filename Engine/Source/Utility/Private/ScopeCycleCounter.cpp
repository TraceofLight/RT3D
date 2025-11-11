#include "pch.h"
#include "Utility/Public/ScopeCycleCounter.h"

TMap<FString, FTimeProfile> TimeProfileMap;
//Map에 이미 있으면 시간, 콜스택 추가
void FScopeCycleCounter::AddTimeProfile(const TStatId& Key, double InMilliseconds)
{
	if (TimeProfileMap.Contains(Key.Key) == false)
	{
		TimeProfileMap[Key.Key] = FTimeProfile{ InMilliseconds, 1 };
	}
	else
	{
		TimeProfileMap[Key.Key].Milliseconds += InMilliseconds;
		TimeProfileMap[Key.Key].CallCount++;
	}
}
//시간, 콜스택 초기화
void FScopeCycleCounter::TimeProfileInit()
{
	for (const auto& Pair : TimeProfileMap)
	{
		TimeProfileMap[Pair.first].Milliseconds = 0;
		TimeProfileMap[Pair.first].CallCount = 0;
	}
}
//const TMap<FString, FTimeProfile>& FScopeCycleCounter::GetTimeProfiles()
//{
//    return TimeProfileMap;
//}
const TArray<FString> FScopeCycleCounter::GetTimeProfileKeys()
{
	TArray<FString> Keys;
	Keys.Reserve(TimeProfileMap.Num());
	for (const auto& Pair : TimeProfileMap)
	{
		Keys.Add(Pair.first);
	}
	return Keys;
}

const TArray<FTimeProfile> FScopeCycleCounter::GetTimeProfileValues()
{
	TArray<FTimeProfile> Values;
	Values.Reserve(TimeProfileMap.Num());
	for (const auto& Pair : TimeProfileMap)
	{
		Values.Add(Pair.second);
	}
	return Values;
}

const FTimeProfile& FScopeCycleCounter::GetTimeProfile(const FString& Key)
{
	return TimeProfileMap[Key];
}
double FWindowsPlatformTime::GSecondsPerCycle = 0.0;
bool FWindowsPlatformTime::bInitialized = false;
