#pragma once
#include"pch.h"
#include "EditorEngine.h"
#include"GameEngine.h"
#include "World.h"
#include "SelectionManager.h"
#include "FViewportClient.h"
#include"SViewportWindow.h"
#include"SMultiViewportWindow.h"
UEditorEngine::UEditorEngine()
{
}

void UEditorEngine::Tick(float DeltaSeconds)
{
    // 지연 삭제 처리 (이전 프레임에서 요청된 삭제)
    if (bPIEShutdownRequested)
    {
        // GameEngine 삭제
        if (PendingDeleteGameEngine)
        {
            ObjectFactory::DeleteObject(PendingDeleteGameEngine);
            //  PendingDeleteGameEngine->EndGame();  // CleanupWorld 호출
            PendingDeleteGameEngine = nullptr;
        }

        // PIE 월드 삭제
        if (PendingDeletePIEWorld)
        {
            ObjectFactory::DeleteObject(PendingDeletePIEWorld);
            PendingDeletePIEWorld = nullptr;
        }

        bPIEShutdownRequested = false;
    }

    // PIE 실행 중이면 PIE 월드만 Tick
    if (GameEngine && GameEngine->GameWorld)
    {
        GameEngine->Tick(DeltaSeconds);
    }
    else
    {
        // 에디터 모드일 때만 에디터 월드 Tick
        for (size_t i = 0; i < WorldContexts.size(); ++i)
        {
            UWorld* World = WorldContexts[i].World();
            if (!World) continue;

            if (WorldContexts[i].WorldType == EWorldType::Editor)
            {
                World->Tick(DeltaSeconds);
            }
        }
    }
}

void UEditorEngine::Render()
{
    // PIE 실행 중이면 PIE 월드만 렌더링
    if (GameEngine && GameEngine->GameWorld)
    {
        GameEngine->Render();
    }
    else
    {
        // 에디터 모드일 때만 에디터 월드 렌더링
        for (size_t i = 0; i < WorldContexts.size(); ++i)
        {
            UWorld* World = WorldContexts[i].World();
            if (!World) continue;

            if (WorldContexts[i].WorldType == EWorldType::Editor)
            {
                World->Render();
            }
        }
    }
}

void UEditorEngine::StartPIE()
{
    UWorld* EditorWorld = GetWorld(EWorldType::Editor);
    if (!EditorWorld) return;

    // PIE 시작 전 에디터 선택 해제 (PIE와 에디터 선택 분리)
    USelectionManager::GetInstance().ClearSelection();

    UWorld* PIEWorld = UWorld::DuplicateWorldForPIE(EditorWorld);
    if (!PIEWorld) return;



    FWorldContext PieCtx;
    PieCtx.SetWorld(PIEWorld, EWorldType::PIE);
    WorldContexts.push_back(PieCtx);

    // GWorld를 PIE 월드로 전환
    GWorld = PIEWorld;

    // 메인 뷰포트 ViewportClient를 PIE 월드로 전환
    PIEWorld->GetMainViewport()->GetViewportClient()->SetWorld(PIEWorld);


    GameEngine = NewObject<UGameEngine>();
    GameEngine->StartGame(PIEWorld);

    PIEWorld->InitializeActorsForPlay();
}

void UEditorEngine::EndPIE()
{
    if (!GameEngine)
        return;

    UWorld* PIEWorld = nullptr;

    // PIE 월드 찾기
    for (auto& Context : WorldContexts)
    {
        if (Context.WorldType == EWorldType::PIE)
        {
            PIEWorld = Context.World();
            break;
        }
    }

    // PIE 종료 시 선택 해제 (PIE 액터 참조 제거)
    USelectionManager::GetInstance().ClearSelection();

    // ViewportClient의 World를 에디터 월드로 복원
    UWorld* EditorWorld = GetWorld(EWorldType::Editor);
    if (EditorWorld && EditorWorld->GetMainViewport())
    {
        if (FViewportClient* ViewportClient = EditorWorld->GetMainViewport()->GetViewportClient())
        {
            ViewportClient->SetWorld(EditorWorld);
        }
    }

    // GameEngine 정리 (CleanupWorld 호출)
    if (GameEngine)
    {
        // 지연 삭제를 위해 저장
        PendingDeleteGameEngine = GameEngine;

        GameEngine = nullptr;  // 즉시 nullptr로 설정해서 Tick/Render에서 안 쓰도록
    }

    // PIE 월드 지연 삭제 예약
    if (PIEWorld)
    {
        PendingDeletePIEWorld = PIEWorld;
    }

    // 다음 Tick에서 삭제하도록 플래그 설정
    bPIEShutdownRequested = true;

    // PIE WorldContext 제거 (즉시)
    for (int i = (int)WorldContexts.size() - 1; i >= 0; --i)
    {
        if (WorldContexts[i].WorldType == EWorldType::PIE)
        {
            WorldContexts.erase(WorldContexts.begin() + i);
            break;
        }
    }
}

UEditorEngine::~UEditorEngine()
{
}

