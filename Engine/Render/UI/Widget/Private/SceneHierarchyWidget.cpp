#include "pch.h"
#include "Render/UI/Widget/Public/SceneHierarchyWidget.h"

#include "Manager/Level/Public/LevelManager.h"
#include "Level/Public/Level.h"
#include "Mesh/Public/Actor.h"
#include "Mesh/Public/CubeActor.h"
#include "Mesh/Public/SphereActor.h"
#include "Mesh/Public/TriangleActor.h"
#include "Mesh/Public/SquareActor.h"
#include "Editor/Public/Camera.h"

USceneHierarchyWidget::USceneHierarchyWidget()
	: UWidget("Scene Hierarchy Widget")
{
}

USceneHierarchyWidget::~USceneHierarchyWidget() = default;

void USceneHierarchyWidget::Initialize()
{
	UE_LOG("SceneHierarchyWidget: Initialized");
}

void USceneHierarchyWidget::Update()
{
	// 필요한 경우 UI 상태 업데이트
}

void USceneHierarchyWidget::RenderWidget()
{
	ULevel* CurrentLevel = ULevelManager::GetInstance().GetCurrentLevel();

	if (!CurrentLevel)
	{
		ImGui::TextUnformatted("No Level Loaded");
		return;
	}

	// 헤더 정보
	ImGui::Text("Level: %s", CurrentLevel->GetName().ToString().c_str());
	ImGui::Separator();

	// --- View Mode Selection ---
	ImGui::Text("View Mode");
	EViewModeIndex currentMode = ULevelManager::GetInstance().GetEditor()->GetViewMode();
	int modeIndex = static_cast<int>(currentMode);

	ImGui::RadioButton("Lit", &modeIndex, static_cast<int>(EViewModeIndex::VMI_Lit)); ImGui::SameLine();
	ImGui::RadioButton("Unlit", &modeIndex, static_cast<int>(EViewModeIndex::VMI_Unlit)); ImGui::SameLine();
	ImGui::RadioButton("Wireframe", &modeIndex, static_cast<int>(EViewModeIndex::VMI_Wireframe));

	if (modeIndex != static_cast<int>(currentMode))
	{
		ULevelManager::GetInstance().GetEditor()->SetViewMode(static_cast<EViewModeIndex>(modeIndex));
	}
	ImGui::Separator();

	// --- Show Flag Selection ---
	struct FlagEntry
	{
		const char* Label;
		EEngineShowFlags Flag;
	};

	// 체크박스로 표시할 플래그 목록
	static const FlagEntry Entries[] = {
		{ "Primitives",    EEngineShowFlags::SF_Primitives },
		{ "BillboardText", EEngineShowFlags::SF_BillboardText },
	};

	uint64 ShowFlags = ULevelManager::GetInstance().GetCurrentLevel()->GetShowFlags();
	size_t Idx = 0;
	const size_t Count = std::size(Entries);
	for (auto& Entry : Entries)
	{
		uint64 Flag = static_cast<uint64>(Entry.Flag);
		bool bChecked = (ShowFlags & Flag) != 0;

		if (ImGui::Checkbox(Entry.Label, &bChecked))
		{
			if (bChecked)
				ShowFlags |= Flag;   // 비트 켜기
			else
				ShowFlags &= ~Flag;  // 비트 끄기
		}
		if (++Idx < Count) // 마지막이 아니면
			ImGui::SameLine();
	}
	ULevelManager::GetInstance().GetCurrentLevel()->SetShowFlags(ShowFlags);
	ImGui::Separator();

	// --- 옵션 체크박스들 ---
	ImGui::Checkbox("Show Details", &bShowDetails);
	ImGui::Separator();

	const TArray<TObjectPtr<AActor>>& LevelActors = CurrentLevel->GetLevelActors();

	if (LevelActors.empty())
	{
		ImGui::TextUnformatted("No Actors in Level");
		return;
	}

	// Actor 개수 표시
	ImGui::Text("Total Actors: %zu", LevelActors.size());
	ImGui::Spacing();

	// Actor 리스트를 스크롤 가능한 영역으로 표시
	if (ImGui::BeginChild("ActorList", ImVec2(0, -80), true))
	{
		for (int32 i = 0; i < static_cast<int32>(LevelActors.size()); ++i)
		{
			if (LevelActors[i])
			{
				RenderActorInfo(LevelActors[i], i);
			}
		}
	}
	ImGui::EndChild();

	// 하단 정보
	AActor* SelectedActor = CurrentLevel->GetSelectedActor();
	if (SelectedActor)
	{
		ImGui::Text("Selected: %s", SelectedActor->GetName().ToString().data());

		if (bShowDetails)
		{
			const FVector& Location = SelectedActor->GetActorLocation();
			const FVector& Rotation = SelectedActor->GetActorRotation();
			const FVector& Scale = SelectedActor->GetActorScale3D();

			ImGui::Text("Location: (%.2f, %.2f, %.2f)", Location.X, Location.Y, Location.Z);
			ImGui::Text("Rotation: (%.2f, %.2f, %.2f)", Rotation.X, Rotation.Y, Rotation.Z);
			ImGui::Text("Scale: (%.2f, %.2f, %.2f)", Scale.X, Scale.Y, Scale.Z);
		}
	}
	else
	{
		ImGui::TextUnformatted("No Actor Selected");
	}
}

