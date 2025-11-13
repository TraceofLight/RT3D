#include "pch.h"
#include "Render/UI/Widget/Public/SkeletalMeshComponentWidget.h"
#include "Component/Mesh/Public/SkeletalMeshComponent.h"
#include "Actor/Public/DirectionalLight.h"
#include "Component/Mesh/Public/SkeletalMesh.h"

#include "Level/Public/Level.h"
#include "Level/Public/World.h"
#include "Runtime/CoreUObject/Public/ObjectIterator.h"
#include "Texture/Public/Material.h"
#include "Texture/Public/Texture.h"
#include "Editor/Public/EditorEngine.h"
#include "Manager/UI/Public/UIManager.h"
#include "Render/UI/Factory/Public/UIWindowFactory.h"
#include "Render/UI/Window/Public/FbxViewportWindow.h"
#include "Render/UI/Window/Public/UIWindow.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Manager/Path/Public/PathManager.h"
#include "Editor/Public/Editor.h"
#include "Editor/Public/Gizmo.h"

IMPLEMENT_CLASS(USkeletalMeshComponentWidget, UWidget)

void USkeletalMeshComponentWidget::Initialize()
{
	if (!World)
	{
		World = GWorld;
	}

	// Preview 컨트롤용 아이콘 로드
	UAssetManager& AssetManager = UAssetManager::GetInstance();
	UPathManager& PathManager = UPathManager::GetInstance();
	FString IconBasePath = PathManager.GetAssetPath().string() + "\\Icon\\";

	IconSelect = AssetManager.LoadTexture((IconBasePath + "Select.png").data());
	IconTranslate = AssetManager.LoadTexture((IconBasePath + "Translate.png").data());
	IconRotate = AssetManager.LoadTexture((IconBasePath + "Rotate.png").data());
	IconScale = AssetManager.LoadTexture((IconBasePath + "Scale.png").data());
	IconCamera = AssetManager.LoadTexture((IconBasePath + "Camera.png").data());
}

void USkeletalMeshComponentWidget::SetTargetWorld(UWorld* InWorld)
{
	World = InWorld ? InWorld : GWorld;
}

void USkeletalMeshComponentWidget::SetTargetComponent(USkeletalMeshComponent* InComponent)
{
	OverrideTargetComponent = InComponent;
}

UDirectionalLightComponent* USkeletalMeshComponentWidget::FindFirstDirectional(UWorld* TargetWorld) const
{
	if (!TargetWorld || !TargetWorld->GetLevel()) return nullptr;

	for (auto* LightComp : TargetWorld->GetLevel()->GetLightComponents())
	{
		if (auto* Dir = Cast<UDirectionalLightComponent>(LightComp)) {
			return Dir;
		}
	}
	return nullptr;
}

void USkeletalMeshComponentWidget::RenderWidget()
{
	UWorld* TargetWorld = World ? World : GWorld;
	if (!TargetWorld)
	{
		ImGui::TextUnformatted("No World");
		return;
	}

	ULevel* CurrentLevel = TargetWorld->GetLevel();

	if (!CurrentLevel)
	{
		ImGui::TextUnformatted("No Level Loaded");
		return;
	}

	UActorComponent* Component = GEditor->GetEditorModule()->GetSelectedComponent();
	USkeletalMeshComponent* TargetComponent = OverrideTargetComponent ? OverrideTargetComponent : Cast<USkeletalMeshComponent>(Component);
	if (!TargetComponent)
	{
		ImGui::TextUnformatted("No SkeletalMeshComponent");
		return;
	}

	SkeletalMeshComponent = TargetComponent;

	// 모든 입력 필드를 검은색으로 설정
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

	RenderPreviewTopControls(TargetWorld, TargetComponent);

	RenderSkeletalMeshSelector();

	// Mesh가 할당된 경우에만 Material Section 표시
	if (SkeletalMeshComponent->GetSkeletalMesh())
	{
		ImGui::Separator();
		RenderMaterialSections();
	}


	ImGui::PopStyleColor(5);
}

