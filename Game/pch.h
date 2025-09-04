#pragma once

#include <windows.h>

// D3D 사용에 필요한 라이브러리들을 링크합니다.
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

// D3D 사용에 필요한 헤더파일들을 포함합니다.
#include <d3d11.h>
#include <d3dcompiler.h>

// Standard Library
#include <cmath>
#include <map>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <functional>

// Magic Enum
#include <magic_enum/magic_enum.hpp>

#include "Global/Constant.h"
#include "Global/Struct.h"
#include "Global/Function.h"
#include "Global/Enum.h"
#include "Global/Macro.h"

using std::map;
using std::vector;
using std::string;
using std::clamp;
using std::unordered_map;
using std::to_string;
using std::function;

#include "Manager/Public/TimeManager.h"
