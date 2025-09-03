#pragma once

class UPrimitive;

class Scene
{
private:
	vector<UPrimitive*> ScenePrimivites;

public:
	Scene(const string& name) : m_name(name), m_isActive(false)
	{
	}

	virtual ~Scene() = default;

	virtual void Init() = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void Render() = 0;
	virtual void Cleanup() = 0;

	const string& GetName() const { return m_name; }
	bool IsActive() const { return m_isActive; }
	void SetActive(bool active) { m_isActive = active; }
	void AddPrimitive(UPrimitive* InPrimitive) { ScenePrimivites.push_back(InPrimitive); }
	vector<UPrimitive*>* GetScenePrimitives()  { return &ScenePrimivites; }
	bool SetPause(bool p) { pause = p; }
	bool GetPause() { return pause; }
protected:
	std::string m_name;
	bool m_isActive;
	bool pause;
};
