#include "pch.h"
#include "Factory/Actor/Public/CubeActorFactory.h"
#include "Mesh/Public/CubeActor.h"

IMPLEMENT_CLASS(UCubeActorFactory, UActorFactory)

/**
 * @brief Factory Constructor
 * Factory 등록도 함께 수행
 */
UCubeActorFactory::UCubeActorFactory()
{
	SupportedClass = ACubeActor::StaticClass();
	Description = "CubeActor Factory";

	RegisterFactory(this);
}

/**
 * @brief CubeActor 인스턴스를 생성합니다
 * @return 생성된 CubeActor
 */
AActor* UCubeActorFactory::CreateNewActor()
{
	UE_LOG_SUCCESS("CubeActorFactory: Creating new CubeActor instance");
	return new ACubeActor();
}
