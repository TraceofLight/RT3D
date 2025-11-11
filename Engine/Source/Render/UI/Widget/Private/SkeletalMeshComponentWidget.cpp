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

IMPLEMENT_CLASS(USkeletalMeshComponentWidget, UWidget)

void USkeletalMeshComponentWidget::Initialize()
{
	if (!World)
	{
		World = GWorld;
	}
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
		// if (TargetWorld->GetWorldType() == EWorldType::EditorPreview)
		{
			RenderBoneHierachy(SkeletalMeshComponent);
		}
	}


	ImGui::PopStyleColor(5);
}

void USkeletalMeshComponentWidget::RenderSkeletalMeshSelector()
{
	USkeletalMesh* CurrentSkeletalMesh = SkeletalMeshComponent->GetSkeletalMesh();
	FString PreviewName = "None";

	if (CurrentSkeletalMesh && CurrentSkeletalMesh->GetSkeletalMeshAsset())
	{
		PreviewName = CurrentSkeletalMesh->GetSkeletalMeshAsset()->PathFileName.ToString();
	}

	if (ImGui::BeginCombo("Skeletal Mesh", PreviewName.c_str()))
	{
		for (TObjectIterator<USkeletalMesh> It; It; ++It)
		{
			USkeletalMesh* MeshInList = *It;
			if (!MeshInList || !MeshInList->IsValid()) continue;

			FString MeshName = MeshInList->GetSkeletalMeshAsset()->PathFileName.ToString();
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
		UMaterial* CurrentMaterial = CurrentMesh->GetMaterial(SlotIndex);
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

		std::string ComboId = "##MaterialSlotCombo_" + std::to_string(SlotIndex);
		if (ImGui::BeginCombo(ComboId.c_str(), PreviewName.c_str()))
		{
			RenderAvailableMaterials(SlotIndex);
			ImGui::EndCombo();
		}

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
		bool bIsSelected = (SkeletalMeshComponent->GetSkeletalMesh()->GetMaterial(TargetSlotIndex) == Material);

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
			SkeletalMeshComponent->GetSkeletalMesh()->SetMaterial(TargetSlotIndex, Material);
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

	 if (ImGui::CollapsingHeader("Preview Controls", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // 1) Directional Light 회전
        if (UDirectionalLightComponent* Dir = FindFirstDirectional(TargetWorld))
        {
            FVector euler = Dir->GetRelativeRotation().ToEuler(); // Pitch, Yaw, Roll
            float pitch = euler.X, yaw = euler.Y, roll = euler.Z;

            ImGui::TextUnformatted("Directional Light");
            bool changed = false;
            changed |= ImGui::DragFloat("Pitch", &pitch, 0.2f, -89.9f, 89.9f, "%.1f deg");
            changed |= ImGui::DragFloat("Yaw",   &yaw,   0.2f, -360.f, 360.f, "%.1f deg");
            changed |= ImGui::DragFloat("Roll",  &roll,  0.2f, -360.f, 360.f, "%.1f deg");
            if (changed)
            {
                Dir->SetRelativeRotation(FQuat::FromEuler(FVector(pitch, yaw, roll)));
            }
        	float Intensity = Dir->GetIntensity();
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

        // 2) Skeletal 위치/스케일
        {
            ImGui::TextUnformatted("SkeletalMesh Transform");
            FVector Location = TargetComponent->GetRelativeLocation();
        	FVector Rotation = TargetComponent->GetRelativeRotation().ToEuler();
            FVector Scale = TargetComponent->GetRelativeScale3D();

            if (ImGui::DragFloat3("Location", &Location.X, 0.5f)) {
                TargetComponent->SetRelativeLocation(Location);
            }
        	if (ImGui::DragFloat3("Rotation", &Rotation.X, 0.5f)) {
        		TargetComponent->SetRelativeRotation(FQuat::FromEuler(Rotation));
        	}

            // Uniform 스케일 토글
            static bool bUniform = true;
            ImGui::Checkbox("Uniform Scale", &bUniform);

            if (bUniform)
            {
                float s = (Scale.X + Scale.Y + Scale.Z) / 3.0f;
                if (ImGui::DragFloat("Scale", &s, 0.01f, 0.001f, 100.0f, "%.3f")) {
                    TargetComponent->SetRelativeScale3D(FVector(s, s, s));
                }
            }
            else
            {
                if (ImGui::DragFloat3("ScaleXYZ", &Scale.X, 0.01f, 0.001f, 100.0f, "%.3f")) {
                    TargetComponent->SetRelativeScale3D(Scale);
                }
            }
        }

        ImGui::Separator();

        // 3) 카메라 속도
        {
            ImGui::TextUnformatted("Editor Camera Speed");
            if (PreviewClient)
            {
                float base  = PreviewClient->GetMoveSpeedBase();

                bool c1 = ImGui::DragFloat("Base (units/s)", &base, 1.0f, 1.0f, 2000.0f, "%.0f");

                if (c1) PreviewClient->SetMoveSpeedBase(base);
            }
            else
            {
                ImGui::TextDisabled("PreviewClient not set");
            }
        }
    }

}

void USkeletalMeshComponentWidget::DrawSkeletalBone(FSkeleton* Skeleton, int idx)
{
	ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth |
		ImGuiTreeNodeFlags_DefaultOpen;

	bool bHasChild = Skeleton->Childs.Num() > 0;
	if (bHasChild == false)
	{
		NodeFlags |= ImGuiTreeNodeFlags_Leaf;
	}

	FName CurName = Skeleton->BoneNames[idx];
	if (SelectedBoneName == CurName)
	{
		NodeFlags |= ImGuiTreeNodeFlags_Selected;
	}

	if (ImGui::TreeNodeEx(Skeleton->BoneNames[idx].ToString().c_str(), NodeFlags))
	{
		if (ImGui::IsItemClicked())
		{
			SelectedBoneName = Skeleton->BoneNames[idx];
			SelectedBoneIdx = idx;
		}
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
	uint32 BoneCount = Skeleton->BoneNames.Num();
	DrawSkeletalBone(Skeleton, 0);
	ImGui::Text("Transform");

	if (SelectedBoneIdx != -1)
	{
		FTransform& BoneTransform = SkeletalMeshComponent->GetLocalPose(SelectedBoneIdx);
		ImGui::DragFloat3("Bone Location", &BoneTransform.Location.X, 0.1f);

		FVector EulerRotation = BoneTransform.Rotation.ToEuler();
		if (ImGui::DragFloat3("Bone Rotation", &EulerRotation.X, 0.001f))
		{
			BoneTransform.Rotation = FQuat::FromEuler(EulerRotation);
		}

		ImGui::DragFloat3("Bone Scale", &BoneTransform.Scale.X, 0.1f);

		SkeletalMeshComponent->SetLocalPose(SelectedBoneIdx, BoneTransform);
	}

}

void USkeletalMeshComponentWidget::OpenFbxPreviewViewport(USkeletalMesh* SkeletalMesh)
{
	if (!SkeletalMesh)
	{
		return;
	}

	UUIManager& UIManager = UUIManager::GetInstance();
	const FName PreviewWindowName = FName("FBX Viewport");
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
