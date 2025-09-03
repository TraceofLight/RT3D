#pragma once

#include "UIElement.h"
#include "Text.h"

class Button : public UIElement {
private:
	Text ButtonText;
	function<void()> OnClickFunction;
	bool bIsPressed = false;
	bool bIsHovered = false;

public:
	Button(const string& InButtonText, function<void()> InCallback);

	void Update(float InDeltaTime) override;
	void Render() override;

	void SetOnClick(function<void()> InCallback) { OnClickFunction = InCallback; }
	void SetText(string InString) { ButtonText.SetText(InString); }

	bool OnMouseClick(FVector3 loc) override;
};
