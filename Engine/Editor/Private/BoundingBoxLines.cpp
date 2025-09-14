#include "pch.h"
#include "Editor/Public/BoundingBoxLines.h"

UBoundingBoxLines::UBoundingBoxLines()
	: Vertices(TArray<FVector>())
	, NumVertices(8)
{
	Vertices.reserve(NumVertices);
	UpdateVertices({ {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f} });
}

void UBoundingBoxLines::MergeVerticesAt(TArray<FVector>& destVertices, size_t insertStartIndex)
{
	// 인덱스 범위 보정
	if (insertStartIndex > destVertices.size())
		insertStartIndex = destVertices.size();

	// 미리 메모리 확보
	destVertices.reserve(destVertices.size() + std::distance(Vertices.begin(), Vertices.end()));

	// 덮어쓸 수 있는 개수 계산
	size_t overwriteCount = std::min(
		Vertices.size(),
		destVertices.size() - insertStartIndex
	);

	// 기존 요소 덮어쓰기
	std::copy(
		Vertices.begin(),
		Vertices.begin() + overwriteCount,
		destVertices.begin() + insertStartIndex
	);

	// 원하는 위치에 삽입
	/*destVertices.insert(
		destVertices.begin() + insertStartIndex,
		Vertices.begin(),
		Vertices.end()
	);*/
}

void UBoundingBoxLines::UpdateVertices(FBoundingBox boundingBoxInfo)
{
	// 중복 삽입 방지
	if (RenderedBoxInfo.min == boundingBoxInfo.min && RenderedBoxInfo.max == boundingBoxInfo.max)
	{
		return;
	}

	float minX = boundingBoxInfo.min.X, minY = boundingBoxInfo.min.Y, minZ = boundingBoxInfo.min.Z;
	float maxX = boundingBoxInfo.max.X, maxY = boundingBoxInfo.max.Y, maxZ = boundingBoxInfo.max.Z;

	if (Vertices.size() < NumVertices)
	{
		Vertices.resize(NumVertices);
	}

	// 꼭짓점 정의 (0~3: 앞면, 4~7: 뒷면)
	uint32 vertexIndex = 0;
	Vertices[vertexIndex++] = { minX, minY, minZ }; // Front-Bottom-Left
	Vertices[vertexIndex++] = { maxX, minY, minZ }; // Front-Bottom-Right
	Vertices[vertexIndex++] = { maxX, maxY, minZ }; // Front-Top-Right
	Vertices[vertexIndex++] = { minX, maxY, minZ }; // Front-Top-Left
	Vertices[vertexIndex++] = { minX, minY, maxZ }; // Back-Bottom-Left
	Vertices[vertexIndex++] = { maxX, minY, maxZ }; // Back-Bottom-Right
	Vertices[vertexIndex++] = { maxX, maxY, maxZ }; // Back-Top-Right
	Vertices[vertexIndex++] = { minX, maxY, maxZ }; // Back-Top-Left
}
