#pragma once

#include "UIElement.h"
#include "Text.h"

class Button : public UIElement {
private:
	Text text;
	std::function<void()> onClick;
	bool isPressed = false;
	bool isHovered = false;

public:
	Button(const std::string& buttonText, std::function<void()> callback);

	void Update(float deltaTime) override;
	void Render() override;

	void SetOnClick(std::function<void()> callback) { onClick = callback; }
	void SetText(std::string str) { text.SetText(str); }

	bool OnMouseClick(FVector3 loc);
};
