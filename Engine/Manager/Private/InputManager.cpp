#include "pch.h"
#include "Manager/Public/InputManager.h"

FInputManager* FInputManager::Instance = nullptr;

FInputManager::FInputManager()
	: bIsWindowFocused(true)
{
	InitializeKeyMapping();
}

FInputManager::~FInputManager()
{
	Instance = nullptr;
}

FInputManager* FInputManager::GetInstance()
{
	if (!Instance)
	{
		Instance = new FInputManager();
	}
	return Instance;
}

void FInputManager::InitializeKeyMapping()
{
	// 알파벳 키 매핑
	VirtualKeyMap['W'] = EKeyInput::W;
	VirtualKeyMap['A'] = EKeyInput::A;
	VirtualKeyMap['S'] = EKeyInput::S;
	VirtualKeyMap['D'] = EKeyInput::D;

	// 화살표 키 매핑
	VirtualKeyMap[VK_UP] = EKeyInput::Up;
	VirtualKeyMap[VK_DOWN] = EKeyInput::Down;
	VirtualKeyMap[VK_LEFT] = EKeyInput::Left;
	VirtualKeyMap[VK_RIGHT] = EKeyInput::Right;

	// 액션 키 매핑
	VirtualKeyMap[VK_SPACE] = EKeyInput::Space;
	VirtualKeyMap[VK_RETURN] = EKeyInput::Enter;
	VirtualKeyMap[VK_ESCAPE] = EKeyInput::Esc;
	VirtualKeyMap[VK_TAB] = EKeyInput::Tab;
	VirtualKeyMap[VK_SHIFT] = EKeyInput::Shift;
	VirtualKeyMap[VK_CONTROL] = EKeyInput::Ctrl;
	VirtualKeyMap[VK_MENU] = EKeyInput::Alt;

	// 숫자 키 매핑
	VirtualKeyMap['0'] = EKeyInput::Num0;
	VirtualKeyMap['1'] = EKeyInput::Num1;
	VirtualKeyMap['2'] = EKeyInput::Num2;
	VirtualKeyMap['3'] = EKeyInput::Num3;
	VirtualKeyMap['4'] = EKeyInput::Num4;
	VirtualKeyMap['5'] = EKeyInput::Num5;
	VirtualKeyMap['6'] = EKeyInput::Num6;
	VirtualKeyMap['7'] = EKeyInput::Num7;
	VirtualKeyMap['8'] = EKeyInput::Num8;
	VirtualKeyMap['9'] = EKeyInput::Num9;

	// 마우스 버튼 (특별 처리 필요)
	VirtualKeyMap[VK_LBUTTON] = EKeyInput::MouseLeft;
	VirtualKeyMap[VK_RBUTTON] = EKeyInput::MouseRight;
	VirtualKeyMap[VK_MBUTTON] = EKeyInput::MouseMiddle;

	// 기타 키 매핑
	VirtualKeyMap[VK_F1] = EKeyInput::F1;
	VirtualKeyMap[VK_F2] = EKeyInput::F2;
	VirtualKeyMap[VK_F3] = EKeyInput::F3;
	VirtualKeyMap[VK_F4] = EKeyInput::F4;
	VirtualKeyMap[VK_BACK] = EKeyInput::Backspace;
	VirtualKeyMap[VK_DELETE] = EKeyInput::Delete;

	// 모든 키 상태를 false로 초기화
	for (int i = 0; i < static_cast<int>(EKeyInput::End); ++i)
	{
		EKeyInput Key = static_cast<EKeyInput>(i);
		CurrentKeyState[Key] = false;
		PreviousKeyState[Key] = false;
	}
}

void FInputManager::Update()
{
	// 이전 프레임 상태를 현재 프레임 상태로 복사
	PreviousKeyState = CurrentKeyState;

	// 윈도우가 포커스를 잃었을 때는 입력 처리를 중단
	if (!bIsWindowFocused)
	{
		// 모든 키 상태를 false로 유지
		for (auto& Pair : CurrentKeyState)
		{
			Pair.second = false;
		}
		return;
	}

	// 마우스 위치 업데이트
	UpdateMousePosition();

	// GetAsyncKeyState를 사용하여 현재 키 상태를 업데이트
	for (auto& Pair : VirtualKeyMap)
	{
		int VirtualKey = Pair.first;
		EKeyInput KeyInput = Pair.second;

		// 마우스 버튼은 GetAsyncKeyState가 잘 작동하지 않을 수 있으므로 메시지 기반으로 처리
		if (KeyInput == EKeyInput::MouseLeft || KeyInput == EKeyInput::MouseRight || KeyInput == EKeyInput::MouseMiddle)
		{
			// 마우스 버튼은 ProcessKeyMessage에서 처리
			continue;
		}

		// GetAsyncKeyState의 반환값에서 최상위 비트가 1이면 키가 눌린 상태
		bool IsKeyDown = (GetAsyncKeyState(VirtualKey) & 0x8000) != 0;
		CurrentKeyState[KeyInput] = IsKeyDown;
	}
}

