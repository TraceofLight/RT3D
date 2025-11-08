#pragma once
#include "FBX/fbxsdk.h"

using namespace fbxsdk;


enum EAxis
{
	eAxis_Y,
	eAxis_z
};

struct FBXObj
{
	FbxNode* ParentNode;
	FbxNode* CurNode;
};

struct VertexKey
{
	int ControlPoint;
	int nX, nY, nZ;
	int u, v;

	bool operator==(const VertexKey& o) const
	{
		return ControlPoint == o.ControlPoint &&
			nX == o.nX && nY == o.nY && nZ == o.nZ &&
			u == o.u && v == o.v;
	}
};

struct VertexKeyHash
{
	size_t operator() (const VertexKey& k) const
	{
		size_t h = std::hash<int>{} (k.ControlPoint);
		auto mix = [&](int x)
			{
				h ^= std::hash<int>{}(x) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
			};

		mix(k.nX);
		mix(k.nY);
		mix(k.nZ);
		mix(k.u);
		mix(k.v);

		return h;
	}
};

static inline int Q(float f, float mul = 10000.0f) { return (int)llroundf(f * mul); }


class FbxLoader
{
public:
	FbxLoader();

	bool Initialize();
	bool Release();

	bool ImportFromFile(const char* filename);

	void NodeProcess(FbxNode* ParentNode, FbxNode* Node);
	void ParseMesh(FBXObj* Node);

	static FbxAMatrix GetGeometryTransform(FbxNode* Node);
public:
	FbxManager* m_pFBXManager;
	FbxImporter* m_pFBXImporter;
	FbxScene* m_pFBXScene;
	//FbxIOSettings* m_pSettings;

	TArray<FBXObj*> MeshList;

	// 파싱 결과물을 저장하는 곳
	TArray<FNormalVertex> OutVertices;
	TArray<uint32> OutIndices;
};
