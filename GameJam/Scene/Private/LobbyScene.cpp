#include "pch.h"
#include "Scene/Public/LobbyScene.h"

#include "Manager/Public/SceneManager.h"
#include "UI/Public/Button.h"
#include "Manager/Public/UIManager.h"

// UI생성 및 추가
void LobbyScene::Init()
{
	Button* startButton = new Button("GAME START", []()
	{
		DEBUG_PRINT("Game Start Button Pressed!\n");
		UIManager::GetInstance().Cleanup();
		FSceneManager::GetInstance().LoadScene("GAME START");
	});

	startButton->SetPosition(FVector3(0.0f, -0.2f, 0.0f));
	startButton->SetSize(FVector3(0.2f, 0.2f, 0.0f));

	UIManager::GetInstance().AddElement(startButton);

	Button* creditButton = new Button("CREDIT", []()
	{
		DEBUG_PRINT("CREDIT button Pressed\n");
	});
	creditButton->SetPosition(FVector3(0.0f, 0.2f, 0.0f));
	startButton->SetSize(FVector3(0.2f, 0.2f, 0.0f));
	UIManager::GetInstance().AddElement(creditButton);
}

void LobbyScene::Update(float deltaTime)
{
	UIManager::GetInstance().Update(deltaTime);
}

void LobbyScene::Render()
{
	UIManager::GetInstance().Render();
}

void LobbyScene::Cleanup()
{
	UIManager::GetInstance();
}
