#pragma once

enum class EKeyInput : UINT8
{
	// 이동 키
	W,
	A,
	S,
	D,

	// 화살표 키
	Up,
	Down,
	Left,
	Right,

	// 액션 키
	Space,
	Enter,
	Esc,
	Tab,
	Shift,
	Ctrl,
	Alt,

	// 숫자 키
	Num0,
	Num1,
	Num2,
	Num3,
	Num4,
	Num5,
	Num6,
	Num7,
	Num8,
	Num9,

	// 마우스
	MouseLeft,
	MouseRight,
	MouseMiddle,

	// 기타
	F1,
	F2,
	F3,
	F4,
	Backspace,
	Delete,

	End
};
