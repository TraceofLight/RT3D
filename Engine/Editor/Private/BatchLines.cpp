#include "pch.h"
#include "Editor/Public/BatchLines.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Mesh/Public/ResourceManager.h"

UBatchLines::UBatchLines()
	: Grid()
	, BoundingBoxLines()
{
	Vertices.reserve(Grid.GetNumVertices() + BoundingBoxLines.GetNumVertices());

	Grid.MergeVerticesAt(Vertices, 0);
	BoundingBoxLines.MergeVerticesAt(Vertices, Vertices.size());

	SetIndices();

	URenderer& Renderer = URenderer::GetInstance();

	//BatchLineConstData.CellSize = 1.0f;
	//BatchLineConstData.BoundingBoxModel = FMatrix::Identity();

	/*AddWorldGridVerticesAndConstData();
	AddBoundingBoxVertices();*/

	// to do: 아래 코드가 맞는지 확인 필요(index buffer 가능하게 만들어야 할 것 같은데)
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	Primitive.NumVertices = static_cast<uint32>(Vertices.size());
	Primitive.IndexBuffer = Renderer.CreateIndexBuffer(Indices.data(), Primitive.NumIndices * sizeof(uint32));
	//Primitive.Color = FVector4(1, 1, 1, 0.2f);
	Primitive.Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	Primitive.Vertexbuffer = Renderer.CreateVertexBuffer(
		Vertices.data(), Primitive.NumVertices * sizeof(FVector));
	Primitive.Location = FVector(0, 0, 0);
	Primitive.Rotation = FVector(0, 0, 0);
	Primitive.Scale = FVector(1, 1, 1);
	Primitive.VertexShader = UResourceManager::GetInstance().GetVertexShader(EShaderType::BatchLine);
	Primitive.InputLayout = UResourceManager::GetInstance().GetIputLayout(EShaderType::BatchLine);
	Primitive.PixelShader = UResourceManager::GetInstance().GetPixelShader(EShaderType::BatchLine);
}

void UBatchLines::UpdateUGridVertices(const float newCellSize)
{
	if (newCellSize == Grid.GetCellSize())
	{
		return;
	}
	Grid.UpdateVerticesBy(newCellSize);
	Grid.MergeVerticesAt(Vertices, 0);
}

void UBatchLines::UpdateBoundingBoxVertices(const FBoundingBox& newBoundingBoxInfo)
{
	FBoundingBox curBoudingBoxInfo = BoundingBoxLines.GetRenderedBoxInfo();
	if (newBoundingBoxInfo.min == curBoudingBoxInfo.min && newBoundingBoxInfo.max == curBoudingBoxInfo.max)
	{
		return;
	}
	BoundingBoxLines.UpdateVertices(newBoundingBoxInfo);
	BoundingBoxLines.MergeVerticesAt(Vertices, Grid.GetNumVertices());
}

void UBatchLines::UpdateBatchLineVertices(const float newCellSize, const FBoundingBox& newBoundingBoxInfo)
{
	UpdateUGridVertices(newCellSize);
	UpdateBoundingBoxVertices(newBoundingBoxInfo);
}

//void UBatchLines::UpdateConstant(FBoundingBox boundingBoxInfo)
//{
//	Primitive.Scale = boundingBoxInfo.max - boundingBoxInfo.min;
//	Primitive.Location = boundingBoxInfo.min;
//	URenderer::GetInstance().UpdateConstant(Primitive.Location, Primitive.Rotation, Primitive.Scale);
//	URenderer::GetInstance().UpdateAndSetBatchLineConstant(BatchLineConstData);
//}

//void BatchLines::Update()
//{
//	URenderer::GetInstance().UpdateConstant();
//}

void UBatchLines::Render()
{
	URenderer& Renderer = URenderer::GetInstance();

	// to do: 아래 함수를 batch에 맞게 수정해야 함.
	Renderer.RenderPrimitiveIndexed(Primitive, Primitive.RenderState, false);
}

void UBatchLines::SetIndices()
{
	for (int index = 0; index < Grid.GetNumVertices(); ++index)
	{
		Indices.push_back(index);
	}

	// 인덱스 (각 면이 시계방향이 되도록)
	uint32_t boundngBoxIdx[36] = {
		// 앞면 (+Z에서 봤을 때 시계방향)
		0, 2, 1,  0, 3, 2,
		// 뒷면 (-Z에서 봤을 때 시계방향)
		4, 5, 6,  4, 6, 7,
		// 왼쪽면 (-X에서 봤을 때 시계방향)
		4, 7, 3,  4, 3, 0,
		// 오른쪽면 (+X에서 봤을 때 시계방향)
		1, 2, 6,  1, 6, 5,
		// 윗면 (+Y에서 봤을 때 시계방향)
		3, 7, 6,  3, 6, 2,
		// 아랫면 (-Y에서 봤을 때 시계방향)
		4, 0, 1,  4, 1, 5
	};

	for (int i = 0; i < 36; ++i)
		Indices.push_back(boundngBoxIdx[i]);
}