void FInputManager::UpdateMousePosition()
{
	PreviousMousePosition = CurrentMousePosition;

	POINT MousePoint;
	if (GetCursorPos(&MousePoint))
	{
		ScreenToClient(GetActiveWindow(), &MousePoint);
		CurrentMousePosition.x = static_cast<float>(MousePoint.x);
		CurrentMousePosition.y = static_cast<float>(MousePoint.y);
	}

	MouseDelta = CurrentMousePosition - PreviousMousePosition;
}

bool FInputManager::IsKeyDown(EKeyInput InKey) const
{
	auto Iter = CurrentKeyState.find(InKey);
	if (Iter != CurrentKeyState.end())
	{
		return Iter->second;
	}
	return false;
}

bool FInputManager::IsKeyPressed(EKeyInput InKey) const
{
	auto CurrentIter = CurrentKeyState.find(InKey);
	auto PrevIter = PreviousKeyState.find(InKey);

	if (CurrentIter != CurrentKeyState.end() && PrevIter != PreviousKeyState.end())
	{
		// 이전 프레임에는 안 눌렸고, 현재 프레임에는 눌림
		return CurrentIter->second && !PrevIter->second;
	}

	return false;
}

bool FInputManager::IsKeyReleased(EKeyInput InKey) const
{
	auto CurrentIter = CurrentKeyState.find(InKey);
	auto PrevIter = PreviousKeyState.find(InKey);

	if (CurrentIter != CurrentKeyState.end() && PrevIter != PreviousKeyState.end())
	{
		// 이전 프레임에는 눌렸고, 현재 프레임에는 안 눌림
		return !CurrentIter->second && PrevIter->second;
	}
	return false;
}

void FInputManager::ProcessKeyMessage(UINT InMessage, WPARAM WParam, LPARAM LParam)
{
	switch (InMessage)
	{
	//case WM_KEYDOWN:
	//case WM_SYSKEYDOWN:
	//	{
	//		auto it = VirtualKeyMap.find(static_cast<int>(WParam));
	//		if (it != VirtualKeyMap.end())
	//		{
	//			CurrentKeyState[it->second] = true;
	//		}
	//	}
	//	break;

	//case WM_KEYUP:
	//case WM_SYSKEYUP:
	//	{
	//		auto it = VirtualKeyMap.find(static_cast<int>(WParam));
	//		if (it != VirtualKeyMap.end())
	//		{
	//			CurrentKeyState[it->second] = false;
	//		}
	//	}
	//	break;

	case WM_LBUTTONDOWN:
		CurrentKeyState[EKeyInput::MouseLeft] = true;
		break;

	case WM_LBUTTONUP:
		CurrentKeyState[EKeyInput::MouseLeft] = false;
		break;

	case WM_RBUTTONDOWN:
		CurrentKeyState[EKeyInput::MouseRight] = true;
		break;

	case WM_RBUTTONUP:
		CurrentKeyState[EKeyInput::MouseRight] = false;
		break;

	case WM_MBUTTONDOWN:
		CurrentKeyState[EKeyInput::MouseMiddle] = true;
		break;

	case WM_MBUTTONUP:
		CurrentKeyState[EKeyInput::MouseMiddle] = false;
		break;

	default:
		break;
	}
}

vector<EKeyInput> FInputManager::GetKeysByStatus(EKeyStatus InStatus) const
{
	vector<EKeyInput> Keys;

	for (int i = 0; i < static_cast<int>(EKeyInput::End); ++i)
	{
		EKeyInput Key = static_cast<EKeyInput>(i);
		if (GetKeyStatus(Key) == InStatus)
		{
			Keys.push_back(Key);
		}
	}

	return Keys;
}

EKeyStatus FInputManager::GetKeyStatus(EKeyInput InKey) const
{
	auto CurrentIter = CurrentKeyState.find(InKey);
	auto PrevIter = PreviousKeyState.find(InKey);

	if (CurrentIter == CurrentKeyState.end() || PrevIter == PreviousKeyState.end())
	{
		return EKeyStatus::Unknown;
	}

	// Pressed -> Released -> Down -> Up
	if (CurrentIter->second && !PrevIter->second)
	{
		return EKeyStatus::Pressed;
	}
	if (!CurrentIter->second && PrevIter->second)
	{
		return EKeyStatus::Released;
	}
	if (CurrentIter->second)
	{
		return EKeyStatus::Down;
	}
	return EKeyStatus::Up;
}

vector<EKeyInput> FInputManager::GetPressedKeys() const
{
	return GetKeysByStatus(EKeyStatus::Down);
}

vector<EKeyInput> FInputManager::GetNewlyPressedKeys() const
{
	// Pressed 상태의 키들을 반환
	return GetKeysByStatus(EKeyStatus::Pressed);
}

vector<EKeyInput> FInputManager::GetReleasedKeys() const
{
	// Released 상태의 키들을 반환
	return GetKeysByStatus(EKeyStatus::Released);
}

const char* FInputManager::KeyInputToString(EKeyInput InKey)
{
	static string KeyString;
	KeyString = string(magic_enum::enum_name<EKeyInput>(InKey));

	return KeyString.c_str();
}

void FInputManager::SetWindowFocus(bool bInFocused)
{
	bIsWindowFocused = bInFocused;

	if (!bInFocused)
	{
		for (auto& Pair : CurrentKeyState)
		{
			Pair.second = false;
		}

		for (auto& Pair : PreviousKeyState)
		{
			Pair.second = false;
		}
	}
}
