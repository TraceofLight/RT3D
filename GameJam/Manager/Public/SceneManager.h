#pragma once

class Scene;

class SceneManager
{
public:
	static SceneManager& GetInstance();
	void RegisterScene(const std::string& name, Scene* scene);
	void LoadScene(const std::string& name);
	void Update(float deltaTime);
	void Render(ID3D11DeviceContext* context);

	Scene* GetCurrentScene() { return m_currentScene; }

private:
	SceneManager() = default;
	unordered_map<string, Scene*> m_scenes;
	Scene* m_currentScene;
};
