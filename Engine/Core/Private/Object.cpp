#include "pch.h"
#include "Core/Public/Object.h"
#include "Core/Public/EngineStatics.h"
#include "Core/Public/Name.h"

uint32 UEngineStatics::NextUUID = 0;
TArray<UObject*> GUObjectArray;

IMPLEMENT_CLASS_BASE(UObject)

UObject::UObject()
	: Name(FName::None), Outer(nullptr)
{
	UUID = UEngineStatics::GenUUID();
	Name = FName("Object_" + to_string(UUID));

	GUObjectArray.push_back(this);
	InternalIndex = static_cast<uint32>(GUObjectArray.size()) - 1;
}

UObject::UObject(const FString& InString)
	: Name(FName(InString))
	  , Outer(nullptr)
{
	UUID = UEngineStatics::GenUUID();

	GUObjectArray.push_back(this);
	InternalIndex = static_cast<uint32>(GUObjectArray.size()) - 1;
}

void UObject::SetOuter(UObject* InObject)
{
	if (Outer == InObject)
	{
		return;
	}

	// 기존 Outer가 있다면 해당 오브젝트에서 메모리 관리 제거
	if (Outer)
	{
		Outer->RemoveMemoryUsage(AllocatedBytes, AllocatedCounts);
	}

	// 새로운 Outer 설정 후 새로운 Outer에서 메모리 관리
	Outer = InObject;
	if (Outer)
	{
		Outer->AddMemoryUsage(AllocatedBytes, AllocatedCounts);
	}
}

void UObject::AddMemoryUsage(uint64 InBytes, uint32 InCount)
{
	// 최상위에서 InBytes를 디폴트인자로 호출했다면
	if (InBytes == 0)
	{
		AllocatedBytes += GetClass()->GetClassSize();
	}
	AllocatedBytes += InBytes;
	AllocatedCounts += InCount;

	if (Outer)
	{
		Outer->AddMemoryUsage(InBytes);
	}
}

void UObject::RemoveMemoryUsage(uint64 InBytes, uint32 InCount)
{
	if (AllocatedBytes >= InBytes)
	{
		AllocatedBytes -= InBytes;
	}
	if (AllocatedCounts >= InCount)
	{
		AllocatedCounts -= InCount;
	}

	if (Outer)
	{
		Outer->RemoveMemoryUsage(InBytes);
	}
}

/**
 * @brief 해당 클래스가 현재 내 클래스의 조상 클래스인지 판단하는 함수
 * 내부적으로 재귀를 활용해서 부모를 계속 탐색한 뒤 결과를 반환한다
 * @param InClass 판정할 Class
 * @return 판정 결과
 */
bool UObject::IsA(const UClass* InClass) const
{
	if (!InClass)
	{
		return false;
	}

	return GetClass()->IsChildOf(InClass);
}
