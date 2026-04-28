#include "pch.h"
#include "BVH.h"
#include"StaticMeshActor.h"
#include "Picking.h"
#include "PickingTimer.h"
#include "UI/GlobalConsole.h"
#include <algorithm>
#include <cfloat>

FBVH::FBVH() : MaxDepth(0)
{
}

FBVH::~FBVH()
{
    Clear();
}

void FBVH::Build(const TArray<AActor*>& Actors)
{
    TStatId BVHBuildStatId;
    FScopeCycleCounter BVHBuildTimer(BVHBuildStatId);

    Clear();

    if (Actors.Num() == 0)
        return;

    // 1. 액터들의 AABB 정보 수집
    ActorBounds.Reserve(Actors.Num());
    ActorIndices.Reserve(Actors.Num());

    for (int i = 0; i < Actors.Num(); ++i)
    {
        AActor* Actor = Actors[i];
        if (!Actor || Actor->GetActorHiddenInGame())
            continue;

        // 액터의 AABB 가져오기 (기존 피킹 시스템과 동일한 방식)
        const FBound* ActorBounds_Local=nullptr;
        bool bHasBounds = false;

        // StaticMeshActor의 경우 AABB 컴포넌트에서 바운드 가져오기
        if (const AStaticMeshActor* StaticMeshActor = Cast<const AStaticMeshActor>(Actor))
        {
            for (auto Component : StaticMeshActor->GetComponents())//최적화
            {
                if (UAABoundingBoxComponent* AABBComponent = Cast<UAABoundingBoxComponent>(Component))
                {
                    ActorBounds_Local = AABBComponent->GetFBound();
                    bHasBounds = true;
                    break;
                }
            }
        }

        if (bHasBounds)
        {
            FActorBounds AB(Actor, *ActorBounds_Local);
            ActorBounds.Add(AB);
            ActorIndices.Add(ActorBounds.Num() - 1);
        }
    }

    if (ActorBounds.Num() == 0)
        return;

    // 2. 노드 배열 예약 (최악의 경우 2*N-1개 노드)
    Nodes.Reserve(ActorBounds.Num() * 2);

    // 3. 재귀적으로 BVH 구축
    MaxDepth = 0;
    int RootIndex = BuildRecursive(0, ActorBounds.Num(), 0);

    uint64_t BuildCycles = BVHBuildTimer.Finish();
    double BuildTimeMs = FPlatformTime::ToMilliseconds(BuildCycles);

    char buf[256];
    sprintf_s(buf, "[BVH] Built for %d actors, %d nodes, depth %d (Time: %.3fms)\n",
             ActorBounds.Num(), Nodes.Num(), MaxDepth, BuildTimeMs);
    UE_LOG(buf);
}

void FBVH::Clear()
{
    Nodes.Empty();
    ActorBounds.Empty();
    ActorIndices.Empty();
    MaxDepth = 0;
}

AActor* FBVH::Intersect(const FVector& RayOrigin, const FVector& RayDirection, float& OutDistance) const
{
    if (Nodes.Num() == 0)
        return nullptr;

    TStatId BVHIntersectStatId;
    FScopeCycleCounter BVHIntersectTimer(BVHIntersectStatId);

    OutDistance = FLT_MAX;
    AActor* HitActor = nullptr;

    bool bHit = IntersectNode(0, RayOrigin, RayDirection, OutDistance, HitActor);

    uint64_t IntersectCycles = BVHIntersectTimer.Finish();
    double IntersectTimeMs = FPlatformTime::ToMilliseconds(IntersectCycles);

    if (bHit)
    {
        char buf[256];
        sprintf_s(buf, "[BVH Pick] Hit actor at distance %.3f (Time: %.3fms)\n",
                 OutDistance, IntersectTimeMs);
        UE_LOG(buf);
        return HitActor;
    }
    else
    {
        char buf[256];
        sprintf_s(buf, "[BVH Pick] No hit (Time: %.3fms)\n", IntersectTimeMs);
        UE_LOG(buf);
        return nullptr;
    }
}

