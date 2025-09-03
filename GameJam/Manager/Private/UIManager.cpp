#include "pch.h"
#include "Manager/Public/UIManager.h"

UIManager& UIManager::GetInstance()
{
	static UIManager instance;
	return instance;
}

void UIManager::AddElement(UIElement* element)
{
	UIelements.push_back(element);
}

void UIManager::Cleanup()
{
	UIelements.clear();
}

void UIManager::Update(float deltaTime)
{
	for (auto& element : UIelements)
	{
		element->Update(deltaTime);
	}
}

void UIManager::Render()
{
	for (auto elem : UIelements)
	{
		elem->Render();
	}
}


void UIManager::OnMouseClick(float x, float y)
{
	for (auto it = UIelements.rbegin(); it != UIelements.rend(); it++)
	{
		UIElement* elem = *it;

		if (elem != nullptr)
		{
			bool handled = elem->OnMouseClick(FVector3(x, y));
			if (handled) break;
		}
	}
	return;
}
