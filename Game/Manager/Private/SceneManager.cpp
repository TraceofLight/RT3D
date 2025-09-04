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
	std::cout << "RegisterScene : " << name;
	if (scene) std::cout << "[success!]\n";
	else std::cout << "[scene is null]\n";
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
		std::cout << "LoadScene : [" << name << " NOT FOUND]\n";
		return;
	}
	m_currentScene->SetActive(false);
	m_currentScene->Cleanup();


	m_currentScene = m_scenes[name];
	m_currentScene->Init();
	m_currentScene->SetActive(true);
	dirtyflag = true;
}



void FSceneManager::Update(float deltaTime)
{
}

void FSceneManager::Render(ID3D11DeviceContext* context)
{

}






