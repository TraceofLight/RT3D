#include "pch.h"
#include "Manager/Asset/Public/FbxLoader.h"

FbxLoader::FbxLoader() : m_pFBXManager(nullptr), m_pFBXImporter(nullptr), m_pFBXScene(nullptr)
{

}

bool FbxLoader::Initialize()
{
	m_pFBXManager = FbxManager::Create();
	if (!m_pFBXManager)
	{
		UE_LOG("Failed To Create FBXManager");
		return false;
	}
	m_pFBXImporter = FbxImporter::Create(m_pFBXManager, "");
	if (!m_pFBXImporter)
	{
		UE_LOG("Failed To Create m_pFBXImporter ");
		return false;
	}
	m_pFBXScene = FbxScene::Create(m_pFBXManager, "");
	if (!m_pFBXScene)
	{
		UE_LOG("Failed To Create m_pFBXImporter ");
		return false;
	}
	//m_pSettings = FbxIOSettings::Create(m_pFBXManager, IOSROOT);
	return true;
}

bool FbxLoader::ImportFromFile(const char* filename)
{
	if (!m_pFBXImporter || !m_pFBXManager || !m_pFBXScene)
	{
		UE_LOG("Call FbxLoader::Initialize First");

		return false;
	}

	const bool bImportStatus = m_pFBXImporter->Initialize(filename); //세팅도 넘길 수 있다.
	if (!bImportStatus)
	{
		UE_LOG("Importer로 %s 가져오기 실패", filename);
		return false;
	}

	bool bLoadStatus = m_pFBXImporter->Import(m_pFBXScene);
	if (!bLoadStatus)
	{
		UE_LOG("FBXImporter로 Import 실패");
		return false;
	}

	FbxSystemUnit SceneUnit = m_pFBXScene->GetGlobalSettings().GetSystemUnit();
	if (SceneUnit != FbxSystemUnit::m)
	{
		FbxSystemUnit::m.ConvertScene(m_pFBXScene);
	}

	//좌표계 변환
	// Z-front로 변경
	//FbxAxisSystem TargetSystem(FbxAxisSystem::eZAxis, FbxAxisSystem::eParityEven, FbxAxisSystem::eLeftHanded);
	//FbxAxisSystem SceneSystem = m_pFBXScene->GetGlobalSettings().GetAxisSystem(); 
	//if (SceneSystem != TargetSystem)
	//{
	//	TargetSystem.ConvertScene(m_pFBXScene);
	//}
	FbxAxisSystem TargetSystem(FbxAxisSystem::eZAxis, FbxAxisSystem::eParityEven, FbxAxisSystem::eRightHanded);


	// 지오메트리 전처리
	FbxGeometryConverter Converter(m_pFBXManager);
	Converter.Triangulate(m_pFBXScene, true, false);
	Converter.RemoveBadPolygonsFromMeshes(m_pFBXScene);

	OutVertices.Empty();
	OutIndices.Empty();
	MeshList.Empty();

	//TODO
	// import하는 정보를 설정할 수 있다. (ex. animation, keyframe... )
	//SetBoolProp( , true/false);


	FbxNode* RootNode = m_pFBXScene->GetRootNode();

	if (RootNode)
	{
		NodeProcess(nullptr, RootNode);

		for (int ObjID = 0; ObjID < MeshList.Num(); ++ObjID)
		{
			ParseMesh(MeshList[ObjID]);
		}

		return true;

	}
	else
	{
		UE_LOG("FBXScene에 RootNode가 없습니다");
		return false;
	}

	return !OutVertices.IsEmpty() && !OutIndices.IsEmpty();
}

// 카메라, 라이트 예외 처리하지 않음
// Mesh만 있다고 가정했음 
void FbxLoader::NodeProcess(FbxNode* ParentNode, FbxNode* Node)
{
	FbxMesh* Mesh = Node->GetMesh();

	if (Mesh)
	{
		FBXObj* CurNode = new FBXObj;  
		CurNode->CurNode = Node;
		CurNode->ParentNode = ParentNode; 
		MeshList.Add(CurNode);
	}

	const int NumChild = Node->GetChildCount();
	for (int NodeCount = 0; NodeCount < NumChild; ++NodeCount)
	{
		FbxNode* Child = Node->GetChild(NodeCount);
		NodeProcess(Node, Child);
	}
}

