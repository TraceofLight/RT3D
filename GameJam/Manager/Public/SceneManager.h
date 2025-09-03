#pragma once
#include "Core/Public/Primitive.h"
#include "Scene/Public/Scene.h"

class FSceneManager
{
public:
	static FSceneManager& GetInstance();
	void RegisterScene(const std::string& name, Scene* scene);
	void LoadScene(const std::string& name);
	void Update(float deltaTime);
	void Render(ID3D11DeviceContext* context);

	Scene* GetCurrentScene() const { return m_currentScene; }
	// TODO(KHJ): 구현 필요
	vector<UPrimitive*> GetAllScenePrimivites() { 
		if (m_currentScene) return m_currentScene->GetScenePrimitives(); 
		return vector<UPrimitive*>(); 
	}
	void AddPrimitiveToScene(UPrimitive* InPrimitive);

private:
	FSceneManager() : m_currentScene(nullptr) {}
	unordered_map<string, Scene*> m_scenes;
	Scene* m_currentScene;
	bool dirtyflag = false;
};
