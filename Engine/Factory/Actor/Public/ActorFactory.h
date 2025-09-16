#pragma once
#include "Factory/Public/Factory.h"

class AActor;

/**
 * @brief Actor Factory
 */
UCLASS()
class UActorFactory :
	public UFactory
{
	GENERATED_BODY()
	DECLARE_CLASS(UActorFactory, UFactory)

public:
	virtual AActor* CreateActor(UObject* InWorld, class ULevel* InLevel,
	                            const FTransform& InTransform = FTransform(), uint32 InObjectFlags = 0);

	bool IsActorFactory() const override { return true; }

	// Special member function
	UActorFactory();
	~UActorFactory() override = default;

protected:
	// Inheritable function
	virtual AActor* CreateNewActor() { return nullptr; }
	UObject* CreateNew() override;

	virtual void PostCreateActor(AActor* InActor, const FTransform& InTransform);
};