/**
 * @brief Actor 정보를 렌더링하는 헬퍼 함수
 * TODO(KHJ): 컴포넌트 정보 탐색을 위한 트리 노드를 작업 후 남겨두었음, 필요하다면 사용할 것
 * @param InActor 렌더링할 Actor
 * @param InIndex Actor의 인덱스
 */
void USceneHierarchyWidget::RenderActorInfo(AActor* InActor, int32 InIndex)
{
	if (!InActor)
	{
		return;
	}

	// 현재 선택된 Actor인지 확인
	ULevel* CurrentLevel = ULevelManager::GetInstance().GetCurrentLevel();
	bool bIsSelected = (CurrentLevel && CurrentLevel->GetSelectedActor() == InActor);

	// 선택된 Actor는 하이라이트
	if (bIsSelected)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // 노란색
	}

	FName ActorName = InActor->GetName();
	FString ActorDisplayName = ActorName.ToString() + " [" + std::to_string(InIndex) + "]";

	if (ImGui::Selectable(ActorDisplayName.data(), bIsSelected))
	{
		SelectActor(InActor);
	}

	// 트리 노드로 표시 (접을 수 있도록)
	// ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
	// if (bIsSelected)
	// {
	// 	NodeFlags |= ImGuiTreeNodeFlags_Selected;
	// }

	// bool bNodeOpen = ImGui::TreeNodeEx(ActorDisplayName.c_str(), NodeFlags);

	// 클릭 감지
	// if (ImGui::IsItemClicked())
	// {
	// 	SelectActor(InActor);
	// }

	// if (bNodeOpen)
	// {
	// if (bShowDetails)
	// {
	// Component 정보 표시
	// 	const TArray<UActorComponent*>& Components = InActor->GetOwnedComponents();
	// 	ImGui::Text("  Components: %zu", Components.size());
	// }
	//
	// 	ImGui::TreePop();
	// }

	if (bIsSelected)
	{
		ImGui::PopStyleColor();
	}
}

/**
 * @brief Actor를 선택하는 함수
 * @param InActor 선택할 Actor
 */
void USceneHierarchyWidget::SelectActor(AActor* InActor)
{
	ULevel* CurrentLevel = ULevelManager::GetInstance().GetCurrentLevel();
	if (CurrentLevel)
	{
		CurrentLevel->SetSelectedActor(InActor);
		UE_LOG("SceneHierarchy: '%s' 를 선택했습니다", InActor->GetName().ToString().data());

		if (InActor)
		{
			FocusOnActor(InActor);
		}
	}
}

/**
 * @brief 카메라를 특정 Actor에 포커스하는 함수
 * @param InActor 포커스할 Actor
 */
void USceneHierarchyWidget::FocusOnActor(const AActor* InActor) const
{
	if (!Camera || !InActor)
	{
		return;
	}

	// Actor의 월드 위치를 얻음
	FVector ActorLocation = InActor->GetActorLocation();

	// 카메라를 Actor로부터 적당한 거리에 배치
	// 기본적으로 Actor 뒤쪽에서 약간 위에서 바라보도록 설정
	FVector CameraOffset = FVector(-FOCUS_DISTANCE, 0.0f, FOCUS_HEIGHT_OFFSET);
	FVector NewCameraLocation = ActorLocation + CameraOffset;

	// 카메라가 Actor를 바라보도록 회전 계산
	FVector DirectionToActor = ActorLocation - NewCameraLocation;
	DirectionToActor.Normalize();

	// 엔진 좌표계에 맞는 회전 계산
	// Z축이 Forward이므로 DirectionToActor가 Z축 양의 방향을 가리키도록 회전 계산

	// Yaw 계산 (Y축 회전, 좌우)
	// X-Z 평면에서의 각도 계산
	float Yaw = FVector::GetRadianToDegree(atan2f(DirectionToActor.X, DirectionToActor.Z));

	// Pitch 계산 (X축 회전, 상하)
	// Y축 성분을 이용하여 상하 각도 계산
	float HorizontalLength = sqrtf(DirectionToActor.X * DirectionToActor.X + DirectionToActor.Z * DirectionToActor.Z);
	float Pitch = FVector::GetRadianToDegree(atan2f(-DirectionToActor.Y, HorizontalLength));

	// Roll은 0으로 설정
	float Roll = 0.0f;

	// 카메라 위치 및 회전 설정
	Camera->SetLocation(NewCameraLocation);
	Camera->SetRotation(FVector(Pitch, Yaw, Roll));

	UE_LOG("SceneHierarchy: 카메라를 '%s' 에 포커싱합니다", InActor->GetName().ToString().data());
}
