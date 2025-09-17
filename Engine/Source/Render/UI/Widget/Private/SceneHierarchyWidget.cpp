#include "pch.h"
#include "Render/UI/Widget/Public/SceneHierarchyWidget.h"

#include "Manager/Level/Public/LevelManager.h"
#include "Level/Public/Level.h"
#include "Actor/Public/Actor.h"
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
	// 카메라 애니메이션 업데이트
	if (bIsCameraAnimating)
	{
		UpdateCameraAnimation();
	}
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

	ImGui::RadioButton("Lit", &modeIndex, static_cast<int>(EViewModeIndex::VMI_Lit));
	ImGui::SameLine();
	ImGui::RadioButton("Unlit", &modeIndex, static_cast<int>(EViewModeIndex::VMI_Unlit));
	ImGui::SameLine();
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
		{"Primitives", EEngineShowFlags::SF_Primitives},
		{"BillboardText", EEngineShowFlags::SF_BillboardText},
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
			{
				ShowFlags |= Flag;
			}
			else
			{
				ShowFlags &= ~Flag;
			}
		}

		// 마지막이 아니면
		if (++Idx < Count)
		{
			ImGui::SameLine();
		}
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
void USceneHierarchyWidget::RenderActorInfo(TObjectPtr<AActor> InActor, int32 InIndex)
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

	// 싱글 클릭: 선택만 수행
	if (ImGui::Selectable(ActorDisplayName.data(), bIsSelected))
	{
		SelectActor(InActor, false);
	}

	// 더블 클릭 감지: 카메라 이동 수행
	if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
	{
		SelectActor(InActor, true);
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
 * @param bFocusCamera 카메라 포커싱 여부 (더블 클릭 시 true)
 */
void USceneHierarchyWidget::SelectActor(TObjectPtr<AActor> InActor, bool bFocusCamera)
{
	ULevel* CurrentLevel = ULevelManager::GetInstance().GetCurrentLevel();
	if (CurrentLevel)
	{
		CurrentLevel->SetSelectedActor(InActor);
		UE_LOG("SceneHierarchy: %s를 선택했습니다", InActor->GetName().ToString().data());

		// 카메라 포커싱은 더블 클릭에서만 수행
		if (InActor && bFocusCamera)
		{
			FocusOnActor(InActor);
			UE_LOG("SceneHierarchy: %s에 카메라 포커싱 (더블 클릭)", InActor->GetName().ToString().data());
		}
	}
}

/**
 * @brief 카메라를 특정 Actor에 포커스하는 함수
 * @param InActor 포커스할 Actor
 */
void USceneHierarchyWidget::FocusOnActor(TObjectPtr<AActor> InActor)
{
	if (!Camera || !InActor)
	{
		return;
	}

	// 현재 카메라의 위치와 회전을 저장
	CameraStartLocation = Camera->GetLocation();
	CameraCurrentRotation = Camera->GetRotation();

	// Actor의 월드 위치를 얻음
	FVector ActorLocation = InActor->GetActorLocation();

	// 카메라의 정확한 Forward 벡터를 사용하여 화면 중앙 배치 보정
	// Camera 클래스에서 이미 계산된 정확한 Forward 벡터 사용
	FVector CameraForward = Camera->GetForward();

	// Actor를 정확히 화면 중앙에 놓기 위해 Forward 방향의 반대로 거리를 둔 위치에 카메라 배치
	// 이렇게 하면 카메라 회전 유지 상태에서 Actor가 정확히 화면 중심에 위치함
	CameraTargetLocation = ActorLocation - (CameraForward * FOCUS_DISTANCE);

	// 카메라 애니메이션 시작
	bIsCameraAnimating = true;
	CameraAnimationTime = 0.0f;

	UE_LOG("SceneHierarchy: 카메라를 %s에 포커싱합니다", InActor->GetName().ToString().data());
}

/**
 * @brief 카메라 애니메이션을 업데이트하는 함수
 * 선형 보간을 활용한 부드러운 움직임을 구현함
 */
void USceneHierarchyWidget::UpdateCameraAnimation()
{
	if (!bIsCameraAnimating || !Camera)
	{
		return;
	}

	CameraAnimationTime += DT;

	// 애니메이션 진행 비율 계산
	float Progress = CameraAnimationTime / CAMERA_ANIMATION_DURATION;

	if (Progress >= 1.0f)
	{
		// 애니메이션 완료
		Progress = 1.0f;
		bIsCameraAnimating = false;
	}

	// Easing 함수를 사용하여 부드러운 움직임 확보 (easeInOutQuart)
	float SmoothProgress;
	if (Progress < 0.5f)
	{
		// 움직인 전반에서는 아주 천천히 가속
		SmoothProgress = 8.0f * Progress * Progress * Progress * Progress;
	}
	else
	{
		// 움직인 후반에서는 아주 천천히 감속
		float ProgressFromEnd = Progress - 1.0f;
		SmoothProgress = 1.0f - 8.0f * ProgressFromEnd * ProgressFromEnd * ProgressFromEnd * ProgressFromEnd;
	}

	// Linear interpolation으로 위치 보간
	FVector CurrentLocation = CameraStartLocation + (CameraTargetLocation - CameraStartLocation) * SmoothProgress;

	// 카메라 위치 설정
	// 의도가 카메라의 위치만 옮겨서 화면 중앙에 오브젝트를 두는 것이었기 때문에 Rotation은 처리하지 않음
	Camera->SetLocation(CurrentLocation);

	if (!bIsCameraAnimating)
	{
		UE_LOG("SceneHierarchy: 카메라 포커싱 애니메이션 완료");
	}
}
