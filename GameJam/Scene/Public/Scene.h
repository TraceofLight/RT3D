#pragma once

class UPrimitive;

class Scene
{
private:
	vector<UPrimitive*> ScenePrimivites;

protected:
	string m_name;
	bool m_isActive;
	bool bIsPause;

public:
	explicit Scene(const string& name) : m_name(name), m_isActive(false), bIsPause(false)
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
	void RemovePrimitive(UPrimitive* InPrimitive)
	{
		auto iter = std::find(ScenePrimivites.begin(), ScenePrimivites.end(), InPrimitive);
		if (iter != ScenePrimivites.end())
		{
			ScenePrimivites.erase(iter);
		}
	}
	// vector<UPrimitive*> GetScenePrimitives() const { return ScenePrimivites; }
	vector<UPrimitive*>* GetScenePrimitives() { return &ScenePrimivites; }
	void SetPause() { bIsPause = true; }
	void SetResume() { bIsPause = false; }
	bool IsPause() const { return bIsPause; }
};
