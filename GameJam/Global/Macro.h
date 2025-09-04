#pragma once

extern bool bShowExternalTerminal;

// TimeManager 관련 매크로들
#define DT FTimeManager::GetInstance()->GetDeltaTime()

#define DEBUG_PRINT(format, ...) \
do { \
if (SHOW_EXTERNAL_TERMINAL) \
printf(format, ##__VA_ARGS__); \
else \
OutputDebugStringA((std::string(format).c_str())); \
} while(0)

// 문자열 포맷팅을 위한 헬퍼 매크로
#define DEBUG_PRINT_FORMAT(format, ...) \
do { \
if (SHOW_EXTERNAL_TERMINAL) \
printf(format, ##__VA_ARGS__); \
else { \
char buffer[1024]; \
sprintf_s(buffer, sizeof(buffer), format, ##__VA_ARGS__); \
OutputDebugStringA(buffer); \
} \
} while(0)