void FbxLoader::ParseMesh(FBXObj* NodeObj)
{
	FbxNode* Node = NodeObj->CurNode;  
	FbxMesh* Mesh = NodeObj->CurNode->GetMesh();
	if (!Mesh) return;

	// 글로벌 변환
	FbxAMatrix Global = Node->EvaluateGlobalTransform(); // World Transform
	FbxAMatrix Geometry = GetGeometryTransform(Node); // pivot은 두고, pivotㄱ
	FbxAMatrix Total = Global * Geometry;

	// 노말이 없으면 생성
	if (!Mesh->GetElementNormalCount())
	{
		Mesh->GenerateNormals(true, true, true);
	}

	// Get UV 세트 이름
	FbxStringList UVNames;
	Mesh->GetUVSetNames(UVNames);
	const char* UVSetName = (UVNames.GetCount() > 0) ? UVNames[0] : nullptr;

	FbxVector4* ControlPoints = Mesh->GetControlPoints();

	std::unordered_map<VertexKey, uint32, VertexKeyHash> Map;

	const int PolyCount = Mesh->GetPolygonCount(); 

	int NumFace = 0;
	uint32 CCWIndices[3];

	for (int p = 0; p < PolyCount; ++p)
	{
		// Vector VS Vector4
		const int PolySize = Mesh->GetPolygonSize(p);
		if (PolySize != 3)
		{
			UE_LOG("triangulate해줬는데 왜 아니야!!");
			continue;
		}

		for (int v = 0; v < 3; ++v)
		{
			int CPIndex = Mesh->GetPolygonVertex(p, v);

			// Position
			FbxVector4 P = ControlPoints[CPIndex];
			FbxVector4 Pw = Total.MultT(P); // World Space로 변환하는 작업

			// Normal
			FbxVector4 Nl;
			bool hasNormal = Mesh->GetPolygonVertexNormal(p, v, Nl);
			if (!hasNormal)
			{
				Nl = FbxVector4(0, 0, 1, 0);
			}
			FbxAMatrix NMat = Total;
			NMat = NMat.Inverse(); NMat = NMat.Transpose();
			FbxVector4 Nw = NMat.MultT(Nl);

			//normalize
			double len = sqrt(Nw[0] * Nw[0] + Nw[1] * Nw[1] + Nw[2] * Nw[2]);
			if (len > 1e-12) { Nw[0] /= len; Nw[1] /= len; Nw[2] /= len; }
					
			// UV
			FbxVector2 UV(0, 0);
			if (UVSetName)
			{
				bool unmapped = false;
				UV[1] = 1.0 - UV[1];

				Mesh->GetPolygonVertexUV(p, v, UVSetName, UV, unmapped);
				// DirectX 계열 V 뒤집기 필요 시: UV[1] = 1.0 - UV[1];
			}

			// Key 생성
			VertexKey key{
				CPIndex,
				Q((float)Nw[0]), Q((float)Nw[1]), Q((float)Nw[2]),
				Q((float)UV[0]), Q((float)UV[1])
			};

			auto it = Map.find(key);
			uint32 idx;

			// 처음 보는 키면, 계산한 정보들로 reset
			// texture 가 없을 때 기본 색상은 흰색
			// 탄젠트는 나중에 
			if (it == Map.end())
			{
				FNormalVertex V = {};
				V.Position = FVector((float)Pw[0], (float)Pw[1], (float)Pw[2]);
				V.Normal = FVector((float)Nw[0], (float)Nw[1], (float)Nw[2]);
				V.Color = FVector4(1, 1, 1, 1);
				V.TexCoord = FVector2((float)UV[0], (float)UV[1]);
				V.Tangent = FVector4(0, 0, 0, 1);

				idx = (uint32)OutVertices.Num();
				OutVertices.Add(V);
				Map.emplace(key, idx);
			}
			else
			{
				idx = it->second;
			}

			CCWIndices[v] = idx;
 		}
		std::swap(CCWIndices[0], CCWIndices[2]);

		OutIndices.Add(CCWIndices[0]);
		OutIndices.Add(CCWIndices[1]);
		OutIndices.Add(CCWIndices[2]);
	}

	//TODO: 필요시 CPU 탄젠트 계산  (ObjManager에서 참고)
}

FbxAMatrix FbxLoader::GetGeometryTransform(FbxNode* Node)
{
	const FbxVector4 T = Node->GetGeometricTranslation(FbxNode::eSourcePivot);
	const FbxVector4 R  = Node->GetGeometricRotation(FbxNode::eSourcePivot);
	const FbxVector4 S = Node->GetGeometricScaling(FbxNode::eSourcePivot);

	FbxAMatrix Translation;
	Translation.SetT(T);

	FbxAMatrix Rotation;
	Rotation.SetR(R);

	FbxAMatrix Scaling;
	Scaling.SetS(S);

	return Translation * Rotation * Scaling;
}

bool FbxLoader::Release()
{
	for (FBXObj* Obj : MeshList)
	{
		delete Obj;
	}

	if (m_pFBXImporter)
	{
		m_pFBXImporter->Destroy();
		m_pFBXImporter = nullptr;
	}
	if (m_pFBXScene)
	{
		m_pFBXScene->Destroy();
		m_pFBXScene = nullptr;
	}
	if(m_pFBXManager)
	{
		m_pFBXManager->Destroy();
		m_pFBXManager = nullptr;
	}
	return true;
}

// x y z
// y z  x
