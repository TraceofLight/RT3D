#include "pch.h"
#include "Editor/Public/Grid.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Manager/Config/Public/ConfigManager.h"

UGrid::UGrid()
	: Vertices()
{
	NumVertices = NumLines * 4;
	Vertices.Reserve(static_cast<int32>(NumVertices));
	Vertices.SetNum(static_cast<int32>(NumVertices)); // Add this line to ensure Vertices has enough elements
	UpdateVerticesBy(UConfigManager::GetInstance().LoadCellSize());
}

UGrid::~UGrid()
{
	UConfigManager::GetInstance().SaveCellSize(CellSize);
}

void UGrid::UpdateVerticesBy(float NewCellSize)
{
	constexpr float EPSILON = 0.00001f;

	// 중복 삽입 방지
	if (abs(CellSize - NewCellSize) < EPSILON)
	{
		return;
	}

	CellSize = NewCellSize; // 필요하다면 멤버 변수도 갱신

	float LineLength = NewCellSize * static_cast<float>(NumLines) / 2.f;

	// Vertices.SetNum(static_cast<int32>(NumVertices)); // This line is now in the constructor

	uint32 vertexIndex = 0;
	FVector4 GridColor = FVector4(0.5f, 0.5f, 0.5f, 1.0f); // Default gray color for grid lines
	// Y축 라인 업데이트 (X 방향으로 그리는 선들)
	for (int32 LineCount = -NumLines / 2; LineCount < NumLines / 2; ++LineCount)
	{
		Vertices[static_cast<int32>(vertexIndex++)] = { { static_cast<float>(LineCount) * NewCellSize, -LineLength, 0.0f }, GridColor };
		Vertices[static_cast<int32>(vertexIndex++)] = { { static_cast<float>(LineCount) * NewCellSize, LineLength, 0.0f }, GridColor };
	}

	// X축 라인 업데이트 (Y 방향으로 그리는 선들)
	for (int32 LineCount = -NumLines / 2; LineCount < NumLines / 2; ++LineCount)
	{
		Vertices[static_cast<int32>(vertexIndex++)] = { { -LineLength, static_cast<float>(LineCount) * NewCellSize, 0.0f }, GridColor };
		Vertices[static_cast<int32>(vertexIndex++)] = { { LineLength, static_cast<float>(LineCount) * NewCellSize, 0.0f }, GridColor };
	}
}

void UGrid::MergeVerticesAt(TArray<FVertexPositionColor>& DestVertices, size_t InsertStartIndex)
{
	// 인덱스 범위 보정
	InsertStartIndex = std::min(DestVertices.Num(), static_cast<int32>(InsertStartIndex));

	// 미리 메모리 확보
	DestVertices.Reserve(
		static_cast<int32>(DestVertices.Num() + std::distance(Vertices.begin(), Vertices.end())));

	// 덮어쓸 수 있는 개수 계산
	size_t overwriteCount = std::min(
		Vertices.Num(),
		DestVertices.Num() - static_cast<int32>(InsertStartIndex)
	);

	// 기존 요소 덮어쓰기
	std::copy(
		Vertices.begin(),
		Vertices.begin() + overwriteCount,
		DestVertices.begin() + InsertStartIndex
	);
}
