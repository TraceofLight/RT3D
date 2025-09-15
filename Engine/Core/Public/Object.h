#pragma once
#include "Class.h"
#include "Name.h"

UCLASS()
class UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UObject, UObject)

public:
	bool IsA(const UClass* InClass) const;
	void AddMemoryUsage(uint64 InBytes, uint32 InCount);
	void RemoveMemoryUsage(uint64 InBytes, uint32 InCount);

	// Getter & Setter
	const FName& GetName() const { return Name; }
	const UObject* GetOuter() const { return Outer; }

	uint64 GetAllocatedBytes() const { return AllocatedBytes; }
	uint32 GetAllocatedCount() const { return AllocatedCounts; }

	void SetName(const FName& InName) { Name = InName; }
	void SetOuter(UObject* InObject);

	// Special Member Function
	UObject();
	explicit UObject(const FName& InName);
	virtual ~UObject() = default;

private:
	uint32 UUID;
	uint32 InternalIndex;
	FName Name;
	UObject* Outer;

	uint64 AllocatedBytes = 0;
	uint32 AllocatedCounts = 0;

	void PropagateMemoryChange(uint64 InBytesDelta, uint32 InCountDelta);
};

extern TArray<UObject*> GUObjectArray;
