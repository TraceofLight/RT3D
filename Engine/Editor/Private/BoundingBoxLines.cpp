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

	// 원하는 위치에 삽입
	destVertices.insert(
		destVertices.begin() + insertStartIndex,
		Vertices.begin(),
		Vertices.end()
	);
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

	// 꼭짓점 정의 (0~3: 앞면, 4~7: 뒷면)
	Vertices[0] = { minX, minY, minZ }; // Front-Bottom-Left
	Vertices[1] = { maxX, minY, minZ }; // Front-Bottom-Right
	Vertices[2] = { maxX, maxY, minZ }; // Front-Top-Right
	Vertices[3] = { minX, maxY, minZ }; // Front-Top-Left
	Vertices[4] = { minX, minY, maxZ }; // Back-Bottom-Left
	Vertices[5] = { maxX, minY, maxZ }; // Back-Bottom-Right
	Vertices[6] = { maxX, maxY, maxZ }; // Back-Top-Right
	Vertices[7] = { minX, maxY, maxZ }; // Back-Top-Left
}
