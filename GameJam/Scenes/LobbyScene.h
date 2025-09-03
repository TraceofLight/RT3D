#include "Scene.h"
#include "UI/UIElement.h"
class LobbyScene : public Scene{
public:
	LobbyScene() : Scene("MainMenu") { Init(); }
	void Init() override;
	void Update(float deltaTime) override;
	void Render() override;
	void Cleanup() override;
private:
};
