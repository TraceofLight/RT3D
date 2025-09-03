#include "pch.h"
#include "Manager/Public/SceneManager.h"

#include "Scene/Public/Scene.h"

FSceneManager& FSceneManager::GetInstance()
{
	static FSceneManager instance;
	return instance;
}

void FSceneManager::RegisterScene(const std::string& name, Scene* scene)
{
	if (scene)
		DEBUG_PRINT_FORMAT("RegisterScene : %s [success!]\n", name.c_str());
	else
		DEBUG_PRINT_FORMAT("RegisterScene : %s [scene is null]\n", name.c_str());
	m_scenes[name] = scene;
	if (m_currentScene == nullptr)
	{
		m_currentScene = scene;
	}
}

void FSceneManager::LoadScene(const std::string& name)
{
	if (m_scenes.find(name) == m_scenes.end())
	{
		DEBUG_PRINT_FORMAT("LoadScene : [%s NOT FOUND]\n", name.c_str());
		return;
	}
	m_currentScene->SetActive(false);
	m_currentScene->Cleanup();


	m_currentScene = m_scenes[name];
	m_currentScene->Init();
	m_currentScene->SetActive(true);
}



void FSceneManager::Update(float deltaTime)
{
}

void FSceneManager::Render(ID3D11DeviceContext* context)
{

}

void FSceneManager::AddPrimitiveToScene(UPrimitive* InPrimitive)
{
	if (m_currentScene && InPrimitive)
	{
		m_currentScene->AddPrimitive(InPrimitive);
		DEBUG_PRINT("[SCENE_MANAGER] Primitive added to scene\n");
	}
	else
	{
		DEBUG_PRINT("[SCENE_MANAGER] Failed to add primitive - null scene or primitive\n");
	}
}






