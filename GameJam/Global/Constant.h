#pragma once

// Screen Size
constexpr int SCREEN_HEIGHT = 1024;
constexpr int SCREEN_WIDTH = 1024;

// Score System
// 1초마다 100점, 충돌 시 10점
constexpr int TIME_SCORE_PER_SECOND = 100;
constexpr int COLLISION_SCORE = 10;

// 원주율 상수
constexpr float PI = 3.1415926f;

constexpr float OBSTACLE_RESTITUTION = 0.8f; // 장애물 탄성 계수
constexpr float PAD_RESTITUTION = 1.2f;      // 패드 탄성 계수
constexpr float GRAVITY_ACCELERATION = 2.0f; // 중력 가속도
