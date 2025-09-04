#pragma once

// FIXME(KHJ): Singleton 패턴화해서 구현되면 적용할 것
class FInputManager
{
private:
	static FInputManager* Instance;

	// Key Status
	map<EKeyInput, bool> CurrentKeyState;
	map<EKeyInput, bool> PreviousKeyState;
	map<int, EKeyInput> VirtualKeyMap;

	// Mouse Position
	FVector2 CurrentMousePosition;
	FVector2 PreviousMousePosition;
	FVector2 MouseDelta;

	// Window Focus
	bool bIsWindowFocused;

private:
	void InitializeKeyMapping();
	FInputManager();

public:
	void Update();
	void UpdateMousePosition();
	void ProcessKeyMessage(UINT InMessage, WPARAM WParam, LPARAM LParam);

	bool IsKeyDown(EKeyInput InKey) const;
	bool IsKeyPressed(EKeyInput InKey) const;
	bool IsKeyReleased(EKeyInput InKey) const;

	EKeyStatus GetKeyStatus(EKeyInput InKey) const;
	vector<EKeyInput> GetKeysByStatus(EKeyStatus InStatus) const;

	vector<EKeyInput> GetPressedKeys() const;
	vector<EKeyInput> GetNewlyPressedKeys() const;
	vector<EKeyInput> GetReleasedKeys() const;

	// Helper Function
	static const char* KeyInputToString(EKeyInput InKey);

	// Window Focus Management
	void SetWindowFocus(bool bInFocused);
	bool IsWindowFocused() const { return bIsWindowFocused; }

	// Getter
	FVector2 GetMousePosition() const { return CurrentMousePosition; }
	FVector2 GetMouseDelta() const { return MouseDelta; }

	// Special Member Function
	static FInputManager* GetInstance();
	~FInputManager();
};
