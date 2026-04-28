#include "pch.h"
#include "AABoundingBoxComponent.h"
#include "StaticMeshActor.h"
#include "ObjectFactory.h"

AStaticMeshActor::AStaticMeshActor()
{
    // TODO(KJC): AActor의 생성자에서 USceneComponent를 Root로 넣어주고 있지만
    // 현재 StaticMeshActor 에서는 Root가 StaticMeshComponent가 되어야 하므로 기존 Root를 제거 후 새로 할당함
    // 필요에 따라서, Root를 Default로 생성하지 않고 인자로 처리하는 함수를 제공하면 좋음
    // 현재는 생성 직후 소멸로 이어지기 때문에 GUObectArray를 계속 counting 처리하는 방향이라 좋지 않음
    if (RootComponent)
    {
        USceneComponent* TempRootComponent = RootComponent;
        RootComponent = nullptr;
        DeleteComponent(TempRootComponent);
    }

    Name = "Static Mesh Actor";
    StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("StaticMesh");
    RootComponent = StaticMeshComponent;
    AddComponent(StaticMeshComponent);

    CollisionComponent = CreateDefaultSubobject<UAABoundingBoxComponent>(FName("CollisionBox"));
	CollisionComponent->SetupAttachment(RootComponent);
}

void AStaticMeshActor::Tick(float DeltaTime)
{
    if (World->WorldType == EWorldType::PIE) {
        RootComponent->AddLocalOffset({ 0.1f, 0.1f,0.1f });
    }
    if(bIsPicked&& CollisionComponent)
    CollisionComponent->SetFromVertices(StaticMeshComponent->GetStaticMesh()->GetStaticMeshAsset()->Vertices);
}

AStaticMeshActor::~AStaticMeshActor()
{
    if (StaticMeshComponent)
    {
        ObjectFactory::DeleteObject(StaticMeshComponent);
    }
    StaticMeshComponent = nullptr;
}

void AStaticMeshActor::SetStaticMeshComponent(UStaticMeshComponent* InStaticMeshComponent)
{
    StaticMeshComponent = InStaticMeshComponent;
}

void AStaticMeshActor::SetCollisionComponent(EPrimitiveType InType)
{
    if (!CollisionComponent) {
        return;
    }
    CollisionComponent->SetFromVertices(StaticMeshComponent->GetStaticMesh()->GetStaticMeshAsset()->Vertices);
    CollisionComponent->SetPrimitiveType(InType);
}

UObject* AStaticMeshActor::Duplicate()
{
    AStaticMeshActor* DuplicatedActor = NewObject<AStaticMeshActor>(*this);

    // 생성자가 빈 컴포넌트를 만들었으므로, 원본의 속성을 복사
    if (this->StaticMeshComponent && DuplicatedActor->StaticMeshComponent)
    {
        UStaticMeshComponent* SrcComp = this->StaticMeshComponent;
        UStaticMeshComponent* DstComp = DuplicatedActor->StaticMeshComponent;

        // Transform 복사
        DstComp->SetRelativeLocation(SrcComp->GetRelativeLocation());
        DstComp->SetRelativeRotation(SrcComp->GetRelativeRotation());
        DstComp->SetRelativeScale(SrcComp->GetRelativeScale());

        // StaticMesh 복사 (SetStaticMesh는 내부적으로 Material도 설정)
        if (SrcComp->GetStaticMesh())
        {
            DstComp->SetStaticMesh(SrcComp->GetStaticMesh()->GetAssetPathFileName());
        }
    }

    DuplicatedActor->DuplicateSubObjects();

    return DuplicatedActor;
}

void AStaticMeshActor::DuplicateSubObjects()
{
    Super_t::DuplicateSubObjects();
    if (!RootComponent)
    {
        UE_LOG("AStaticMeshActor: DuplicateSubObjects 실패\n");
        return;
    }

    if (UStaticMeshComponent* Component = Cast<UStaticMeshComponent>(RootComponent))
    {
        StaticMeshComponent = Component;
    }
}

// 특화된 멤버 컴포넌트 CollisionComponent, StaticMeshComponent 는 삭제 시 포인터를 초기화합니다.
bool AStaticMeshActor::DeleteComponent(USceneComponent* ComponentToDelete)
{
    // 1. [자식 클래스의 추가 처리] 삭제 대상이 나의 특정 컴포넌트인지 확인합니다.
    if (ComponentToDelete == CollisionComponent)
    {
        // 맞다면, 나의 멤버 포인터를 nullptr로 설정합니다.
        CollisionComponent = nullptr;
    }
    else if (ComponentToDelete == StaticMeshComponent)
    {
        // AStaticMeshActor는 StaticMeshComponent가 Root 이기 때문에 삭제할 수 없음
        UE_LOG("루트 컴포넌트는 직접 삭제할 수 없습니다.");
        return false;
    }

    // 2. [부모 클래스의 원래 기능 호출]
    // 기본적인 삭제 로직(소유 목록 제거, 메모리 해제 등)은 부모에게 위임합니다.
    // Super:: 키워드를 사용합니다.
    return AActor::DeleteComponent(ComponentToDelete);
}
