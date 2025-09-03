#pragma once
#include "Scene.h"
#include <memory>
#include <unordered_map>

class SceneManager {
public:
	static SceneManager& GetInstance();
	void RegisterScene(const std::string& name, Scene* scene);
	void LoadScene(const std::string& name);
	void Update(float deltaTime);
	void Render(ID3D11DeviceContext* context);

	Scene* GetCurrentScene() { return m_currentScene; }
private:
	SceneManager() = default;
	std::unordered_map<std::string, Scene*> m_scenes;
	Scene* m_currentScene;
};
