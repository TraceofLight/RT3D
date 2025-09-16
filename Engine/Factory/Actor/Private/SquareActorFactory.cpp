#include "pch.h"
#include "Factory/Actor/Public/SquareActorFactory.h"
#include "Mesh/Public/SquareActor.h"

IMPLEMENT_CLASS(USquareActorFactory, UActorFactory)

/**
 * @brief Factory Constructor
 * Factory 등록도 함께 수행
 */
USquareActorFactory::USquareActorFactory()
{
	SupportedClass = ASquareActor::StaticClass();
	Description = "SquareActor Factory";

	RegisterFactory(TObjectPtr<UFactory>(this));
}

/**
 * @brief SquareActor 인스턴스를 생성합니다
 * @return 생성된 SquareActor
 */
AActor* USquareActorFactory::CreateNewActor()
{
	UE_LOG_SUCCESS("SquareActorFactory: Creating new SquareActor instance");
	return new ASquareActor();
}
