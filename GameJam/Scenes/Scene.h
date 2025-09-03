#pragma once
#include <string>
#include "UI/UIElement.h"
class ID3D11DeviceContext;

class Scene
{
public:
    Scene(const std::string& name) : m_name(name), m_isActive(false) {}
    virtual ~Scene() = default;

    virtual void Init() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
    virtual void Cleanup() = 0;

    const std::string& GetName() const { return m_name; }
    bool IsActive() const { return m_isActive; }
    void SetActive(bool active) { m_isActive = active; }

protected:
    std::string m_name;
    bool m_isActive;
};
