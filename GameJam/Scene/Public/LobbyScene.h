#pragma once
#include "Scene.h"

class LobbyScene
	: public Scene
{
private:

public:
	LobbyScene() : Scene("LOBBY") {}
	void Init() override;
	void Update(float deltaTime) override;
	void Render() override;
	void Cleanup() override;
};
