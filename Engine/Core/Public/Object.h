#pragma once
#include "Class.h"

UCLASS()
class UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UObject, UObject)

public:
	// Special Member Function
	UObject();
	explicit UObject(const FString& InString);
	virtual ~UObject() = default;

	// Getter & Setter
	const FString& GetName() const { return Name; }
	const UObject* GetOuter() const { return Outer; }

	void SetName(const FString& InName) { Name = InName; }
	void SetOuter(UObject* InObject);

	// InBytes가 디폴트 인자인 0으로 들어가면, RTTI로 실제타입의 사이즈를 InBytes 대신 씀.
	void AddMemoryUsage(uint64 InBytes = 0, uint32 InCount = 1);
	void RemoveMemoryUsage(uint64 InBytes, uint32 InCount = 1);

	uint64 GetAllocatedBytes() const { return AllocatedBytes; }
	uint32 GetAllocatedCount() const { return AllocatedCounts; }

	bool IsA(const UClass* InClass) const;

private:
	uint32 UUID = -1;
	uint32 InternalIndex = -1;
	FString Name;
	UObject* Outer;

	uint64 AllocatedBytes = 0;
	uint32 AllocatedCounts = 0;
};

extern TArray<UObject*> GUObjectArray;
