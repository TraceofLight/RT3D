#pragma once
#include "Global/Types.h"
#include "Global/CoreTypes.h"
#include "Editor/Public/EditorPrimitive.h"

class FVertex;

class BatchLines
{
public:
	BatchLines();
	~BatchLines();

	void SetCellSize(const float size)
	{
		BatchLineConstData.CellSize = size;
	}

	float GetCellSize() const
	{
		return BatchLineConstData.CellSize;
	}

	void UpdateBoundingBoxTransformBy(FBoundingBox boundingBoxInfo);

	void RenderBatchLines();

private:
	void AddWorldGridVerticesAndConstData();
	void AddBoundingBoxVertices();

	TArray<FVertex> Vertices; // 그리드 라인 정보 + (offset 후)디폴트 바운딩 박스 라인 정보(minx, miny가 0,0에 정의된 크기가 1인 cube)
	TArray<uint32> Indices; // 월드 그리드는 그냥 정점 순서, 바운딩 박스는 실제 인덱싱
	BatchLineContants BatchLineConstData;
	FEditorPrimitive Primitive;

	int32 NumLines;
};
