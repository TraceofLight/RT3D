#include "pch.h"
#include <iostream>
#include "LobbyScene.h"
#include "UI/Button.h"
#include "SceneManager.h"
#include "UI/UIManager.h"
void LobbyScene::Init()//UI생성 및 추가
{
	Button* startButton = new Button("GAME START", []() {
		std::cout << "Game Start Button Pressed!";
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

}

