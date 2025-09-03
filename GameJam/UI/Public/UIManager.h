#pragma once

#include <vector>
#include "UIElement.h"

class UIManager
{
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
