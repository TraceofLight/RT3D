#pragma once
#include "UIElement.h"

class Text : public UIElement
{
public:
	Text() : m_fontSize(16) {}

	void SetText(const std::string& text) { m_text = text; }
	void SetFontSize(float size) { m_fontSize = size; }

	void Render() override {
		//render logic
	}
	void Update(float deltaTime) override;

private:
	std::string m_text;
	float m_fontSize;
};
