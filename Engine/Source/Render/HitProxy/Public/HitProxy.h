#pragma once

class UPrimitiveComponent;
class USkeletalMeshComponent;

// 기즈모 축 타입
enum class EGizmoAxisType : uint8
{
	None = 0,
	X = 1,
	Y = 2,
	Z = 3,
	Center = 4,
	XY = 5, // XY 평면
	XZ = 6, // XZ 평면
	YZ = 7 // YZ 평면
};

// HitProxy ID
struct FHitProxyId
{
	// RGB를 uint32로 변환 (R << 16 | G << 8 | B)
	uint32 Index;

	FHitProxyId();
	explicit FHitProxyId(uint32 InIndex);
	FHitProxyId(uint8 Red, uint8 Green, uint8 Blue);

	bool IsValid() const;
	FVector4 GetColor() const;

	bool operator==(const FHitProxyId& Other) const;
	bool operator!=(const FHitProxyId& Other) const;
};

// Invalid HitProxy ID
static const FHitProxyId InvalidHitProxyId = FHitProxyId(0);

// HitProxy 기본 클래스
class HHitProxy
{
public:
	FHitProxyId Id;

	HHitProxy(FHitProxyId InId);
	virtual ~HHitProxy();

	virtual bool IsWidgetAxis() const;
	virtual bool IsComponent() const;
	virtual bool IsBone() const;
};

// 기즈모 축 HitProxy
class HWidgetAxis : public HHitProxy
{
public:
	EGizmoAxisType Axis;

	HWidgetAxis(EGizmoAxisType InAxis, FHitProxyId InId);
	bool IsWidgetAxis() const override;
};

// 컴포넌트 HitProxy
class HComponent : public HHitProxy
{
public:
	UPrimitiveComponent* Component;

	HComponent(UPrimitiveComponent* InComponent, FHitProxyId InId);
	bool IsComponent() const override;
};

// 본 HitProxy (Bone-level picking)
class HBone : public HHitProxy
{
public:
	USkeletalMeshComponent* SkeletalMesh;
	int32 BoneIndex;

	HBone(USkeletalMeshComponent* InSkeletalMesh, int32 InBoneIndex, FHitProxyId InId);
	virtual bool IsBone() const;
};

// HitProxy 관리자
class FHitProxyManager
{
public:
	static FHitProxyManager& GetInstance();

	// HitProxy 할당 및 ID 반환
	FHitProxyId AllocateHitProxyId(HHitProxy* HitProxy);

	// ID로 HitProxy 조회
	HHitProxy* GetHitProxy(FHitProxyId Id) const;

	// 모든 HitProxy 제거 (프레임 시작 시 호출)
	void ClearAllHitProxies();

	~FHitProxyManager();

private:
	TMap<uint32, HHitProxy*> HitProxyMap;
	uint32 NextIndex;

	FHitProxyManager();
	FHitProxyManager(const FHitProxyManager&) = delete;
	FHitProxyManager& operator=(const FHitProxyManager&) = delete;
};
