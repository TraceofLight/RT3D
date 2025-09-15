#pragma once
#include "Core/Public/Object.h"
#include "Global/CoreTypes.h"

class UBoundingBoxLines : UObject
{
public:
	UBoundingBoxLines();
	~UBoundingBoxLines() = default;

	void MergeVerticesAt(TArray<FVector>& destVertices, size_t insertStartIndex);
	void UpdateVertices(FBoundingBox boundingBoxInfo);

	uint32 GetNumVertices() const
	{
		return NumVertices;
	}

	FBoundingBox GetRenderedBoxInfo() const
	{
		return RenderedBoxInfo;
	}

private:
	TArray<FVector> Vertices;
	uint32 NumVertices = 8;
	FBoundingBox RenderedBoxInfo;
};
