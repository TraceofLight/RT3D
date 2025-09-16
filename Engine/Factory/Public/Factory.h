#pragma once
#include "Core/Public/Object.h"

/**
 * @brief Base Factory Class
 * Editor, Runtime 에서 객체의 생성을 담당한다
 */
UCLASS()
class UFactory :
	public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UFactory, UObject)

public:
	// 생성 함수
	virtual UObject* CreateNew() { return nullptr; }
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, const FName& InName,
	                                  uint32 InFlags = 0, UObject* InContext = nullptr, void* InWarning = nullptr);

	// Factory가 지원하는 클래스인지 확인하기 위한 함수
	virtual UClass* GetSupportedClass() const;
	virtual bool DoesSupportClass(UClass* InClass);

	// 생성을 처리하는 과정에서 해당 객체를 생성해 줄 Factory를 찾기 위한 함수
	static TArray<UFactory*>& GetFactoryList();
	static void RegisterFactory(UFactory* InFactory);
	static UFactory* FindFactory(UClass* InClass);

	// Getter
	const FString& GetDescription() const { return Description; }
	virtual bool IsActorFactory() const { return false; }

	// Special member function
	UFactory();
	~UFactory() override = default;

protected:
	UClass* SupportedClass;
	FString Description;
};
