#include "pch.h"
#include "Render/HitProxy/Public/HitProxy.h"

FHitProxyId::FHitProxyId()
	: Index(0)
{
}

FHitProxyId::FHitProxyId(uint32 InIndex)
	: Index(InIndex)
{
}

FHitProxyId::FHitProxyId(uint8 Red, uint8 Green, uint8 Blue)
	: Index((static_cast<uint32>(Red) << 16) | (static_cast<uint32>(Green) << 8) | static_cast<uint32>(Blue))
{
}

bool FHitProxyId::IsValid() const
{
	return Index != 0;
}

FVector4 FHitProxyId::GetColor() const
{
	uint8 Red = (Index >> 16) & 0xFF;
	uint8 Green = (Index >> 8) & 0xFF;
	uint8 Blue = (Index >> 0) & 0xFF;
	return {Red / 255.0f, Green / 255.0f, Blue / 255.0f, 1.0f};
}

bool FHitProxyId::operator==(const FHitProxyId& Other) const
{
	return Index == Other.Index;
}

bool FHitProxyId::operator!=(const FHitProxyId& Other) const
{
	return Index != Other.Index;
}

HHitProxy::HHitProxy(FHitProxyId InId)
	: Id(InId)
{
}

HHitProxy::~HHitProxy() = default;

bool HHitProxy::IsWidgetAxis() const
{
	return false;
}

bool HHitProxy::IsComponent() const
{
	return false;
}

HWidgetAxis::HWidgetAxis(EGizmoAxisType InAxis, FHitProxyId InId)
	: HHitProxy(InId), Axis(InAxis)
{
}

bool HWidgetAxis::IsWidgetAxis() const
{
	return true;
}

HComponent::HComponent(UPrimitiveComponent* InComponent, FHitProxyId InId)
	: HHitProxy(InId), Component(InComponent)
{
}

bool HComponent::IsComponent() const
{
	return true;
}

FHitProxyManager& FHitProxyManager::GetInstance()
{
	static FHitProxyManager Instance;
	return Instance;
}

FHitProxyManager::FHitProxyManager()
	: NextIndex(1) // 0은 배경용
{
}

FHitProxyManager::~FHitProxyManager()
{
	ClearAllHitProxies();
}

FHitProxyId FHitProxyManager::AllocateHitProxyId(HHitProxy* HitProxy)
{
	if (!HitProxy)
	{
		return InvalidHitProxyId;
	}

	// 새로운 ID 할당 (1부터 시작, 0은 배경)
	uint32 NewIndex = NextIndex++;
	FHitProxyId NewId(NewIndex);
	HitProxy->Id = NewId;

	// 맵에 등록
	HitProxyMap[NewIndex] = HitProxy;
	return NewId;
}

HHitProxy* FHitProxyManager::GetHitProxy(FHitProxyId Id) const
{
	return HitProxyMap.FindRef(Id.Index);
}

void FHitProxyManager::ClearAllHitProxies()
{
	for (auto& Pair : HitProxyMap)
	{
		delete Pair.second;
	}
	HitProxyMap.Empty();
	NextIndex = 1;
}