//void UBatchLines::AddWorldGridVerticesAndConstData()
//{
//	const float& cellSize = BatchLineConstData.CellSize;
//
//	float LineLength = cellSize * static_cast<float>(NumLines) / 2.f;
//
//	uint32 index = 0;
//	for (int32 LineCount = -NumLines / 2; LineCount < NumLines / 2; ++LineCount) // z축 라인
//	{
//		if (LineCount == 0)
//		{
//			Vertices.push_back({ {static_cast<float>(LineCount) * cellSize,0.f , -LineLength}, Primitive.Color });
//			Vertices.push_back({ {static_cast<float>(LineCount) * cellSize,0.f , 0.f}, Primitive.Color });
//		}
//		else
//		{
//			Vertices.push_back({ {static_cast<float>(LineCount) * cellSize,0.f , -LineLength}, Primitive.Color });
//			Vertices.push_back({ {static_cast<float>(LineCount) * cellSize,0.f , LineLength}, Primitive.Color });
//		}
//
//		Indices.push_back(index);
//		Indices.push_back(index + 1);
//		index += 2;
//	}
//
//	BatchLineConstData.ZGridStartIndex = Vertices.size();
//
//	for (int32 LineCount = -NumLines / 2; LineCount < NumLines / 2; ++LineCount) // x축 라인
//	{
//		if (LineCount == 0)
//		{
//			Vertices.push_back({ {-LineLength, 0.f, static_cast<float>(LineCount) * cellSize}, Primitive.Color });
//			Vertices.push_back({ {0.f, 0.f, static_cast<float>(LineCount) * cellSize}, Primitive.Color });
//		}
//		else
//		{
//			Vertices.push_back({ {-LineLength, 0.f, static_cast<float>(LineCount) * cellSize}, Primitive.Color });
//			Vertices.push_back({ {LineLength, 0.f, static_cast<float>(LineCount) * cellSize}, Primitive.Color });
//		}
//
//		Indices.push_back(index);
//		Indices.push_back(index + 1);
//		index += 2;
//	}
//
//	BatchLineConstData.BoundingBoxStartIndex = Vertices.size();
//}
//
//void UBatchLines::AddBoundingBoxVertices()
//{
//	float minX = 0.0f, minY = 0.0f, minZ = 0.0f;
//	float maxX = 1.0f, maxY = 1.0f, maxZ = 1.0f;
//
//	// 꼭짓점 정의 (0~3: 앞면, 4~7: 뒷면)
//	Vertices[0] = { {minX, minY, minZ}, {1, 0, 0, 1} }; // Front-Bottom-Left
//	Vertices[1] = { {maxX, minY, minZ}, {1, 0, 0, 1} }; // Front-Bottom-Right
//	Vertices[2] = { {maxX, maxY, minZ}, {1, 0, 0, 1} }; // Front-Top-Right
//	Vertices[3] = { {minX, maxY, minZ}, {1, 0, 0, 1} }; // Front-Top-Left
//	Vertices[4] = { {minX, minY, maxZ}, {1, 0, 0, 1} }; // Back-Bottom-Left
//	Vertices[5] = { {maxX, minY, maxZ}, {1, 0, 0, 1} }; // Back-Bottom-Right
//	Vertices[6] = { {maxX, maxY, maxZ}, {1, 0, 0, 1} }; // Back-Top-Right
//	Vertices[7] = { {minX, maxY, maxZ}, {1, 0, 0, 1} }; // Back-Top-Left
//
//	// 인덱스 (각 면이 시계방향이 되도록)
//	uint32_t idx[36] = {
//		// 앞면 (+Z에서 봤을 때 시계방향)
//		0, 2, 1,  0, 3, 2,
//		// 뒷면 (-Z에서 봤을 때 시계방향)
//		4, 5, 6,  4, 6, 7,
//		// 왼쪽면 (-X에서 봤을 때 시계방향)
//		4, 7, 3,  4, 3, 0,
//		// 오른쪽면 (+X에서 봤을 때 시계방향)
//		1, 2, 6,  1, 6, 5,
//		// 윗면 (+Y에서 봤을 때 시계방향)
//		3, 7, 6,  3, 6, 2,
//		// 아랫면 (-Y에서 봤을 때 시계방향)
//		4, 0, 1,  4, 1, 5
//	};
//
//	for (int i = 0; i < 36; ++i)
//		Indices.push_back(idx[i]);
//}