int FBVH::BuildRecursive(int FirstActor, int ActorCount, int Depth)
{
    MaxDepth = FMath::Max(MaxDepth, Depth);

    int NodeIndex = Nodes.Num();
    FBVHNode NewNode;
    Nodes.Add(NewNode);
    FBVHNode& Node = Nodes[NodeIndex];

    // 노드의 경계 박스 계산
    Node.BoundingBox = CalculateBounds(FirstActor, ActorCount);

    // 리프 노드 조건: 액터 수가 적거나 최대 깊이에 도달
    if (ActorCount <= MaxActorsPerLeaf || Depth >= MaxBVHDepth)
    {
        // 리프 노드로 설정
        Node.FirstActor = FirstActor;
        Node.ActorCount = ActorCount;
        return NodeIndex;
    }

    // 최적 분할 지점 찾기 (Surface Area Heuristic)
    int BestAxis;
    float BestSplitPos;
    int SplitIndex = FindBestSplit(FirstActor, ActorCount, BestAxis, BestSplitPos);

    // 분할이 의미없는 경우 (모든 액터가 한쪽으로 몰림) 리프로 만들기
    if (SplitIndex == FirstActor || SplitIndex == FirstActor + ActorCount)
    {
        Node.FirstActor = FirstActor;
        Node.ActorCount = ActorCount;
        return NodeIndex;
    }

    // 실제 분할 수행
    int ActualSplit = PartitionActors(FirstActor, ActorCount, BestAxis, BestSplitPos);

    int LeftCount = ActualSplit - FirstActor;
    int RightCount = ActorCount - LeftCount;

    // 분할 결과가 유효하지 않으면 리프로 만들기
    if (LeftCount == 0 || RightCount == 0)
    {
        Node.FirstActor = FirstActor;
        Node.ActorCount = ActorCount;
        return NodeIndex;
    }

    // 자식 노드들 재귀 생성
    Node.LeftChild = BuildRecursive(FirstActor, LeftCount, Depth + 1);
    Node.RightChild = BuildRecursive(ActualSplit, RightCount, Depth + 1);

    return NodeIndex;
}

FBound FBVH::CalculateBounds(int FirstActor, int ActorCount) const
{
    FBound Bounds;
    bool bFirst = true;

    for (int i = 0; i < ActorCount; ++i)
    {
        int ActorIndex = ActorIndices[FirstActor + i];
        const FBound& ActorBound = ActorBounds[ActorIndex].Bounds;

        if (bFirst)
        {
            Bounds = ActorBound;
            bFirst = false;
        }
        else
        {
            // Min/Max 확장
            Bounds.Min.X = FMath::Min(Bounds.Min.X, ActorBound.Min.X);
            Bounds.Min.Y = FMath::Min(Bounds.Min.Y, ActorBound.Min.Y);
            Bounds.Min.Z = FMath::Min(Bounds.Min.Z, ActorBound.Min.Z);

            Bounds.Max.X = FMath::Max(Bounds.Max.X, ActorBound.Max.X);
            Bounds.Max.Y = FMath::Max(Bounds.Max.Y, ActorBound.Max.Y);
            Bounds.Max.Z = FMath::Max(Bounds.Max.Z, ActorBound.Max.Z);
        }
    }

    return Bounds;
}

FBound FBVH::CalculateCentroidBounds(int FirstActor, int ActorCount) const
{
    FBound Bounds;
    bool bFirst = true;

    for (int i = 0; i < ActorCount; ++i)
    {
        int ActorIndex = ActorIndices[FirstActor + i];
        const FVector& Center = ActorBounds[ActorIndex].Center;

        if (bFirst)
        {
            Bounds.Min = Bounds.Max = Center;
            bFirst = false;
        }
        else
        {
            Bounds.Min.X = FMath::Min(Bounds.Min.X, Center.X);
            Bounds.Min.Y = FMath::Min(Bounds.Min.Y, Center.Y);
            Bounds.Min.Z = FMath::Min(Bounds.Min.Z, Center.Z);

            Bounds.Max.X = FMath::Max(Bounds.Max.X, Center.X);
            Bounds.Max.Y = FMath::Max(Bounds.Max.Y, Center.Y);
            Bounds.Max.Z = FMath::Max(Bounds.Max.Z, Center.Z);
        }
    }

    return Bounds;
}

