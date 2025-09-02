#include "pch.h"
#include "Manager/Public/KeyManager.h"

FKeyManager* FKeyManager::Instance = nullptr;

FKeyManager::FKeyManager()
{
	InitializeKeyMapping();
}

FKeyManager::~FKeyManager()
{
	Instance = nullptr;
}

FKeyManager* FKeyManager::GetInstance()
{
	if (!Instance)
	{
		Instance = new FKeyManager();
	}
	return Instance;
}

void FKeyManager::InitializeKeyMapping()
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

void FKeyManager::Update()
{
	// 이전 프레임 상태를 현재 프레임 상태로 복사
	PreviousKeyState = CurrentKeyState;

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

bool FKeyManager::IsKeyDown(EKeyInput InKey) const
{
	auto Iter = CurrentKeyState.find(InKey);
	if (Iter != CurrentKeyState.end())
	{
		return Iter->second;
	}
	return false;
}

bool FKeyManager::IsKeyPressed(EKeyInput InKey) const
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

bool FKeyManager::IsKeyReleased(EKeyInput InKey) const
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

void FKeyManager::ProcessKeyMessage(UINT InMessage, WPARAM WParam, LPARAM LParam)
{
	// Windows 메시지 기반 키 처리 (옵션)
	// 현재는 GetAsyncKeyState를 주로 사용하므로 필요시 구현
	switch (InMessage)
	{
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		{
			auto it = VirtualKeyMap.find(static_cast<int>(WParam));
			if (it != VirtualKeyMap.end())
			{
				CurrentKeyState[it->second] = true;
			}
		}
		break;

	case WM_KEYUP:
	case WM_SYSKEYUP:
		{
			auto it = VirtualKeyMap.find(static_cast<int>(WParam));
			if (it != VirtualKeyMap.end())
			{
				CurrentKeyState[it->second] = false;
			}
		}
		break;

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

vector<EKeyInput> FKeyManager::GetPressedKeys() const
{
	vector<EKeyInput> PressedKeys;

	for (const auto& KeyPair : CurrentKeyState)
	{
		// 키가 눌린 상태일 경우 처리
		if (KeyPair.second)
		{
			PressedKeys.push_back(KeyPair.first);
		}
	}

	return PressedKeys;
}

const char* FKeyManager::KeyInputToString(EKeyInput InKey)
{
	switch (InKey)
	{
	case EKeyInput::W: return "W";
	case EKeyInput::A: return "A";
	case EKeyInput::S: return "S";
	case EKeyInput::D: return "D";

	case EKeyInput::Up: return "↑";
	case EKeyInput::Down: return "↓";
	case EKeyInput::Left: return "←";
	case EKeyInput::Right: return "→";

	case EKeyInput::Space: return "Space";
	case EKeyInput::Enter: return "Enter";
	case EKeyInput::Esc: return "Esc";
	case EKeyInput::Tab: return "Tab";
	case EKeyInput::Shift: return "Shift";
	case EKeyInput::Ctrl: return "Ctrl";
	case EKeyInput::Alt: return "Alt";

	case EKeyInput::Num0: return "0";
	case EKeyInput::Num1: return "1";
	case EKeyInput::Num2: return "2";
	case EKeyInput::Num3: return "3";
	case EKeyInput::Num4: return "4";
	case EKeyInput::Num5: return "5";
	case EKeyInput::Num6: return "6";
	case EKeyInput::Num7: return "7";
	case EKeyInput::Num8: return "8";
	case EKeyInput::Num9: return "9";

	case EKeyInput::MouseLeft: return "Mouse Left Click";
	case EKeyInput::MouseRight: return "Mouse Right Click";
	case EKeyInput::MouseMiddle: return "Mouse Wheel Click";

	case EKeyInput::F1: return "F1";
	case EKeyInput::F2: return "F2";
	case EKeyInput::F3: return "F3";
	case EKeyInput::F4: return "F4";
	case EKeyInput::Backspace: return "Backspace";
	case EKeyInput::Delete: return "Delete";
	case EKeyInput::End:
		// Nothing

	default: return "Unknown";
	}
}
