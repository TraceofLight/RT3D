#pragma once
#include "Scene.h"

class LobbyScene
	: public Scene
{
private:

public:
	LobbyScene() : Scene("MainMenu") { Init(); }
	void Init() override;
	void Update(float deltaTime) override;
	void Render() override;
	void Cleanup() override;
};
