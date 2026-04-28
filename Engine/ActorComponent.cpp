#include "pch.h"
#include "ActorComponent.h"

UActorComponent::UActorComponent()
    : Owner(nullptr), bIsActive(true), bCanEverTick(false)
{
}

UActorComponent::~UActorComponent()
{
}

void UActorComponent::InitializeComponent()
{
    // 액터에 부착될 때 초기화
    // 필요하다면 Override
}

void UActorComponent::BeginPlay()
{
    // 게임 시작 시
    // 필요하다면 Override
}

void UActorComponent::TickComponent(float DeltaSeconds)
{
    if (!bIsActive || !bCanEverTick)
        return;

    // 매 프레임 처리
    // 자식 클래스에서 Override
}

void UActorComponent::EndPlay(EEndPlayReason::Type EndPlayReason)
{
    if (EndPlayReason == EEndPlayReason::EndPlayInEditor)
    {
        // End Replication
    }
}

UObject* UActorComponent::Duplicate()
{
    UActorComponent* DuplicatedComponent = NewObject<UActorComponent>(*this);
    DuplicatedComponent->DuplicateSubObjects();

    return DuplicatedComponent;
}

void UActorComponent::DuplicateSubObjects()
{
    Super_t::DuplicateSubObjects();


}