int FBVH::FindBestSplit(int FirstActor, int ActorCount, int& OutAxis, float& OutSplitPos) const
{
    FBound CentroidBounds = CalculateCentroidBounds(FirstActor, ActorCount);
    FBound ParentBounds = CalculateBounds(FirstActor, ActorCount);

    // 가장 긴 축 선택
    FVector Extent = CentroidBounds.Max - CentroidBounds.Min;
    OutAxis = 0;
    if (Extent.Y > Extent.X) OutAxis = 1;
    if (Extent.Z > Extent[OutAxis]) OutAxis = 2;

    // 모든 센터가 같은 위치에 있으면 중간으로 분할
    if (Extent[OutAxis] < KINDA_SMALL_NUMBER)
    {
        OutSplitPos = CentroidBounds.Min[OutAxis];
        return FirstActor + ActorCount / 2;
    }

    // Surface Area Heuristic으로 최적 분할 찾기
    float BestCost = FLT_MAX;
    int BestSplit = FirstActor + ActorCount / 2;
    OutSplitPos = (CentroidBounds.Min[OutAxis] + CentroidBounds.Max[OutAxis]) * 0.5f;

    for (int i = 1; i < SAHSamples; ++i)
    {
        float t = (float)i / SAHSamples;
        float SplitPos = CentroidBounds.Min[OutAxis] + t * Extent[OutAxis];

        // 이 분할점에서의 왼쪽/오른쪽 개수 계산
        int LeftCount = 0;
        for (int j = 0; j < ActorCount; ++j)
        {
            int ActorIndex = ActorIndices[FirstActor + j];
            if (ActorBounds[ActorIndex].Center[OutAxis] < SplitPos)
                LeftCount++;
        }

        int RightCount = ActorCount - LeftCount;

        // 유효하지 않은 분할은 건너뛰기
        if (LeftCount == 0 || RightCount == 0)
            continue;

        float Cost = CalculateSAH(FirstActor, LeftCount, RightCount, ParentBounds);
        if (Cost < BestCost)
        {
            BestCost = Cost;
            BestSplit = FirstActor + LeftCount;
            OutSplitPos = SplitPos;
        }
    }

    return BestSplit;
}

static inline float SurfaceArea(const FBound& b) {
    FVector s = b.Max - b.Min;
    if (s.X <= 0 || s.Y <= 0 || s.Z <= 0) return 0.0f;
    return 2.0f * (s.X * s.Y + s.Y * s.Z + s.Z * s.X);
}

float FBVH::CalculateSAH(int FirstActor, int LeftCount, int RightCount, const FBound& Parent) const
{
    // 좌/우 AABB 계산 (간단하지만 O(N); 더 빠르게 하려면 prefix/suffix 누적 사용)
    FBound LB = CalculateBounds(FirstActor, LeftCount);
    FBound RB = CalculateBounds(FirstActor + LeftCount, RightCount);

    float SA_P = SurfaceArea(Parent) + 1e-6f;
    float SA_L = SurfaceArea(LB);
    float SA_R = SurfaceArea(RB);

    constexpr float Ct = 1.0f; // 트래버스 비용
    constexpr float Ci = 1.0f; // 교차 비용
    return Ct + Ci * ((SA_L / SA_P) * LeftCount + (SA_R / SA_P) * RightCount);
}
int FBVH::PartitionActors(int FirstActor, int ActorCount, int Axis, float SplitPos)
{
    int Left = FirstActor;
    int Right = FirstActor + ActorCount - 1;

    while (Left <= Right)
    {
        // 왼쪽에서 SplitPos보다 큰 액터 찾기
        while (Left <= Right)
        {
            int LeftActorIndex = ActorIndices[Left];
            const FVector& LeftCenter = ActorBounds[LeftActorIndex].Center;
            if (LeftCenter[Axis] >= SplitPos)
                break;
            Left++;
        }

        // 오른쪽에서 SplitPos보다 작은 액터 찾기
        while (Left <= Right)
        {
            int RightActorIndex = ActorIndices[Right];
            const FVector& RightCenter = ActorBounds[RightActorIndex].Center;
            if (RightCenter[Axis] < SplitPos)
                break;
            Right--;
        }

        // 스왑이 필요한 경우에만 수행
        if (Left < Right)
        {
            int Temp = ActorIndices[Left];
            ActorIndices[Left] = ActorIndices[Right];
            ActorIndices[Right] = Temp;
            Left++;
            Right--;
        }
    }

    return Left;
}

