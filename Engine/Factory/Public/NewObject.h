#pragma once
#include "Factory.h"
#include "Factory/Actor/Public/ActorFactory.h"
#include "Core/Public/ObjectPtr.h"

using std::is_base_of_v;

/**
 * @brief 클래스를 지정하여 새 객체를 생성하는 함수
 * @tparam T 생성할 객체의 타입
 * @param InOuter 생성될 객체의 부모
 * @param InClass 생성할 클래스 (nullptr이면 T::StaticClass() 사용)
 * @param InName 객체 이름
 * @param InFlags 객체 플래그
 * @return 생성된 객체
 */
template <typename T>
T* NewObject(UObject* InOuter = nullptr, UClass* InClass = nullptr,
             const FName& InName = FName::None, uint32 InFlags = 0)
{
	static_assert(is_base_of_v<UObject, T>, "생성할 클래스는 UObject를 반드시 상속 받아야 합니다");

	UClass* ClassToUse = InClass ? InClass : T::StaticClass();

	// Factory를 사용하여 생성 시도
	TObjectPtr<UFactory> FactoryPtr = UFactory::FindFactory(TObjectPtr<UClass>(ClassToUse));
	UFactory* Factory = FactoryPtr.Get();
	if (Factory)
	{
		UObject* NewObject = Factory->FactoryCreateNew(ClassToUse, InOuter, InName, InFlags);
		if (NewObject)
		{
			return static_cast<T*>(NewObject);
		}
	}

	// Factory가 없으면 기존 방식으로 폴백
	UE_LOG_WARNING("NewObject: %s를 생성할 Factory를 찾지 못해, new를 통한 폴백 생성으로 처리합니다",
	               ClassToUse->GetClass().ToString().data());

	T* NewObject = new T();
	if (NewObject)
	{
		if (InName != FName::None)
		{
			NewObject->SetName(InName);
		}
		if (InOuter)
		{
			NewObject->SetOuter(InOuter);
		}
	}

	return NewObject;
}

/**
 * @brief Actor 전용 생성 함수
 * @tparam T 생성할 Actor 타입
 * @param InLevel Actor가 생성될 레벨
 * @param InTransform 초기 변환
 * @param InName Actor 이름
 * @return 생성된 Actor
 */
template <typename T>
T* SpawnActor(ULevel* InLevel, const FTransform& InTransform = FTransform(), const FName& InName = FName::None)
{
	static_assert(is_base_of_v<AActor, T>, "생성할 클래스는 AActor를 반드시 상속 받아야 합니다");

	// ActorFactory를 사용하여 생성 시도
	TObjectPtr<UFactory> FactoryPtr = UFactory::FindFactory(TObjectPtr<UClass>(T::StaticClass()));
	UFactory* Factory = FactoryPtr.Get();
	if (Factory && Factory->IsActorFactory())
	{
		UActorFactory* ActorFactory = static_cast<UActorFactory*>(Factory);
		AActor* NewActor = ActorFactory->CreateActor(nullptr, InLevel, InTransform);
		if (NewActor)
		{
			return static_cast<T*>(NewActor);
		}
	}

	// Factory가 없으면 기존 방식으로 폴백
	UE_LOG_WARNING("NewObject: %s를 생성할 Factory를 찾지 못해, new를 통한 폴백 생성으로 처리합니다",
	       T::StaticClass()->GetClass().ToString().data());

	return NewObject<T>(InLevel, nullptr, InName);
}
