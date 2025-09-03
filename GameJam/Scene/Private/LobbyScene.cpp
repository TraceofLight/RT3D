#include "pch.h"
#include "Scene/Public/LobbyScene.h"

#include "Manager/Public/SceneManager.h"
#include "UI/Public/Button.h"
#include "UI/Public/UIManager.h"

void LobbyScene::Init()//UI생성 및 추가
{
	Button* startButton = new Button("GAME START", []() {
		std::cout << "Game Start Button Pressed!";
		UIManager::GetInstance().Cleanup();
		SceneManager::GetInstance().LoadScene("GAME START");});

	startButton->SetPosition(FVector3(0.0f, -0.2f, 0.0f));
	startButton->SetSize(FVector3(0.2f, 0.2f, 0.0f));

	UIManager::GetInstance().AddElement(startButton);

	Button* creditButton = new Button("CREDIT", []() {
		std::cout << "CREDIT button Pressed";
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