bool FBVH::IntersectNode(int NodeIndex,
    const FVector& RayOrigin,
    const FVector& RayDirection,
    float& InOutDistance,
    AActor*& OutActor) const
{
    const FBVHNode& Node = Nodes[NodeIndex];

    // ────── 1. 노드 AABB와 레이 교차 검사 ──────
    float tNear;
    if (!Node.BoundingBox.RayIntersects(RayOrigin, RayDirection, tNear))
        return false;

    // 이미 더 가까운 히트를 찾았으면 스킵
    if (tNear >= InOutDistance)
        return false;

    // ────── 2. 리프 노드 처리 ──────
    if (Node.IsLeaf())
    {
        bool bHit = false;
        float Closest = InOutDistance;
        AActor* ClosestActor = nullptr;

        for (int i = 0; i < Node.ActorCount; ++i)
        {
            int ActorIndex = ActorIndices[Node.FirstActor + i];
            AActor* Actor = ActorBounds[ActorIndex].Actor;

            float Dist;
            if (IntersectActor(Actor, RayOrigin, RayDirection, Dist))
            {
                if (Dist < Closest)
                {
                    Closest = Dist;
                    ClosestActor = Actor;
                    bHit = true;
                }
            }
        }

        if (bHit)
        {
            InOutDistance = Closest;
            OutActor = ClosestActor;
        }

        return bHit;
    }

    // ────── 3. 내부 노드 near-first 방문 ──────
    struct ChildHit
    {
        int Index;
        float tNear;
        bool bValid;
    };

    auto TestChild = [&](int ChildIdx) -> ChildHit
        {
            if (ChildIdx < 0) return { ChildIdx, FLT_MAX, false };
            float tN;
            if (Nodes[ChildIdx].BoundingBox.RayIntersects(RayOrigin, RayDirection, tN))
                return { ChildIdx, tN, true };
            return { ChildIdx, FLT_MAX, false };
        };

    ChildHit L = TestChild(Node.LeftChild);
    ChildHit R = TestChild(Node.RightChild);

    bool bHit = false;

    if (L.bValid && R.bValid)
    {
        // 가까운 쪽 먼저
        const ChildHit First = (L.tNear < R.tNear) ? L : R;
        const ChildHit Second = (L.tNear < R.tNear) ? R : L;

        if (IntersectNode(First.Index, RayOrigin, RayDirection, InOutDistance, OutActor))
            bHit = true;

        // 첫 번째에서 InOutDistance가 갱신되면 두 번째는 프루닝될 수 있음
        if (InOutDistance > Second.tNear)
        {
            if (IntersectNode(Second.Index, RayOrigin, RayDirection, InOutDistance, OutActor))
                bHit = true;
        }
    }
    else if (L.bValid)
    {
        if (IntersectNode(L.Index, RayOrigin, RayDirection, InOutDistance, OutActor))
            bHit = true;
    }
    else if (R.bValid)
    {
        if (IntersectNode(R.Index, RayOrigin, RayDirection, InOutDistance, OutActor))
            bHit = true;
    }

    return bHit;
}


bool FBVH::IntersectActor(const AActor* Actor, const FVector& RayOrigin, const FVector& RayDirection,
                          float& OutDistance) const
{
    // 기존 피킹 시스템의 CheckActorPicking과 동일한 로직 사용
    FRay Ray;
    Ray.Origin = RayOrigin;
    Ray.Direction = RayDirection;

    return CPickingSystem::CheckActorPicking(Actor, Ray, OutDistance);
}