#pragma once
#include "Vector.h"
#include "UEContainer.h"
#include"AABoundingBoxComponent.h"
#include "Actor.h"

struct FBound;

// BVH 노드 - 메모리 효율성을 위한 컴팩트한 구조
struct FBVHNode
{
    FBound BoundingBox;

    // 리프 노드: FirstActor >= 0, ActorCount > 0
    // 내부 노드: LeftChild >= 0, RightChild >= 0
  
        
            int FirstActor=0;
            int ActorCount=0;
      
            int LeftChild =-1;
            int RightChild = -1;
       
   

    bool IsLeaf() const { return LeftChild < 0 && RightChild < 0; }

    FBVHNode() : FirstActor(0), ActorCount(0) {}
};

// 액터의 AABB와 포인터를 저장
struct FActorBounds
{
    FBound Bounds;
    AActor* Actor;
    FVector Center;

    FActorBounds() : Actor(nullptr) {}
    FActorBounds(AActor* InActor, const FBound& InBounds)
        : Actor(InActor), Bounds(InBounds)
    {
        Center = (InBounds.Min + InBounds.Max) * 0.5f;
    }
};

// 고성능 BVH 구현
class FBVH
{
public:
    FBVH();
    ~FBVH();

    // 액터 배열로부터 BVH 구축
    void Build(const TArray<AActor*>& Actors);
    void Clear();

    // 빠른 레이 교차 검사 - 가장 가까운 액터 반환
    AActor* Intersect(const FVector& RayOrigin, const FVector& RayDirection, float& OutDistance) const;

    // 통계 정보
    int GetNodeCount() const { return Nodes.Num(); }
    int GetActorCount() const { return ActorBounds.Num(); }
    int GetMaxDepth() const { return MaxDepth; }

    // 렌더링을 위한 노드 접근
    const TArray<FBVHNode>& GetNodes() const { return Nodes; }

private:
    TArray<FBVHNode> Nodes;
    TArray<FActorBounds> ActorBounds;
    TArray<int> ActorIndices; // 정렬된 액터 인덱스

    int MaxDepth;

    // 재귀 구축 함수
    int BuildRecursive(int FirstActor, int ActorCount, int Depth = 0);

    // 경계 박스 계산
    FBound CalculateBounds(int FirstActor, int ActorCount) const;
    FBound CalculateCentroidBounds(int FirstActor, int ActorCount) const;

    // Surface Area Heuristic을 이용한 최적 분할
    int FindBestSplit(int FirstActor, int ActorCount, int& OutAxis, float& OutSplitPos) const;
    float CalculateSAH(int FirstActor, int LeftCount, int RightCount, const FBound& ParentBounds) const;

    // 액터 분할
    int PartitionActors(int FirstActor, int ActorCount, int Axis, float SplitPos);

    // 재귀 교차 검사
    bool IntersectNode(int NodeIndex, const FVector& RayOrigin, const FVector& RayDirection,
                       float& InOutDistance, AActor*& OutActor) const;

    // 액터와의 교차 검사
    bool IntersectActor(const AActor* Actor, const FVector& RayOrigin, const FVector& RayDirection,
                        float& OutDistance) const;

    // 상수
    static const int MaxActorsPerLeaf = 8;  // 리프당 최대 액터 수 (마이크로 BVH용)
    static const int MaxBVHDepth = 24;      // 최대 깊이
    static const int SAHSamples = 10;       // SAH 샘플링 수
};