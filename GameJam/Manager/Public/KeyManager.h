#pragma once

// FIXME(KHJ): Singleton 패턴화해서 구현되면 적용할 것
class FKeyManager
{
private:
	static FKeyManager* Instance;
	map<EKeyInput, bool> CurrentKeyState;
	map<EKeyInput, bool> PreviousKeyState;
	map<int, EKeyInput> VirtualKeyMap;

private:
	void InitializeKeyMapping();
	FKeyManager();

public:
	void Update();
	void ProcessKeyMessage(UINT InMessage, WPARAM WParam, LPARAM LParam);

	bool IsKeyDown(EKeyInput InKey) const;
	bool IsKeyPressed(EKeyInput InKey) const;
	bool IsKeyReleased(EKeyInput InKey) const;

	static FKeyManager* GetInstance();
	~FKeyManager();
};
