
class FPreviewScene
{
public:
	AActor* SkeletalActor = nullptr;
	AActor* GroundActor   = nullptr;
	ADirectionalLight* Sun = nullptr;

	// 씬에 필요한 리스트(스태틱, 스켈레탈, 라이트)를 쿨하게 보유
	TArray<UStaticMeshComponent*>   StaticComps;
	TArray<USkeletalMeshComponent*> SkelComps;
	TArray<ULightComponent*>        Lights;

	FPreviewScene() {}
	~FPreviewScene() { /* 컴포넌트/액터 정리 */ }

	// 렌더러에서 가져갈 때 쓸 구조체들 리턴
};