void USkeletalMeshComponentWidget::RenderSkeletalMeshSelector()
{
	USkeletalMesh* CurrentSkeletalMesh = SkeletalMeshComponent->GetSkeletalMesh();
	FString PreviewName = "None";

	if (CurrentSkeletalMesh && CurrentSkeletalMesh->GetSkeletalMeshAsset())
	{
		PreviewName = CurrentSkeletalMesh->GetSkeletalMeshAsset()->PathFileNameString;
	}

	// Detail 패널과 동일한 검은색 스타일
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

	if (ImGui::BeginCombo("Skeletal Mesh", PreviewName.c_str()))
	{
		for (TObjectIterator<USkeletalMesh> It; It; ++It)
		{
			USkeletalMesh* MeshInList = *It;
			if (!MeshInList || !MeshInList->IsValid()) continue;

			FString MeshName = MeshInList->GetSkeletalMeshAsset()->PathFileNameString;
			const bool bIsSelected = (CurrentSkeletalMesh == MeshInList);

			if (ImGui::Selectable(MeshName.c_str(), bIsSelected))
			{
				SkeletalMeshComponent->SetSkeletalMesh(MeshInList);
			}

			if (bIsSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	ImGui::PopStyleColor(3);

	const bool bHasMesh = (SkeletalMeshComponent->GetSkeletalMesh() != nullptr);
	if (!bHasMesh)
	{
		ImGui::BeginDisabled();
	}
	if (ImGui::Button("Open FBX Preview"))
	{
		OpenFbxPreviewViewport(SkeletalMeshComponent->GetSkeletalMesh());
	}
	if (!bHasMesh)
	{
		ImGui::EndDisabled();
	}
}

void USkeletalMeshComponentWidget::RenderMaterialSections()
{
	USkeletalMesh* CurrentMesh = SkeletalMeshComponent->GetSkeletalMesh();
	if (!CurrentMesh || !CurrentMesh->IsValid())
	{
		return;
	}

	FSkeletalMesh* MeshAsset = CurrentMesh->GetSkeletalMeshAsset();
	if (!MeshAsset)
	{
		return;
	}

	ImGui::Text("Material Slots (%d)", static_cast<int>(MeshAsset->MaterialInfo.Num()));

	for (int32 SlotIndex = 0; SlotIndex < MeshAsset->MaterialInfo.Num(); ++SlotIndex)
	{
		UMaterial* CurrentMaterial = SkeletalMeshComponent->GetMaterial(SlotIndex);
		FString PreviewName = CurrentMaterial ? GetMaterialDisplayName(CurrentMaterial) : "None";

		ImGui::PushID(SlotIndex);

		std::string Label = "Element " + std::to_string(SlotIndex);
		ImGui::TextUnformatted(Label.c_str());

		const float PreviewSize = 64.0f;
		UTexture* MaterialPreviewTexture = GetPreviewTextureForMaterial(CurrentMaterial);
		ID3D11ShaderResourceView* ShaderResourceView = nullptr;
		if (MaterialPreviewTexture != nullptr)
		{
			ShaderResourceView = MaterialPreviewTexture->GetTextureSRV();
		}

		if (ShaderResourceView != nullptr)
		{
			ImGui::Image((ImTextureID)ShaderResourceView, ImVec2(PreviewSize, PreviewSize), ImVec2(0, 0), ImVec2(1, 1));
		}
		else
		{
			ImGui::Dummy(ImVec2(PreviewSize, PreviewSize));
		}

		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

		// Detail 패널과 동일한 검은색 스타일
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

		std::string ComboId = "##MaterialSlotCombo_" + std::to_string(SlotIndex);
		if (ImGui::BeginCombo(ComboId.c_str(), PreviewName.c_str()))
		{
			RenderAvailableMaterials(SlotIndex);
			ImGui::EndCombo();
		}

		ImGui::PopStyleColor(3);

		// Color pickers
		auto RenderColorPicker = [](const char* Label, FVector& Color, UMaterial* Material, void (UMaterial::*SetColor)(const FVector&)) {
			float ColorRGB[3] = { Color.X * 255.0f, Color.Y * 255.0f, Color.Z * 255.0f };
			bool ColorChanged = false;
			ImDrawList* DrawList = ImGui::GetWindowDrawList();
			float BoxWidth = 65.0f;

			ImGui::SetNextItemWidth(BoxWidth);
			ImVec2 PosR = ImGui::GetCursorScreenPos();
			std::string IDR = std::string("##") + Label + "R";
			ColorChanged |= ImGui::DragFloat(IDR.c_str(), &ColorRGB[0], 1.0f, 0.0f, 255.0f, "R: %.0f");
			ImVec2 SizeR = ImGui::GetItemRectSize();
			DrawList->AddLine(ImVec2(PosR.x + 5, PosR.y + 2), ImVec2(PosR.x + 5, PosR.y + SizeR.y - 2), IM_COL32(255, 0, 0, 255), 2.0f);
			ImGui::SameLine();

			ImGui::SetNextItemWidth(BoxWidth);
			ImVec2 PosG = ImGui::GetCursorScreenPos();
			std::string IDG = std::string("##") + Label + "G";
			ColorChanged |= ImGui::DragFloat(IDG.c_str(), &ColorRGB[1], 1.0f, 0.0f, 255.0f, "G: %.0f");
			ImVec2 SizeG = ImGui::GetItemRectSize();
			DrawList->AddLine(ImVec2(PosG.x + 5, PosG.y + 2), ImVec2(PosG.x + 5, PosG.y + SizeG.y - 2), IM_COL32(0, 255, 0, 255), 2.0f);
			ImGui::SameLine();

			ImGui::SetNextItemWidth(BoxWidth);
			ImVec2 PosB = ImGui::GetCursorScreenPos();
			std::string IDB = std::string("##") + Label + "B";
			ColorChanged |= ImGui::DragFloat(IDB.c_str(), &ColorRGB[2], 1.0f, 0.0f, 255.0f, "B: %.0f");
			ImVec2 SizeB = ImGui::GetItemRectSize();
			DrawList->AddLine(ImVec2(PosB.x + 5, PosB.y + 2), ImVec2(PosB.x + 5, PosB.y + SizeB.y - 2), IM_COL32(0, 0, 255, 255), 2.0f);
			ImGui::SameLine();

			float Color01[3] = { ColorRGB[0] / 255.0f, ColorRGB[1] / 255.0f, ColorRGB[2] / 255.0f };
			if (ImGui::ColorEdit3(Label, Color01, ImGuiColorEditFlags_NoInputs))
			{
				ColorRGB[0] = Color01[0] * 255.0f;
				ColorRGB[1] = Color01[1] * 255.0f;
				ColorRGB[2] = Color01[2] * 255.0f;
				ColorChanged = true;
			}

			if (ColorChanged)
			{
				Color.X = ColorRGB[0] / 255.0f;
				Color.Y = ColorRGB[1] / 255.0f;
				Color.Z = ColorRGB[2] / 255.0f;
				(Material->*SetColor)(Color);
			}
		};

		if (CurrentMaterial)
		{
			FVector Ambient = CurrentMaterial->GetAmbientColor();
			FVector Diffuse = CurrentMaterial->GetDiffuseColor();
			FVector Specular = CurrentMaterial->GetSpecularColor();

			RenderColorPicker("Ambient", Ambient, CurrentMaterial, &UMaterial::SetAmbientColor);
			RenderColorPicker("Diffuse", Diffuse, CurrentMaterial, &UMaterial::SetDiffuseColor);
			RenderColorPicker("Specular", Specular, CurrentMaterial, &UMaterial::SetSpecularColor);
		}

		ImGui::PopID();
	}
}

void USkeletalMeshComponentWidget::RenderAvailableMaterials(int32 TargetSlotIndex) const
{
	for (TObjectIterator<UMaterial> Iter; Iter; ++Iter)
	{
		UMaterial* Material = *Iter;
		if (!Material)
		{
			continue;
		}

		FString MaterialName = GetMaterialDisplayName(Material);
		bool bIsSelected = (SkeletalMeshComponent->GetMaterial(TargetSlotIndex) == Material);

		constexpr float RowPreviewSize = 20.0f;
		UTexture* RowPreviewTexture = GetPreviewTextureForMaterial(Material);
		ID3D11ShaderResourceView* RowShaderResourceView = nullptr;
		if (RowPreviewTexture != nullptr)
		{
			RowShaderResourceView = RowPreviewTexture->GetTextureSRV();
		}

		if (RowShaderResourceView != nullptr)
		{
			ImGui::Image(RowShaderResourceView, ImVec2(RowPreviewSize, RowPreviewSize), ImVec2(0, 0), ImVec2(1, 1));
		}
		else
		{
			ImGui::Dummy(ImVec2(RowPreviewSize, RowPreviewSize));
		}
		ImGui::SameLine();

		if (ImGui::Selectable(MaterialName.c_str(), bIsSelected))
		{
			SkeletalMeshComponent->SetMaterial(TargetSlotIndex, Material);
		}

		if (bIsSelected)
		{
			ImGui::SetItemDefaultFocus();
		}
	}
}

void USkeletalMeshComponentWidget::RenderPreviewTopControls(UWorld* TargetWorld,
	USkeletalMeshComponent* TargetComponent)
{
	if (!TargetWorld || !TargetComponent || TargetWorld->GetWorldType() != EWorldType::EditorPreview) return;

	// Detail 패널과 동일한 검은색 스타일
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));

	// CollapsingHeader 너비 제한
	ImGui::PushItemWidth(250.0f);
	if (ImGui::CollapsingHeader("Preview Controls", ImGuiTreeNodeFlags_DefaultOpen))
	{
        // 1) Directional Light 회전
        if (UDirectionalLightComponent* Dir = FindFirstDirectional(TargetWorld))
        {
            FVector euler = Dir->GetRelativeRotation().ToEuler(); // Pitch, Yaw, Roll
            float pitch = euler.X, yaw = euler.Y, roll = euler.Z;

            ImGui::TextUnformatted("Directional Light");
            bool changed = false;

            ImGui::SetNextItemWidth(200.0f);
            changed |= ImGui::DragFloat("Pitch", &pitch, 0.2f, -89.9f, 89.9f, "%.1f deg");
            ImGui::SetNextItemWidth(200.0f);
            changed |= ImGui::DragFloat("Yaw",   &yaw,   0.2f, -360.f, 360.f, "%.1f deg");
            ImGui::SetNextItemWidth(200.0f);
            changed |= ImGui::DragFloat("Roll",  &roll,  0.2f, -360.f, 360.f, "%.1f deg");
            if (changed)
            {
                Dir->SetRelativeRotation(FQuat::FromEuler(FVector(pitch, yaw, roll)));
            }
        	float Intensity = Dir->GetIntensity();
        	ImGui::SetNextItemWidth(200.0f);
        	if (ImGui::DragFloat("Intensity", &Intensity, 100.0f, 0.0f, FLT_MAX))
        	{
        		Dir->SetIntensity(Intensity);
        	}
        	if (ImGui::IsItemHovered())
        	{
        		ImGui::SetTooltip("디렉셔널 라이트 밝기 (Lux)\n범위: 0.0 ~ 무제한\n참고: 실외 태양광 약 100000 lux");
        	}
        }
        else
        {
            ImGui::TextDisabled("No DirectionalLight found in preview world");
        }

        ImGui::Separator();

        // 2) Skeletal Transform (Viewport 스타일과 일치)
        {
            ImGui::TextUnformatted("SkeletalMesh Transform");
            ImDrawList* DrawList = ImGui::GetWindowDrawList();

            FVector Location = TargetComponent->GetRelativeLocation();
            FVector Rotation = TargetComponent->GetRelativeRotation().ToEuler();
            FVector Scale = TargetComponent->GetRelativeScale3D();

            float LocArray[3] = {Location.X, Location.Y, Location.Z};
            float RotArray[3] = {Rotation.X, Rotation.Y, Rotation.Z};
            float ScaleArray[3] = {Scale.X, Scale.Y, Scale.Z};

            bool LocChanged = false, RotChanged = false, ScaleChanged = false;

            // Location
            ImGui::Text("Location");
            ImGui::SameLine(100.0f);
            ImVec2 Pos;
            ImVec2 Size;

            Pos = ImGui::GetCursorScreenPos();
            ImGui::SetNextItemWidth(65.0f);
            LocChanged |= ImGui::DragFloat("##PrevLocX", &LocArray[0], 0.5f);
            Size = ImGui::GetItemRectSize();
            DrawList->AddLine(ImVec2(Pos.x + 5, Pos.y + 2), ImVec2(Pos.x + 5, Pos.y + Size.y - 2), IM_COL32(255, 0, 0, 255), 2.0f);
            ImGui::SameLine();

            Pos = ImGui::GetCursorScreenPos();
            ImGui::SetNextItemWidth(65.0f);
            LocChanged |= ImGui::DragFloat("##PrevLocY", &LocArray[1], 0.5f);
            Size = ImGui::GetItemRectSize();
            DrawList->AddLine(ImVec2(Pos.x + 5, Pos.y + 2), ImVec2(Pos.x + 5, Pos.y + Size.y - 2), IM_COL32(0, 255, 0, 255), 2.0f);
            ImGui::SameLine();

            Pos = ImGui::GetCursorScreenPos();
            ImGui::SetNextItemWidth(65.0f);
            LocChanged |= ImGui::DragFloat("##PrevLocZ", &LocArray[2], 0.5f);
            Size = ImGui::GetItemRectSize();
            DrawList->AddLine(ImVec2(Pos.x + 5, Pos.y + 2), ImVec2(Pos.x + 5, Pos.y + Size.y - 2), IM_COL32(0, 0, 255, 255), 2.0f);

            if (LocChanged)
            {
                TargetComponent->SetRelativeLocation({LocArray[0], LocArray[1], LocArray[2]});
            }

            // Rotation
            ImGui::Text("Rotation");
            ImGui::SameLine(100.0f);

            Pos = ImGui::GetCursorScreenPos();
            ImGui::SetNextItemWidth(65.0f);
            RotChanged |= ImGui::DragFloat("##PrevRotX", &RotArray[0], 0.5f);
            Size = ImGui::GetItemRectSize();
            DrawList->AddLine(ImVec2(Pos.x + 5, Pos.y + 2), ImVec2(Pos.x + 5, Pos.y + Size.y - 2), IM_COL32(255, 0, 0, 255), 2.0f);
            ImGui::SameLine();

            Pos = ImGui::GetCursorScreenPos();
            ImGui::SetNextItemWidth(65.0f);
            RotChanged |= ImGui::DragFloat("##PrevRotY", &RotArray[1], 0.5f);
            Size = ImGui::GetItemRectSize();
            DrawList->AddLine(ImVec2(Pos.x + 5, Pos.y + 2), ImVec2(Pos.x + 5, Pos.y + Size.y - 2), IM_COL32(0, 255, 0, 255), 2.0f);
            ImGui::SameLine();

            Pos = ImGui::GetCursorScreenPos();
            ImGui::SetNextItemWidth(65.0f);
            RotChanged |= ImGui::DragFloat("##PrevRotZ", &RotArray[2], 0.5f);
            Size = ImGui::GetItemRectSize();
            DrawList->AddLine(ImVec2(Pos.x + 5, Pos.y + 2), ImVec2(Pos.x + 5, Pos.y + Size.y - 2), IM_COL32(0, 0, 255, 255), 2.0f);

            if (RotChanged)
            {
                TargetComponent->SetRelativeRotation(FQuat::FromEuler({RotArray[0], RotArray[1], RotArray[2]}));
            }

            // Scale
            ImGui::Text("Scale");
            ImGui::SameLine(100.0f);

            Pos = ImGui::GetCursorScreenPos();
            ImGui::SetNextItemWidth(65.0f);
            ScaleChanged |= ImGui::DragFloat("##PrevScaleX", &ScaleArray[0], 0.01f, 0.001f, 100.0f);
            Size = ImGui::GetItemRectSize();
            DrawList->AddLine(ImVec2(Pos.x + 5, Pos.y + 2), ImVec2(Pos.x + 5, Pos.y + Size.y - 2), IM_COL32(255, 0, 0, 255), 2.0f);
            ImGui::SameLine();

            Pos = ImGui::GetCursorScreenPos();
            ImGui::SetNextItemWidth(65.0f);
            ScaleChanged |= ImGui::DragFloat("##PrevScaleY", &ScaleArray[1], 0.01f, 0.001f, 100.0f);
            Size = ImGui::GetItemRectSize();
            DrawList->AddLine(ImVec2(Pos.x + 5, Pos.y + 2), ImVec2(Pos.x + 5, Pos.y + Size.y - 2), IM_COL32(0, 255, 0, 255), 2.0f);
            ImGui::SameLine();

            Pos = ImGui::GetCursorScreenPos();
            ImGui::SetNextItemWidth(65.0f);
            ScaleChanged |= ImGui::DragFloat("##PrevScaleZ", &ScaleArray[2], 0.01f, 0.001f, 100.0f);
            Size = ImGui::GetItemRectSize();
            DrawList->AddLine(ImVec2(Pos.x + 5, Pos.y + 2), ImVec2(Pos.x + 5, Pos.y + Size.y - 2), IM_COL32(0, 0, 255, 255), 2.0f);

            if (ScaleChanged)
            {
                TargetComponent->SetRelativeScale3D({ScaleArray[0], ScaleArray[1], ScaleArray[2]});
            }
        }
    }
	ImGui::PopItemWidth();

	ImGui::PopStyleColor(7);
}

int32 USkeletalMeshComponentWidget::CountAllDescendants(FSkeleton* Skeleton, int32 BoneIndex)
{
	if (BoneIndex >= Skeleton->Childs.Num())
	{
		return 0;
	}

	int32 Count = 0;
	for (int32 ChildIdx : Skeleton->Childs[BoneIndex])
	{
		Count++; // 직접 자식
		Count += CountAllDescendants(Skeleton, ChildIdx); // 재귀적으로 하위 본들
	}
	return Count;
}

bool USkeletalMeshComponentWidget::IsAncestorOf(FSkeleton* Skeleton, int32 AncestorIndex, int32 DescendantIndex)
{
	if (AncestorIndex < 0 || DescendantIndex < 0 || AncestorIndex >= Skeleton->Childs.Num())
	{
		return false;
	}

	// 직접 자식인지 확인
	for (int32 ChildIdx : Skeleton->Childs[AncestorIndex])
	{
		if (ChildIdx == DescendantIndex)
		{
			return true;
		}
		// 재귀적으로 하위 본 확인
		if (IsAncestorOf(Skeleton, ChildIdx, DescendantIndex))
		{
			return true;
		}
	}
	return false;
}

void USkeletalMeshComponentWidget::DrawSkeletalBone(FSkeleton* Skeleton, int idx)
{
	ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

	// 전체 하위 본 개수 계산 (재귀적으로 모든 자손 포함)
	int32 NumDescendants = CountAllDescendants(Skeleton, idx);

	bool bHasChild = (NumDescendants > 0);
	if (!bHasChild)
	{
		NodeFlags |= ImGuiTreeNodeFlags_Leaf;
	}

	// FbxViewportWindow의 선택 상태 확인
	int32 CurrentSelectedBone = OwningFbxViewportWindow ? OwningFbxViewportWindow->GetSelectedBoneIndex() : -1;
	if (CurrentSelectedBone == idx)
	{
		NodeFlags |= ImGuiTreeNodeFlags_Selected;
	}

	// 선택된 Bone의 부모 경로인 경우 자동으로 열기
	if (CurrentSelectedBone >= 0 && IsAncestorOf(Skeleton, idx, CurrentSelectedBone))
	{
		ImGui::SetNextItemOpen(true);
	}

	// Bone 이름 + 전체 하위 본 개수 표시
	FString NodeLabel = Skeleton->BoneNames[idx].ToString();
	if (bHasChild)
	{
		NodeLabel += " (" + std::to_string(NumDescendants) + ")";
	}

	bool bTreeNodeOpen = ImGui::TreeNodeEx(NodeLabel.c_str(), NodeFlags);

	// TreeNodeEx 직후에 클릭 체크 (드랍다운뿐만 아니라 항목 전체 클릭 가능)
	if (ImGui::IsItemClicked())
	{
		// FbxViewportWindow 양방향 연동
		if (OwningFbxViewportWindow)
		{
			OwningFbxViewportWindow->SelectBone(idx);
			OwningFbxViewportWindow->SetEditMode(EEditMode::BoneEdit);
		}
	}

	if (bTreeNodeOpen)
	{
		for (int ChildIdx : Skeleton->Childs[idx])
		{
			DrawSkeletalBone(Skeleton, ChildIdx);
		}

		ImGui::TreePop();
	}
}
void USkeletalMeshComponentWidget::RenderBoneHierachy(USkeletalMeshComponent* SkeletalMeshComponent)
{
	FSkeleton* Skeleton = SkeletalMeshComponent->GetSkeletalMesh()->GetSkeletalMeshAsset()->Skeleton;
	DrawSkeletalBone(Skeleton, 0);
}

void USkeletalMeshComponentWidget::OpenFbxPreviewViewport(USkeletalMesh* SkeletalMesh)
{
	if (!SkeletalMesh)
	{
		return;
	}

	UUIManager& UIManager = UUIManager::GetInstance();
	const FName PreviewWindowName = FName("Preview");
	UFbxViewportWindow* PreviewWindow = nullptr;
	if (UUIWindow* Existing = UIManager.FindUIWindow(PreviewWindowName))
	{
		PreviewWindow = Cast<UFbxViewportWindow>(Existing);
	}

	if (!PreviewWindow)
	{
		PreviewWindow = UUIWindowFactory::CreateFbxViewportWindow(EUIDockDirection::None);
		if (!PreviewWindow)
		{
			UE_LOG_ERROR("SkeletalMeshComponentWidget: failed to allocate FBX viewport window.");
			return;
		}

		if (!UIManager.RegisterUIWindow(PreviewWindow))
		{
			UE_LOG_ERROR("SkeletalMeshComponentWidget: failed to register FBX viewport window.");
			return;
		}
	}

	PreviewWindow->SetPreviewSkeletalMesh(SkeletalMesh);
	PreviewWindow->SetWindowState(EUIWindowState::Visible);
	UIManager.SetFocusedWindow(PreviewWindow);
}

FString USkeletalMeshComponentWidget::GetMaterialDisplayName(UMaterial* Material)
{
	if (!Material)
	{
		return "None";
	}

	FString ObjectName = Material->GetName().ToString();
	if (!ObjectName.empty() && ObjectName.find("Object_") != 0)
	{
		return ObjectName;
	}

	UTexture* DiffuseTexture = Material->GetDiffuseTexture();
	if (DiffuseTexture)
	{
		FString TexturePath = DiffuseTexture->GetFilePath().ToString();
		if (!TexturePath.empty())
		{
			size_t LastSlash = TexturePath.find_last_of("/\\");
			size_t LastDot = TexturePath.find_last_of(".");

			if (LastSlash != std::string::npos)
			{
				FString FileName = TexturePath.substr(LastSlash + 1);
				if (LastDot != std::string::npos && LastDot > LastSlash)
				{
					FileName = FileName.substr(0, LastDot - LastSlash - 1);
				}
				return FileName + " (Mat)";
			}
		}
	}

	TArray<UTexture*> Textures = {
		Material->GetAmbientTexture(),
		Material->GetSpecularTexture(),
		Material->GetNormalTexture(),
		Material->GetOpacityTexture(),
		Material->GetBumpTexture()
	};

	for (UTexture* Texture : Textures)
	{
		if (Texture)
		{
			FString TexturePath = Texture->GetFilePath().ToString();
			if (!TexturePath.empty())
			{
				size_t LastSlash = TexturePath.find_last_of("/\\");
				size_t LastDot = TexturePath.find_last_of(".");

				if (LastSlash != std::string::npos)
				{
					FString FileName = TexturePath.substr(LastSlash + 1);
					if (LastDot != std::string::npos && LastDot > LastSlash)
					{
						FileName = FileName.substr(0, LastDot - LastSlash - 1);
					}
					return FileName + " (Mat)";
				}
			}
		}
	}

	return "Material_" + std::to_string(Material->GetUUID());
}

UTexture* USkeletalMeshComponentWidget::GetPreviewTextureForMaterial(const UMaterial* Material)
{
	if (Material == nullptr)
	{
		return nullptr;
	}

	UTexture* PreviewTexture = nullptr;

	PreviewTexture = Material->GetDiffuseTexture();
	if (PreviewTexture != nullptr)
	{
		return PreviewTexture;
	}

	PreviewTexture = Material->GetAmbientTexture();
	if (PreviewTexture != nullptr)
	{
		return PreviewTexture;
	}

	PreviewTexture = Material->GetSpecularTexture();
	if (PreviewTexture != nullptr)
	{
		return PreviewTexture;
	}

	PreviewTexture = Material->GetNormalTexture();
	if (PreviewTexture != nullptr)
	{
		return PreviewTexture;
	}

	PreviewTexture = Material->GetOpacityTexture();
	if (PreviewTexture != nullptr)
	{
		return PreviewTexture;
	}

	PreviewTexture = Material->GetBumpTexture();
	return PreviewTexture;
}

void USkeletalMeshComponentWidget::RenderComponentTransformEdit(USkeletalMeshComponent* Component)
{
	if (!Component)
	{
		return;
	}

	ImGui::Text("Component Transform");
	ImGui::Separator();

	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	// Drag 필드 색상 설정
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

	// Location
	FVector Location = Component->GetWorldLocation();
	ImGui::Text("Location");
	ImGui::SameLine(100.0f);

	float LocArray[3] = {Location.X, Location.Y, Location.Z};
	bool LocChanged = false;

	ImVec2 PosX = ImGui::GetCursorScreenPos();
	ImGui::SetNextItemWidth(75.0f);
	LocChanged |= ImGui::DragFloat("##CompLocX", &LocArray[0], 0.1f, 0.0f, 0.0f, "%.3f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("X: %.3f", LocArray[0]);
	}
	ImVec2 SizeX = ImGui::GetItemRectSize();
	DrawList->AddLine(ImVec2(PosX.x + 5, PosX.y + 2), ImVec2(PosX.x + 5, PosX.y + SizeX.y - 2), IM_COL32(255, 0, 0, 255), 2.0f);
	ImGui::SameLine();

	ImVec2 PosY = ImGui::GetCursorScreenPos();
	ImGui::SetNextItemWidth(75.0f);
	LocChanged |= ImGui::DragFloat("##CompLocY", &LocArray[1], 0.1f, 0.0f, 0.0f, "%.3f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Y: %.3f", LocArray[1]);
	}
	ImVec2 SizeY = ImGui::GetItemRectSize();
	DrawList->AddLine(ImVec2(PosY.x + 5, PosY.y + 2), ImVec2(PosY.x + 5, PosY.y + SizeY.y - 2), IM_COL32(0, 255, 0, 255), 2.0f);
	ImGui::SameLine();

	ImVec2 PosZ = ImGui::GetCursorScreenPos();
	ImGui::SetNextItemWidth(75.0f);
	LocChanged |= ImGui::DragFloat("##CompLocZ", &LocArray[2], 0.1f, 0.0f, 0.0f, "%.3f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Z: %.3f", LocArray[2]);
	}
	ImVec2 SizeZ = ImGui::GetItemRectSize();
	DrawList->AddLine(ImVec2(PosZ.x + 5, PosZ.y + 2), ImVec2(PosZ.x + 5, PosZ.y + SizeZ.y - 2), IM_COL32(0, 0, 255, 255), 2.0f);
	ImGui::SameLine();

	// Reset button
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
	if (ImGui::SmallButton(reinterpret_cast<const char*>(u8"↻##ResetCompLoc")))
	{
		LocArray[0] = LocArray[1] = LocArray[2] = 0.0f;
		LocChanged = true;
	}
	ImGui::PopStyleColor(3);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Reset to zero");
	}

	if (LocChanged)
	{
		Component->SetWorldLocation({LocArray[0], LocArray[1], LocArray[2]});
	}

	// Rotation
	FQuat RotQuat = Component->GetWorldRotationAsQuaternion();
	FVector EulerDeg = RotQuat.ToEuler();
	EulerDeg.X = FVector::GetRadianToDegree(EulerDeg.X);
	EulerDeg.Y = FVector::GetRadianToDegree(EulerDeg.Y);
	EulerDeg.Z = FVector::GetRadianToDegree(EulerDeg.Z);

	ImGui::Text("Rotation");
	ImGui::SameLine(100.0f);

	float RotArray[3] = {EulerDeg.X, EulerDeg.Y, EulerDeg.Z};
	bool RotChanged = false;

	PosX = ImGui::GetCursorScreenPos();
	ImGui::SetNextItemWidth(75.0f);
	RotChanged |= ImGui::DragFloat("##CompRotX", &RotArray[0], 1.0f, 0.0f, 0.0f, "%.3f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Roll: %.3f", RotArray[0]);
	}
	SizeX = ImGui::GetItemRectSize();
	DrawList->AddLine(ImVec2(PosX.x + 5, PosX.y + 2), ImVec2(PosX.x + 5, PosX.y + SizeX.y - 2), IM_COL32(255, 0, 0, 255), 2.0f);
	ImGui::SameLine();

	PosY = ImGui::GetCursorScreenPos();
	ImGui::SetNextItemWidth(75.0f);
	RotChanged |= ImGui::DragFloat("##CompRotY", &RotArray[1], 1.0f, 0.0f, 0.0f, "%.3f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Pitch: %.3f", RotArray[1]);
	}
	SizeY = ImGui::GetItemRectSize();
	DrawList->AddLine(ImVec2(PosY.x + 5, PosY.y + 2), ImVec2(PosY.x + 5, PosY.y + SizeY.y - 2), IM_COL32(0, 255, 0, 255), 2.0f);
	ImGui::SameLine();

	PosZ = ImGui::GetCursorScreenPos();
	ImGui::SetNextItemWidth(75.0f);
	RotChanged |= ImGui::DragFloat("##CompRotZ", &RotArray[2], 1.0f, 0.0f, 0.0f, "%.3f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Yaw: %.3f", RotArray[2]);
	}
	SizeZ = ImGui::GetItemRectSize();
	DrawList->AddLine(ImVec2(PosZ.x + 5, PosZ.y + 2), ImVec2(PosZ.x + 5, PosZ.y + SizeZ.y - 2), IM_COL32(0, 0, 255, 255), 2.0f);
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
	if (ImGui::SmallButton(reinterpret_cast<const char*>(u8"↻##ResetCompRot")))
	{
		RotArray[0] = RotArray[1] = RotArray[2] = 0.0f;
		RotChanged = true;
	}
	ImGui::PopStyleColor(3);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Reset to zero");
	}

	if (RotChanged)
	{
		FVector EulerRad;
		EulerRad.X = FVector::GetDegreeToRadian(RotArray[0]);
		EulerRad.Y = FVector::GetDegreeToRadian(RotArray[1]);
		EulerRad.Z = FVector::GetDegreeToRadian(RotArray[2]);
		Component->SetWorldRotation(FQuat::FromEuler(EulerRad));
	}

	// Scale
	FVector Scale = Component->GetWorldScale3D();
	ImGui::Text("Scale");
	ImGui::SameLine(100.0f);

	float ScaleArray[3] = {Scale.X, Scale.Y, Scale.Z};
	bool ScaleChanged = false;

	PosX = ImGui::GetCursorScreenPos();
	ImGui::SetNextItemWidth(75.0f);
	ScaleChanged |= ImGui::DragFloat("##CompScaleX", &ScaleArray[0], 0.01f, 0.0f, 0.0f, "%.3f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("X: %.3f", ScaleArray[0]);
	}
	SizeX = ImGui::GetItemRectSize();
	DrawList->AddLine(ImVec2(PosX.x + 5, PosX.y + 2), ImVec2(PosX.x + 5, PosX.y + SizeX.y - 2), IM_COL32(255, 0, 0, 255), 2.0f);
	ImGui::SameLine();

	PosY = ImGui::GetCursorScreenPos();
	ImGui::SetNextItemWidth(75.0f);
	ScaleChanged |= ImGui::DragFloat("##CompScaleY", &ScaleArray[1], 0.01f, 0.0f, 0.0f, "%.3f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Y: %.3f", ScaleArray[1]);
	}
	SizeY = ImGui::GetItemRectSize();
	DrawList->AddLine(ImVec2(PosY.x + 5, PosY.y + 2), ImVec2(PosY.x + 5, PosY.y + SizeY.y - 2), IM_COL32(0, 255, 0, 255), 2.0f);
	ImGui::SameLine();

	PosZ = ImGui::GetCursorScreenPos();
	ImGui::SetNextItemWidth(75.0f);
	ScaleChanged |= ImGui::DragFloat("##CompScaleZ", &ScaleArray[2], 0.01f, 0.0f, 0.0f, "%.3f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Z: %.3f", ScaleArray[2]);
	}
	SizeZ = ImGui::GetItemRectSize();
	DrawList->AddLine(ImVec2(PosZ.x + 5, PosZ.y + 2), ImVec2(PosZ.x + 5, PosZ.y + SizeZ.y - 2), IM_COL32(0, 0, 255, 255), 2.0f);
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
	if (ImGui::SmallButton(reinterpret_cast<const char*>(u8"↻##ResetCompScale")))
	{
		ScaleArray[0] = ScaleArray[1] = ScaleArray[2] = 1.0f;
		ScaleChanged = true;
	}
	ImGui::PopStyleColor(3);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Reset to one");
	}

	if (ScaleChanged)
	{
		Component->SetWorldScale3D({ScaleArray[0], ScaleArray[1], ScaleArray[2]});
	}

	ImGui::PopStyleColor(3);
}
void USkeletalMeshComponentWidget::RenderBoneTransformEdit(USkeletalMeshComponent* Component, int32 BoneIndex)
{
	if (!Component || BoneIndex < 0)
	{
		return;
	}

	USkeletalMesh* SkeletalMesh = Component->GetSkeletalMesh();
	if (!SkeletalMesh)
	{
		return;
	}

	FSkeleton* Skeleton = SkeletalMesh->GetSkeleton();
	if (!Skeleton || BoneIndex >= Skeleton->GetNumBones())
	{
		return;
	}

	ImGui::Text("Bone Transform");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.2f, 1.0f), "[%s]", Skeleton->BoneNames[BoneIndex].ToString().data());
	ImGui::Separator();

	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	// Drag 필드 색상 설정
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

	// Local Pose 가져오기
	FTransform& LocalPose = Component->GetLocalPose(BoneIndex);
	FVector Location = LocalPose.Location;
	FQuat Rotation = LocalPose.Rotation;
	FVector Scale = LocalPose.Scale;

	// Rotation을 Euler로 변환 (Degrees)
	FVector EulerDeg = Rotation.ToEuler();
	EulerDeg.X = FVector::GetRadianToDegree(EulerDeg.X);
	EulerDeg.Y = FVector::GetRadianToDegree(EulerDeg.Y);
	EulerDeg.Z = FVector::GetRadianToDegree(EulerDeg.Z);

	bool bTransformChanged = false;

	// Location
	ImGui::Text("Location");
	ImGui::SameLine(100.0f);

	float LocArray[3] = {Location.X, Location.Y, Location.Z};
	bool LocChanged = false;

	ImVec2 PosX = ImGui::GetCursorScreenPos();
	ImGui::SetNextItemWidth(75.0f);
	LocChanged |= ImGui::DragFloat("##BoneLocX", &LocArray[0], 0.1f, 0.0f, 0.0f, "%.3f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("X: %.3f", LocArray[0]);
	}
	ImVec2 SizeX = ImGui::GetItemRectSize();
	DrawList->AddLine(ImVec2(PosX.x + 5, PosX.y + 2), ImVec2(PosX.x + 5, PosX.y + SizeX.y - 2), IM_COL32(255, 0, 0, 255), 2.0f);
	ImGui::SameLine();

	ImVec2 PosY = ImGui::GetCursorScreenPos();
	ImGui::SetNextItemWidth(75.0f);
	LocChanged |= ImGui::DragFloat("##BoneLocY", &LocArray[1], 0.1f, 0.0f, 0.0f, "%.3f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Y: %.3f", LocArray[1]);
	}
	ImVec2 SizeY = ImGui::GetItemRectSize();
	DrawList->AddLine(ImVec2(PosY.x + 5, PosY.y + 2), ImVec2(PosY.x + 5, PosY.y + SizeY.y - 2), IM_COL32(0, 255, 0, 255), 2.0f);
	ImGui::SameLine();

	ImVec2 PosZ = ImGui::GetCursorScreenPos();
	ImGui::SetNextItemWidth(75.0f);
	LocChanged |= ImGui::DragFloat("##BoneLocZ", &LocArray[2], 0.1f, 0.0f, 0.0f, "%.3f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Z: %.3f", LocArray[2]);
	}
	ImVec2 SizeZ = ImGui::GetItemRectSize();
	DrawList->AddLine(ImVec2(PosZ.x + 5, PosZ.y + 2), ImVec2(PosZ.x + 5, PosZ.y + SizeZ.y - 2), IM_COL32(0, 0, 255, 255), 2.0f);
	ImGui::SameLine();

	// Reset button
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
	if (ImGui::SmallButton(reinterpret_cast<const char*>(u8"↻##ResetBoneLoc")))
	{
		LocArray[0] = LocArray[1] = LocArray[2] = 0.0f;
		LocChanged = true;
	}
	ImGui::PopStyleColor(3);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Reset to zero");
	}

	if (LocChanged)
	{
		Location = {LocArray[0], LocArray[1], LocArray[2]};
		bTransformChanged = true;
	}

	// Rotation
	ImGui::Text("Rotation");
	ImGui::SameLine(100.0f);

	float RotArray[3] = {EulerDeg.X, EulerDeg.Y, EulerDeg.Z};
	bool RotChanged = false;

	PosX = ImGui::GetCursorScreenPos();
	ImGui::SetNextItemWidth(75.0f);
	RotChanged |= ImGui::DragFloat("##BoneRotX", &RotArray[0], 1.0f, 0.0f, 0.0f, "%.3f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Roll: %.3f", RotArray[0]);
	}
	SizeX = ImGui::GetItemRectSize();
	DrawList->AddLine(ImVec2(PosX.x + 5, PosX.y + 2), ImVec2(PosX.x + 5, PosX.y + SizeX.y - 2), IM_COL32(255, 0, 0, 255), 2.0f);
	ImGui::SameLine();

	PosY = ImGui::GetCursorScreenPos();
	ImGui::SetNextItemWidth(75.0f);
	RotChanged |= ImGui::DragFloat("##BoneRotY", &RotArray[1], 1.0f, 0.0f, 0.0f, "%.3f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Pitch: %.3f", RotArray[1]);
	}
	SizeY = ImGui::GetItemRectSize();
	DrawList->AddLine(ImVec2(PosY.x + 5, PosY.y + 2), ImVec2(PosY.x + 5, PosY.y + SizeY.y - 2), IM_COL32(0, 255, 0, 255), 2.0f);
	ImGui::SameLine();

	PosZ = ImGui::GetCursorScreenPos();
	ImGui::SetNextItemWidth(75.0f);
	RotChanged |= ImGui::DragFloat("##BoneRotZ", &RotArray[2], 1.0f, 0.0f, 0.0f, "%.3f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Yaw: %.3f", RotArray[2]);
	}
	SizeZ = ImGui::GetItemRectSize();
	DrawList->AddLine(ImVec2(PosZ.x + 5, PosZ.y + 2), ImVec2(PosZ.x + 5, PosZ.y + SizeZ.y - 2), IM_COL32(0, 0, 255, 255), 2.0f);
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
	if (ImGui::SmallButton(reinterpret_cast<const char*>(u8"↻##ResetBoneRot")))
	{
		RotArray[0] = RotArray[1] = RotArray[2] = 0.0f;
		RotChanged = true;
	}
	ImGui::PopStyleColor(3);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Reset to zero");
	}

	if (RotChanged)
	{
		FVector EulerRad;
		EulerRad.X = FVector::GetDegreeToRadian(RotArray[0]);
		EulerRad.Y = FVector::GetDegreeToRadian(RotArray[1]);
		EulerRad.Z = FVector::GetDegreeToRadian(RotArray[2]);
		Rotation = FQuat::FromEuler(EulerRad);
		bTransformChanged = true;
	}

	// Scale
	ImGui::Text("Scale");
	ImGui::SameLine(100.0f);

	float ScaleArray[3] = {Scale.X, Scale.Y, Scale.Z};
	bool ScaleChanged = false;

	PosX = ImGui::GetCursorScreenPos();
	ImGui::SetNextItemWidth(75.0f);
	ScaleChanged |= ImGui::DragFloat("##BoneScaleX", &ScaleArray[0], 0.01f, 0.0f, 0.0f, "%.3f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("X: %.3f", ScaleArray[0]);
	}
	SizeX = ImGui::GetItemRectSize();
	DrawList->AddLine(ImVec2(PosX.x + 5, PosX.y + 2), ImVec2(PosX.x + 5, PosX.y + SizeX.y - 2), IM_COL32(255, 0, 0, 255), 2.0f);
	ImGui::SameLine();

	PosY = ImGui::GetCursorScreenPos();
	ImGui::SetNextItemWidth(75.0f);
	ScaleChanged |= ImGui::DragFloat("##BoneScaleY", &ScaleArray[1], 0.01f, 0.0f, 0.0f, "%.3f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Y: %.3f", ScaleArray[1]);
	}
	SizeY = ImGui::GetItemRectSize();
	DrawList->AddLine(ImVec2(PosY.x + 5, PosY.y + 2), ImVec2(PosY.x + 5, PosY.y + SizeY.y - 2), IM_COL32(0, 255, 0, 255), 2.0f);
	ImGui::SameLine();

	PosZ = ImGui::GetCursorScreenPos();
	ImGui::SetNextItemWidth(75.0f);
	ScaleChanged |= ImGui::DragFloat("##BoneScaleZ", &ScaleArray[2], 0.01f, 0.0f, 0.0f, "%.3f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Z: %.3f", ScaleArray[2]);
	}
	SizeZ = ImGui::GetItemRectSize();
	DrawList->AddLine(ImVec2(PosZ.x + 5, PosZ.y + 2), ImVec2(PosZ.x + 5, PosZ.y + SizeZ.y - 2), IM_COL32(0, 0, 255, 255), 2.0f);
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
	if (ImGui::SmallButton(reinterpret_cast<const char*>(u8"↻##ResetBoneScale")))
	{
		ScaleArray[0] = ScaleArray[1] = ScaleArray[2] = 1.0f;
		ScaleChanged = true;
	}
	ImGui::PopStyleColor(3);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Reset to one");
	}

	if (ScaleChanged)
	{
		Scale = {ScaleArray[0], ScaleArray[1], ScaleArray[2]};
		bTransformChanged = true;
	}

	ImGui::PopStyleColor(3);

	// Transform 업데이트
	if (bTransformChanged)
	{
		LocalPose.Location = Location;
		LocalPose.Rotation = Rotation;
		LocalPose.Scale = Scale;
		Component->SetLocalPose(BoneIndex, LocalPose);
	}
}
