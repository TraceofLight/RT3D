#include "pch.h"
#include "SceneManager.h"
#include <iostream>
SceneManager& SceneManager::GetInstance()
{
	static SceneManager instance;
	return instance;
}

void SceneManager::RegisterScene(const std::string& name, Scene* scene)
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

void SceneManager::LoadScene(const std::string& name)
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
}



void SceneManager::Update(float deltaTime)
{
}

void SceneManager::Render(ID3D11DeviceContext* context)
{
	
}






