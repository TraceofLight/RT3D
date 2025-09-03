#pragma once

#include <vector>
#include <memory>
#include <string>
#include "UIElement.h"
class UIElement;
class ID3D11DeviceContext;
class InputState;
class ID3D11Buffer;
class ID3D11VertexShader;
class ID3D11PixelShader;
class ID3D11InputLayout;

class UIManager {
private:
	UIManager() = default;
	std::vector<UIElement*> UIelements;

public:
	static UIManager& GetInstance();

	void AddElement(UIElement* element);
	void Update(float deltaTime);
	void Render();
	void OnMouseClick(float x, float y);
};
