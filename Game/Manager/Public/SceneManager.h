#pragma once
#include "Actor/Public/PinBall.h"
#include "Scene/Public/Scene.h"

class Scene;

class FSceneManager
{
public:
	static FSceneManager& GetInstance();
	void RegisterScene(const std::string& name, Scene* scene);
	void LoadScene(const std::string& name);
	void Update(float deltaTime);
	void Render(ID3D11DeviceContext* context);

	Scene* GetCurrentScene() const { return m_currentScene; }
	vector<UPrimitive*>* GetAllScenePrimivites() const { return m_currentScene->GetScenePrimitives(); }
	void AddPrimitiveToScene(UPrimitive* InPrimitive) { m_currentScene->AddPrimitive(InPrimitive); }
	void RemovePrimitiveFromScene(UPrimitive* InPrimitive) { m_currentScene->RemovePrimitive(InPrimitive); }

private:
	FSceneManager() = default;
	unordered_map<string, Scene*> m_scenes;
	Scene* m_currentScene;
	bool dirtyflag = false;
};
