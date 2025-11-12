#include "pch.h"
#include "Render/UI/Window/Public/FbxViewportWindow.h"
#include "Render/UI/Window/Public/PreviewScene.h"
#include "Render/UI/Widget/Public/SkeletalMeshComponentWidget.h"
#include "Render/UI/Widget/Public/ViewportControlWidget.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Component/Mesh/Public/SkeletalMeshComponent.h"
#include "Component/Mesh/Public/BoneTransformProxy.h"
#include "Manager/Input/Public/InputManager.h"
#include "Manager/UI/Public/ViewportManager.h"
#include "Render/UI/Viewport/Public/PreviewViewportClient.h"
#include "Manager/Time/Public/TimeManager.h"
#include "Runtime/CoreUObject/Public/NewObject.h"
#include "Runtime/Renderer/Public/RenderResourceFactory.h"
#include "Editor/Public/BatchLines.h"
#include "Editor/Public/Gizmo.h"
#include "Editor/Public/GizmoMath.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "ImGui/imgui.h"
#include "Component/Mesh/Public/SkeletalMesh.h"
#include "Texture/Public/Material.h"
#include "Texture/Public/Texture.h"
#include "Runtime/CoreUObject/Public/ObjectIterator.h"

IMPLEMENT_CLASS(UFbxViewportWindow, UUIWindow)

void UFbxViewportWindow::SelectBone(int32 BoneIndex)
{
	if (!PreviewScene)
	{
		return;
	}

	USkeletalMeshComponent* PreviewComponent = PreviewScene->GetPreviewSkeletalComponent();
	if (!PreviewComponent || !PreviewComponent->GetSkeletalMesh() || !PreviewComponent->GetSkeletalMesh()->GetSkeleton())
	{
		return;
	}

	const FSkeleton* Skeleton = PreviewComponent->GetSkeletalMesh()->GetSkeleton();
	if (BoneIndex < 0 || BoneIndex >= Skeleton->GetNumBones())
	{
		return;
	}

	// GlobalPose가 비어있으면 초기화
	if (PreviewComponent->GetGlobalPose().IsEmpty())
	{
		PreviewComponent->UseReferencePose();
	}

	// 기존 선택 해제
	DeselectBone();

	// 새로운 본 선택
	SelectedBoneIndex = BoneIndex;

	// Bone 선택 시 자동으로 BoneEdit 모드로 전환
	if (CurrentEditMode != EEditMode::BoneEdit)
	{
		SetEditMode(EEditMode::BoneEdit);
	}

	// BoneTransformProxy 생성
	if (!BoneTransformProxy)
	{
		BoneTransformProxy = NewObject<UBoneTransformProxy>(this);
	}

	BoneTransformProxy->SetBoneInfo(PreviewComponent, BoneIndex);
	BoneTransformProxy->SyncTransformFromBone();

	// PreviewGizmo에 타겟 설정
	if (PreviewGizmo)
	{
		PreviewGizmo->SetSelectedComponent(BoneTransformProxy);
	}

	// SkeletalWidget에 하이라이팅 전파
	if (SkeletalWidget)
	{
		SkeletalWidget->SetHighlightedBoneIndex(BoneIndex);
	}
}

void UFbxViewportWindow::DeselectBone()
{
	SelectedBoneIndex = -1;

	// BoneTransformProxy 정리
	if (BoneTransformProxy)
	{
		// PreviewGizmo 타겟 해제
		if (PreviewGizmo)
		{
			PreviewGizmo->SetSelectedComponent(nullptr);
		}

		SafeDelete(BoneTransformProxy);
		BoneTransformProxy = nullptr;
	}

	// SkeletalWidget 하이라이팅 해제
	if (SkeletalWidget)
	{
		SkeletalWidget->SetHighlightedBoneIndex(-1);
	}
}

void UFbxViewportWindow::LoadFbxFile(const path& File)
{
	const std::string FileName = File.string();
	UE_LOG_WARNING("FbxViewportWindow: LoadFbxFile is not implemented yet (%s).", FileName.c_str());
}

void UFbxViewportWindow::SetPreviewSkeletalMesh(USkeletalMesh* SkeletalMesh)
{
	if (!SkeletalMesh)
	{
		return;
	}

	EnsurePreviewInfrastructure();
	if (!bPreviewReady)
	{
		UE_LOG_ERROR("FbxViewportWindow: cannot apply preview mesh because preview scene is not ready.");
		return;
	}

	USkeletalMeshComponent* PreviewComponent = PreviewScene ? PreviewScene->GetPreviewSkeletalComponent() : nullptr;
	if (!PreviewComponent)
	{
		UE_LOG_ERROR("FbxViewportWindow: preview skeletal component is missing.");
		return;
	}

	PreviewComponent->SetSkeletalMesh(SkeletalMesh);
	PreviewComponent->SetWorldScale3D(FVector(10.0f, 10.0f, 10.0f));
	PreviewComponent->UseReferencePose();

	if (SkeletalWidget)
	{
		SkeletalWidget->SetTargetComponent(PreviewComponent);
	}

	// Material Instancing
	CleanupMaterialInstances();
	CreateMaterialInstances();
}

void UFbxViewportWindow::Initialize()
{
    EnsurePreviewInfrastructure();
    if (!bPreviewReady)
    {
        UE_LOG_ERROR("FbxViewportWindow: Preview setup failed during Initialize().");
        return;
    }

    UpdateSkeletalWidgetTargets();


    // ViewportControlWidget 생성
    if (!ViewportControlWidget)
    {
        ViewportControlWidget = new UViewportControlWidget();
        ViewportControlWidget->Initialize();
    }

    // PreviewGizmo 생성
    if (!PreviewGizmo)
    {
        PreviewGizmo = NewObject<UGizmo>(this);
        PreviewGizmo->SetGizmoMode(EGizmoMode::Translate);
    }

    // PreviewClient에 PreviewGizmo 설정 (드래그 처리를 위해)
    if (PreviewClient && PreviewGizmo)
    {
        PreviewClient->SetGizmo(PreviewGizmo);
    }

    UE_LOG("FbxViewportWindow: initialized");
}

void UFbxViewportWindow::Cleanup()
{
    // 본 선택 정리
    DeselectBone();

    CachedSize = ImVec2(0, 0);
    SRV.Reset();
    RTV.Reset();
    ColorRT.Reset();
    DSV.Reset();
    DepthTex.Reset();

	CleanupMaterialInstances();

    if (PreviewScene)
    {
        // PreviewScene->Shutdown();
        SafeDelete(PreviewScene);
        PreviewScene = nullptr;
    }

    if (PreviewViewport)
    {
        PreviewViewport->SetViewportClient(nullptr);
        SafeDelete(PreviewViewport);
        PreviewViewport = nullptr;
    }

    SafeDelete(PreviewClient);
    PreviewClient = nullptr;

    if (PreviewGizmo)
    {
        PreviewGizmo = nullptr;
    }

    bPreviewReady = false;

    SafeDelete(SkeletalWidget);
    SkeletalWidget = nullptr;

    SafeDelete(ViewportControlWidget);
    ViewportControlWidget = nullptr;
}



void UFbxViewportWindow::EnsureRenderTargets(const ImVec2& size)
{
	const UINT w = (UINT)std::max(1.0f, size.x);
	const UINT h = (UINT)std::max(1.0f, size.y);
	if ((UINT)CachedSize.x == w && (UINT)CachedSize.y == h && SRV) return;

	CachedSize = ImVec2((float)w, (float)h);
	SRV.Reset(); RTV.Reset(); ColorRT.Reset();
	DSV.Reset(); DepthTex.Reset();
	HitProxyRTV.Reset(); HitProxyRT.Reset(); HitProxyStagingTex.Reset();

	auto* dev = URenderer::GetInstance().GetDevice();
	// Color
	D3D11_TEXTURE2D_DESC td{};
	td.Width = w; td.Height = h;
	td.MipLevels = 1; td.ArraySize = 1;
	td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	td.SampleDesc = {1,0};
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	dev->CreateTexture2D(&td, nullptr, ColorRT.GetAddressOf());
	dev->CreateRenderTargetView(ColorRT.Get(), nullptr, RTV.GetAddressOf());
	dev->CreateShaderResourceView(ColorRT.Get(), nullptr, SRV.GetAddressOf());

	// Depth
	td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	dev->CreateTexture2D(&td, nullptr, DepthTex.GetAddressOf());
	dev->CreateDepthStencilView(DepthTex.Get(), nullptr, DSV.GetAddressOf());

	// HitProxy RenderTarget
	td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_RENDER_TARGET;
	td.CPUAccessFlags = 0;
	dev->CreateTexture2D(&td, nullptr, HitProxyRT.GetAddressOf());
	dev->CreateRenderTargetView(HitProxyRT.Get(), nullptr, HitProxyRTV.GetAddressOf());

	// HitProxy Staging Texture (CPU Read용)
	td.Usage = D3D11_USAGE_STAGING;
	td.BindFlags = 0;
	td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	dev->CreateTexture2D(&td, nullptr, HitProxyStagingTex.GetAddressOf());

	// 프리뷰 뷰포트에도 크기 반영
	if (PreviewViewport)
		PreviewViewport->SetRect({0,0,(int)w,(int)h});
}

void UFbxViewportWindow::EnsurePreviewInfrastructure()
{
    if (bPreviewReady && PreviewScene && PreviewScene->GetWorld())
    {
        return;
    }

    if (!PreviewViewport)
    {
        PreviewViewport = new FViewport();
    }

    const bool bClientWasNull = (PreviewClient == nullptr);
    if (!PreviewClient)
    {
        PreviewClient = new FPreviewViewportClient();
    }

    if (PreviewViewport && PreviewClient)
    {
        PreviewViewport->SetViewportClient(PreviewClient);
        PreviewClient->SetOwningViewport(PreviewViewport);
    }

    if (bClientWasNull && PreviewClient)
    {
        PreviewClient->SetViewType(EViewType::Perspective);
        PreviewClient->SetViewMode(EViewModeIndex::VMI_BlinnPhong);
        PreviewClient->EnableEditorCamera(true);
        PreviewClient->SetViewLocation(FVector(-50, 0, 20));
        PreviewClient->SetViewRotation(FVector(-15, 0, 0));
    }

    if (PreviewScene && !PreviewScene->GetWorld())
    {
        PreviewScene->Shutdown();
        SafeDelete(PreviewScene);
    }

    if (!PreviewScene)
    {
        PreviewScene = new FPreviewScene();
        if (!PreviewScene->Initialize(this))
        {
            UE_LOG_ERROR("FbxViewportWindow: Failed to initialize preview scene.");
            SafeDelete(PreviewScene);
        }
    }

    // PreviewClient에 PreviewScene 연결
    if (PreviewClient && PreviewScene)
    {
        PreviewClient->SetPreviewScene(PreviewScene);
    }

    bPreviewReady = (PreviewViewport && PreviewClient && PreviewScene && PreviewScene->GetWorld());
    if (!bPreviewReady)
    {
        UE_LOG_ERROR("FbxViewportWindow: Preview infrastructure is not ready.");
    }
}

void UFbxViewportWindow::OnPostRenderWindow()
{
	EnsurePreviewInfrastructure();
	if (!bPreviewReady) return;

	UpdateSkeletalWidgetTargets();

	const ImVec2 totalAvail = ImGui::GetContentRegionAvail();
	if (totalAvail.x < 1 || totalAvail.y < 1) return;

	const bool bHasSkeletal = (SkeletalWidget && PreviewScene && PreviewScene->GetPreviewSkeletalComponent());
	if (!bHasSkeletal) {
		// 스켈레톤이 없으면 뷰포트만
		RenderPreviewViewport(totalAvail);
		return;
	}

	// 최소 뷰포트 폭
	const float minViewportWidth = 80.0f;

	// 현재 가용폭에서 좌/우 패널이 차지할 수 있는 최대폭 계산
	auto clamp_by_window = [&](float want, float minw) {
		float usedSides = 0.0f;
		if (bLeftVisible)  usedSides += want + SplitterThickness + ImGui::GetStyle().ItemSpacing.x;
		if (bRightVisible) usedSides += RightPanelWidth + SplitterThickness + ImGui::GetStyle().ItemSpacing.x;
		float maxw = std::max(minw, totalAvail.x - usedSides - minViewportWidth);
		return std::clamp(want, minw, maxw);
		};

	LeftPanelWidth = clamp_by_window(LeftPanelWidth, LeftMinWidth);
	// 오른쪽은 좌측 조정 후 다시 계산
	RightPanelWidth = clamp_by_window(RightPanelWidth, RightMinWidth);

	// 중앙 뷰포트 폭 계산
	float viewportWidth = totalAvail.x;
	if (bLeftVisible)
		viewportWidth -= (LeftPanelWidth + SplitterThickness + ImGui::GetStyle().ItemSpacing.x);
	if (bRightVisible)
		viewportWidth -= (RightPanelWidth + SplitterThickness + ImGui::GetStyle().ItemSpacing.x);
	viewportWidth = std::max(viewportWidth, minViewportWidth);

	// 1) 왼쪽 패널 (PreviewTopControls)
	if (bLeftVisible) {
		RenderLeftControlsPanel(ImVec2(LeftPanelWidth, totalAvail.y));
		ImGui::SameLine();

		// Left Splitter (왼쪽 패널 오른쪽 경계)
		const ImVec2 splitterSize(SplitterThickness, totalAvail.y);
		ImVec2 splitterPos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##LeftSplitter", splitterSize, ImGuiButtonFlags_MouseButtonLeft);
		const bool hovered = ImGui::IsItemHovered();
		const bool held = ImGui::IsItemActive();
		if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

		// 핸들 시각화
		ImDrawList* dl = ImGui::GetWindowDrawList();
		ImU32 handleCol = hovered || held ? IM_COL32(200, 200, 200, 180) : IM_COL32(140, 140, 140, 120);
		float cx = splitterPos.x + SplitterThickness * 0.5f;
		dl->AddLine(ImVec2(cx, splitterPos.y + 6.0f), ImVec2(cx, splitterPos.y + splitterSize.y - 6.0f), handleCol, 2.0f);

		if (held) {
			float dx = ImGui::GetIO().MouseDelta.x;
			// 왼쪽 패널은 오른쪽으로 드래그(+dx) => 폭 증가
			float usedRight = bRightVisible ? (RightPanelWidth + SplitterThickness + ImGui::GetStyle().ItemSpacing.x) : 0.0f;
			float maxLeft = std::max(LeftMinWidth, totalAvail.x - usedRight - minViewportWidth);
			LeftPanelWidth = std::clamp(LeftPanelWidth + dx, LeftMinWidth, maxLeft);
		}

		if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			LeftPrevWidth = LeftPanelWidth;
			bLeftVisible = false;
		}

		ImGui::SameLine();
	}

	// 2) 가운데 뷰포트
	RenderPreviewViewport(ImVec2(viewportWidth, totalAvail.y));

	// 3) 오른쪽 스플리터 + 오른쪽 패널 (BoneHierachy)
	if (bRightVisible) {
		ImGui::SameLine();

		// Right Splitter (오른쪽 패널 왼쪽 경계)
		const ImVec2 splitterSize(SplitterThickness, totalAvail.y);
		ImVec2 splitterPos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##RightSplitter", splitterSize, ImGuiButtonFlags_MouseButtonLeft);
		const bool hovered = ImGui::IsItemHovered();
		const bool held = ImGui::IsItemActive();
		if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

		// 핸들 시각화
		ImDrawList* dl = ImGui::GetWindowDrawList();
		ImU32 handleCol = hovered || held ? IM_COL32(200, 200, 200, 180) : IM_COL32(140, 140, 140, 120);
		float cx = splitterPos.x + SplitterThickness * 0.5f;
		dl->AddLine(ImVec2(cx, splitterPos.y + 6.0f), ImVec2(cx, splitterPos.y + splitterSize.y - 6.0f), handleCol, 2.0f);

		if (held) {
			float dx = ImGui::GetIO().MouseDelta.x;
			// 오른쪽 패널은 오른쪽으로 드래그(+dx) => 폭 감소 (경계가 오른쪽으로 이동)
			float usedLeft = bLeftVisible ? (LeftPanelWidth + SplitterThickness + ImGui::GetStyle().ItemSpacing.x) : 0.0f;
			float maxRight = std::max(RightMinWidth, totalAvail.x - usedLeft - minViewportWidth);
			RightPanelWidth = std::clamp(RightPanelWidth - dx, RightMinWidth, maxRight);
		}

		if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			RightPrevWidth = RightPanelWidth;
			bRightVisible = false;
		}

		ImGui::SameLine();
		RenderBoneHeriarchy(ImVec2(RightPanelWidth, totalAvail.y));
	}
}

void UFbxViewportWindow::RenderPreviewViewport(const ImVec2& InSize)
{
	ImVec2 ChildSize(std::max(1.0f, InSize.x), std::max(1.0f, InSize.y));
	ImGui::BeginChild("FBXViewportRegion", ChildSize, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	const ImVec2 viewportAvail = ImGui::GetContentRegionAvail();
	if (viewportAvail.x < 1 || viewportAvail.y < 1)
	{
		ImGui::EndChild();
		return;
	}

	EnsureRenderTargets(viewportAvail);
	ImGui::InvisibleButton("FBXViewportArea", viewportAvail);
	const ImVec2 p0 = ImGui::GetItemRectMin();
	const ImVec2 p1 = ImGui::GetItemRectMax();
	ImDrawList* dl = ImGui::GetWindowDrawList();
	dl->AddImage((ImTextureID)SRV.Get(), p0, p1);

	// PreviewViewport Rect 업데이트 (RenderTarget 상대 좌표 사용 - 0,0 기준)
	if (PreviewViewport)
	{
		PreviewViewport->SetRect({
			0,
			0,
			static_cast<int>(p1.x - p0.x),
			static_cast<int>(p1.y - p0.y)
		});
	}

	bHovered = ImGui::IsItemHovered();
	if (PreviewClient) PreviewClient->SetInputEnabled(bHovered);

	// Preview Viewport에 마우스/키보드 입력 전달 (bHovered일 때만)
	if (bHovered && PreviewViewport && PreviewClient)
	{
		// ImGui가 마우스/키보드 입력을 강제로 소비하도록 설정 (하위 에디터로 전달 방지)
		ImGui::SetNextFrameWantCaptureMouse(true);
		ImGui::SetNextFrameWantCaptureKeyboard(true);

		ImVec2 MousePos = ImGui::GetMousePos();
		ImVec2 LocalMouse = ImVec2(MousePos.x - p0.x, MousePos.y - p0.y);
		int LocalX = static_cast<int>(LocalMouse.x);
		int LocalY = static_cast<int>(LocalMouse.y);

		// 키보드 입력 처리 (W/E/R: 기즈모 모드 전환, Space: 사이클)
		// 우클릭 중에는 카메라 컨트롤이므로 기즈모 모드 변경 무시
		const bool bRightMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Right);

		if (!bRightMouseDown)
		{
			// BoneEdit 모드일 때 Gizmo 모드 변경 (W/E/R)
			if (CurrentEditMode == EEditMode::BoneEdit && PreviewGizmo)
			{
				if (ImGui::IsKeyPressed(ImGuiKey_W))
				{
					PreviewGizmo->SetGizmoMode(EGizmoMode::Translate);
				}
				if (ImGui::IsKeyPressed(ImGuiKey_E))
				{
					PreviewGizmo->SetGizmoMode(EGizmoMode::Rotate);
				}
				if (ImGui::IsKeyPressed(ImGuiKey_R))
				{
					PreviewGizmo->SetGizmoMode(EGizmoMode::Scale);
				}
			}
			else
			{
				// View 모드일 때는 PreviewClient에 전달
				if (ImGui::IsKeyPressed(ImGuiKey_W))
				{
					PreviewClient->InputKey(EKeyInput::W, true);
				}
				if (ImGui::IsKeyPressed(ImGuiKey_E))
				{
					PreviewClient->InputKey(EKeyInput::E, true);
				}
				if (ImGui::IsKeyPressed(ImGuiKey_R))
				{
					PreviewClient->InputKey(EKeyInput::R, true);
				}
			}
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Space))
		{
			PreviewClient->InputKey(EKeyInput::Space, true);
		}

		// 마우스 버튼 Pressed 처리
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			PreviewViewport->HandleMouseDown(0, LocalX, LocalY);
			// Gizmo/Bone 피킹
			HandleMouseClick(LocalMouse);
		}
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		{
			PreviewViewport->HandleMouseDown(1, LocalX, LocalY);
		}
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
		{
			PreviewViewport->HandleMouseDown(2, LocalX, LocalY);
		}

		// 마우스 버튼 Released 처리
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			PreviewViewport->HandleMouseUp(0, LocalX, LocalY);
			// 기즈모 드래그 종료
			if (PreviewGizmo && PreviewGizmo->IsDragging())
			{
				EGizmoDirection Direction = PreviewGizmo->GetGizmoDirection();
				PreviewGizmo->OnMouseRelease(Direction);
				PreviewGizmo->SetGizmoDirection(EGizmoDirection::None);

				// 최종 Transform 적용
				if (BoneTransformProxy)
				{
					BoneTransformProxy->ApplyTransformToBone();
				}
			}
		}
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
		{
			PreviewViewport->HandleMouseUp(1, LocalX, LocalY);
		}
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle))
		{
			PreviewViewport->HandleMouseUp(2, LocalX, LocalY);
		}

		// 마우스 드래그 처리
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			PreviewViewport->HandleCapturedMouseMove(LocalX, LocalY);

			// Gizmo 드래그 중일 때 Bone Transform 업데이트
			if (PreviewGizmo && PreviewGizmo->IsDragging() && BoneTransformProxy)
			{
				// Bone Transform을 BoneTransformProxy를 통해 즉시 반영
				BoneTransformProxy->ApplyTransformToBone();
			}
		}
		else if (ImGui::IsMouseDragging(ImGuiMouseButton_Right) ||
		         ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
		{
			PreviewViewport->HandleCapturedMouseMove(LocalX, LocalY);
		}
		else
		{
			// 일반 마우스 이동
			PreviewViewport->HandleMouseMove(LocalX, LocalY);

			// TODO: Gizmo 호버링 처리 (성능 문제로 임시 비활성화)
			// if (CurrentEditMode == EEditMode::BoneEdit && PreviewGizmo && PreviewGizmo->HasComponent())
			// {
			// 	UpdateGizmoHover(LocalMouse);
			// }
		}
	}

	// ViewportControlWidget 툴바 렌더링 (공통 툴바 사용)
	if (ViewportControlWidget && PreviewViewport && PreviewClient)
	{
		// Child Window 내부에서 커서를 좌상단으로 설정
		ImGui::SetCursorScreenPos(p0);
		ViewportControlWidget->RenderViewportToolbar(PreviewViewport, PreviewClient);
	}

	// 선택된 본 이름 표시 (Overlay)
	if (SelectedBoneIndex >= 0 && PreviewScene)
	{
		USkeletalMeshComponent* PreviewComponent = PreviewScene->GetPreviewSkeletalComponent();
		if (PreviewComponent && PreviewComponent->GetSkeletalMesh() && PreviewComponent->GetSkeletalMesh()->GetSkeleton())
		{
			const FSkeleton* Skeleton = PreviewComponent->GetSkeletalMesh()->GetSkeleton();
			if (SelectedBoneIndex < Skeleton->BoneNames.Num())
			{
				const FName& BoneName = Skeleton->BoneNames[SelectedBoneIndex];
				const std::string BoneNameStr = BoneName.ToString();

				// Viewport 좌상단에 오버레이 표시 (ViewportControl 아래)
				ImGui::SetCursorScreenPos(ImVec2(p0.x + 10, p0.y + 50));
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // 노란색
				ImGui::Text("Selected Bone: %s [%d]", BoneNameStr.c_str(), SelectedBoneIndex);
				ImGui::PopStyleColor();
			}
		}
	}

	// PreviewViewport는 InputManager에 등록되지 않으므로 PumpMouseFromInputManager 호출 불필요
	// ImGui에서 직접 입력을 처리함

	const float DeltaTime = UTimeManager::GetInstance().GetDeltaTime();

	// ImGui 입력을 사용하여 카메라 업데이트
	if (bHovered && PreviewClient && PreviewClient->GetInputEnabled())
	{
		UpdatePreviewCamera(DeltaTime);
	}

	UWorld* SceneWorld = PreviewScene ? PreviewScene->GetWorld() : nullptr;

	if (!PreviewViewport || !PreviewClient) return;

	// RenderExternalViewport가 PreviewScene의 World를 렌더링
	URenderer::GetInstance().RenderExternalViewport(
		PreviewViewport, PreviewClient, RTV.Get(), DSV.Get(), SceneWorld);

	// PreviewBatchLines 렌더링 (Skeleton 시각화)
	// BatchLines는 UObject이므로 RenderExternalViewport가 자동으로 렌더링하지 않음
	if (PreviewScene)
	{
		UBatchLines* PreviewBatchLines = PreviewScene->GetBatchLines();
		USkeletalMeshComponent* PreviewComponent = PreviewScene->GetPreviewSkeletalComponent();
		if (PreviewBatchLines && PreviewComponent && PreviewComponent->GetSkeletalMesh())
		{
			const FSkeleton* Skeleton = PreviewComponent->GetSkeletalMesh()->GetSkeleton();
			const TArray<FMatrix>& GlobalPose = PreviewComponent->GetGlobalPose();
			const FMatrix& ComponentWorld = PreviewComponent->GetWorldTransformMatrix();

			if (Skeleton && !GlobalPose.IsEmpty())
			{
				PreviewBatchLines->UpdateSkeletonVertices(Skeleton, GlobalPose, ComponentWorld, SelectedBoneIndex);
				PreviewBatchLines->UpdateVertexBuffer();

				D3D11_VIEWPORT D3DViewport;
				D3DViewport.TopLeftX = 0;
				D3DViewport.TopLeftY = 0;
				D3DViewport.Width = viewportAvail.x;
				D3DViewport.Height = viewportAvail.y;
				D3DViewport.MinDepth = 0.0f;
				D3DViewport.MaxDepth = 1.0f;

				auto* DeviceContext = URenderer::GetInstance().GetDeviceContext();

				// RenderTarget 백업
				ID3D11RenderTargetView* OldRTV = nullptr;
				ID3D11DepthStencilView* OldDSV = nullptr;
				DeviceContext->OMGetRenderTargets(1, &OldRTV, &OldDSV);

				UINT NumViewports = 1;
				D3D11_VIEWPORT OldViewport;
				DeviceContext->RSGetViewports(&NumViewports, &OldViewport);

				// FbxViewportWindow의 RTV/DSV 설정
				DeviceContext->OMSetRenderTargets(1, RTV.GetAddressOf(), DSV.Get());
				DeviceContext->RSSetViewports(1, &D3DViewport);

				PreviewBatchLines->Render();

				// RenderTarget 복원
				DeviceContext->OMSetRenderTargets(1, &OldRTV, OldDSV);
				DeviceContext->RSSetViewports(1, &OldViewport);

				if (OldRTV) OldRTV->Release();
				if (OldDSV) OldDSV->Release();
			}
		}
	}

	// PreviewGizmo 렌더링 (BoneEdit 모드일 때만)
	if (CurrentEditMode == EEditMode::BoneEdit && PreviewGizmo && PreviewGizmo->HasComponent())
	{
		// BoneTransformProxy Transform 동기화 (매 프레임)
		if (BoneTransformProxy && !PreviewGizmo->IsDragging())
		{
			BoneTransformProxy->SyncTransformFromBone();
		}

		D3D11_VIEWPORT D3DViewport;
		D3DViewport.TopLeftX = 0;
		D3DViewport.TopLeftY = 0;
		D3DViewport.Width = viewportAvail.x;
		D3DViewport.Height = viewportAvail.y;
		D3DViewport.MinDepth = 0.0f;
		D3DViewport.MaxDepth = 1.0f;

		auto* DeviceContext = URenderer::GetInstance().GetDeviceContext();

		// 기존 RenderTarget 백업
		ID3D11RenderTargetView* OldRTV = nullptr;
		ID3D11DepthStencilView* OldDSV = nullptr;
		DeviceContext->OMGetRenderTargets(1, &OldRTV, &OldDSV);

		UINT NumViewports = 1;
		D3D11_VIEWPORT OldViewport;
		DeviceContext->RSGetViewports(&NumViewports, &OldViewport);

		// FbxViewportWindow의 RTV/DSV 설정
		DeviceContext->OMSetRenderTargets(1, RTV.GetAddressOf(), DSV.Get());
		DeviceContext->RSSetViewports(1, &D3DViewport);

		// Gizmo 위치 로그 (BoneTransformProxy를 통해)
		if (BoneTransformProxy)
		{
			FVector GizmoLoc = BoneTransformProxy->GetWorldLocation();
			static int GizmoLogCount = 0;
			if (++GizmoLogCount % 60 == 0)
			{
				UE_LOG("FbxViewportWindow: Gizmo render at Location=(%.1f,%.1f,%.1f)", GizmoLoc.X, GizmoLoc.Y, GizmoLoc.Z);
			}
		}

		// Depth Test 비활성화 (Gizmo가 항상 보이도록)
		ID3D11DepthStencilState* OldDepthStencilState = nullptr;
		UINT OldStencilRef = 0;
		DeviceContext->OMGetDepthStencilState(&OldDepthStencilState, &OldStencilRef);

		auto& Renderer = URenderer::GetInstance();
		DeviceContext->OMSetDepthStencilState(Renderer.GetDisabledDepthStencilState(), 0);

		// Gizmo 렌더링 (false = TargetComponent를 Editor에서 가져오지 않음)
		if (PreviewClient)
		{
			PreviewGizmo->UpdateScale(PreviewClient, D3DViewport, false);
			PreviewGizmo->RenderGizmo(PreviewClient, D3DViewport, false);
		}

		// DepthStencilState 복원
		DeviceContext->OMSetDepthStencilState(OldDepthStencilState, OldStencilRef);
		if (OldDepthStencilState) OldDepthStencilState->Release();

		// RenderTarget 복원
		DeviceContext->OMSetRenderTargets(1, &OldRTV, OldDSV);
		DeviceContext->RSSetViewports(1, &OldViewport);

		if (OldRTV) OldRTV->Release();
		if (OldDSV) OldDSV->Release();
	}

	ImGui::EndChild();
}

void UFbxViewportWindow::RenderLeftControlsPanel(const ImVec2& InSize)
{
	ImVec2 sz(std::max(1.0f, InSize.x), std::max(1.0f, InSize.y));
	ImGui::BeginChild("FBX_LeftControls", sz, true);

	if (SkeletalWidget && PreviewScene) {
		UWorld* SceneWorld = PreviewScene->GetWorld();
		USkeletalMeshComponent* PreviewComponent = PreviewScene->GetPreviewSkeletalComponent();
		if (SceneWorld && PreviewComponent) {
			SkeletalWidget->RenderPreviewTopControls(SceneWorld, PreviewComponent);
		}
	}

	ImGui::Separator();

	RenderSkeletalMeshSelector();

	if (PreviewScene && PreviewScene->GetPreviewSkeletalComponent() && PreviewScene->GetPreviewSkeletalComponent()->GetSkeletalMesh())
	{
		ImGui::Separator();
		RenderMaterialSections();
	}

	ImGui::EndChild();
}

void UFbxViewportWindow::RenderBoneHeriarchy(const ImVec2& InSize)
{
	ImVec2 sz(std::max(1.0f, InSize.x), std::max(1.0f, InSize.y));
	ImGui::BeginChild("FBX_BoneHierarchy", sz, true);

	if (SkeletalWidget && PreviewScene) {
		USkeletalMeshComponent* PreviewComponent = PreviewScene->GetPreviewSkeletalComponent();
		if (PreviewComponent && PreviewComponent->GetSkeletalMesh()) {
			// 요청 사항: 오른쪽에는 BoneHierachy만
			SkeletalWidget->RenderBoneHierachy(PreviewComponent);
		}
	}

	ImGui::EndChild();
}


void UFbxViewportWindow::RenderSkeletalInspector(const ImVec2& InSize)
{
	if (!SkeletalWidget)
	{
		return;
	}

	ImVec2 PanelSize(std::max(1.0f, InSize.x), std::max(1.0f, InSize.y));
	ImGui::BeginChild("FBXBoneInspector", PanelSize, true);
	SkeletalWidget->RenderWidget();
	ImGui::EndChild();
}

void UFbxViewportWindow::UpdateSkeletalWidgetTargets()
{
	if (!SkeletalWidget)
	{
		SkeletalWidget = NewObject<USkeletalMeshComponentWidget>(this);

		if (SkeletalWidget)
		{
			SkeletalWidget->Initialize();
			SkeletalWidget->SetOwningFbxViewportWindow(this);
		}
	}

	if (!SkeletalWidget)
	{
		return;
	}

	UWorld* SceneWorld = PreviewScene ? PreviewScene->GetWorld() : nullptr;
	SkeletalWidget->SetTargetWorld(SceneWorld);
	USkeletalMeshComponent* PreviewComponent = PreviewScene ? PreviewScene->GetPreviewSkeletalComponent() : nullptr;
	SkeletalWidget->SetTargetComponent(PreviewComponent);

	SkeletalWidget->SetPreviewViewportClient(PreviewClient);
}

void UFbxViewportWindow::RenderSkeletalMeshSelector()
{
	if (!PreviewScene) return;
	USkeletalMeshComponent* PreviewComponent = PreviewScene->GetPreviewSkeletalComponent();
	if (!PreviewComponent) return;

	USkeletalMesh* CurrentSkeletalMesh = PreviewComponent->GetSkeletalMesh();
	FString PreviewName = "None";

	if (CurrentSkeletalMesh && CurrentSkeletalMesh->GetSkeletalMeshAsset())
	{
		PreviewName = CurrentSkeletalMesh->GetSkeletalMeshAsset()->PathFileNameString;
	}

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
				SetPreviewSkeletalMesh(MeshInList);
			}

			if (bIsSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
}

void UFbxViewportWindow::RenderMaterialSections()
{
	if (!PreviewScene) return;
	USkeletalMeshComponent* PreviewComponent = PreviewScene->GetPreviewSkeletalComponent();
	if (!PreviewComponent) return;

	USkeletalMesh* CurrentMesh = PreviewComponent->GetSkeletalMesh();
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
		UMaterial* CurrentMaterial = PreviewComponent->GetMaterial(SlotIndex);
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
		ImGui::PopID();
	}
}

void UFbxViewportWindow::RenderAvailableMaterials(int32 TargetSlotIndex)
{
	if (!PreviewScene) return;
	USkeletalMeshComponent* PreviewComponent = PreviewScene->GetPreviewSkeletalComponent();
	if (!PreviewComponent) return;

	for (TObjectIterator<UMaterial> Iter; Iter; ++Iter)
	{
		UMaterial* Material = *Iter;
		if (!Material)
		{
			continue;
		}

		FString MaterialName = GetMaterialDisplayName(Material);
		bool bIsSelected = (PreviewComponent->GetMaterial(TargetSlotIndex) == Material);

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
			// Create a new instance of the selected material
			UMaterial* NewMaterialInstance = NewObject<UMaterial>(PreviewComponent);
			NewMaterialInstance->CopyFrom(Material);

			// Replace the old instance
			MaterialInstances[TargetSlotIndex] = NewMaterialInstance;
			PreviewComponent->SetMaterial(TargetSlotIndex, NewMaterialInstance);
		}

		if (bIsSelected)
		{
			ImGui::SetItemDefaultFocus();
		}
	}
}


void UFbxViewportWindow::CreateMaterialInstances()
{
	USkeletalMeshComponent* PreviewComponent = PreviewScene ? PreviewScene->GetPreviewSkeletalComponent() : nullptr;
	if (!PreviewComponent) return;

	USkeletalMesh* Mesh = PreviewComponent->GetSkeletalMesh();
	if (!Mesh) return;

	const int32 NumMaterials = Mesh->GetNumMaterials();
	MaterialInstances.SetNum(NumMaterials);

	for (int32 i = 0; i < NumMaterials; ++i)
	{
		UMaterial* OriginalMaterial = Mesh->GetMaterial(i);
		if (OriginalMaterial)
		{
			// Create a new material instance, duplicating the original
			FString InstName = OriginalMaterial->GetName().ToString() + "_Inst_";
			UMaterial* MaterialInstance = NewObject<UMaterial>(PreviewComponent);
			MaterialInstance->SetName(InstName);

			MaterialInstance->CopyFrom(OriginalMaterial);

			MaterialInstances[i] = MaterialInstance;

			PreviewComponent->SetMaterial(i, MaterialInstance);
		}
		else
		{
			MaterialInstances[i] = nullptr;
		}
	}
}

void UFbxViewportWindow::CleanupMaterialInstances()
{
	// The material instances are UObjects and will be garbage collected
	// when their outer (the PreviewComponent) is destroyed.
	// We just need to clear our references to them.
	MaterialInstances.Empty();
}

UFbxViewportWindow::UFbxViewportWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "FBX Viewport";
	Config.DefaultSize = ImVec2(1440, 960);
	Config.MinSize = ImVec2(360, 240);
	Config.DefaultPosition = ImVec2(140, 120);
	Config.InitialState = EUIWindowState::Hidden;
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;
	Config.Priority = 25;
	Config.UpdateWindowFlags();

	SetConfig(Config);
	SetWindowState(EUIWindowState::Hidden);
}

void UFbxViewportWindow::UpdatePreviewCamera(float DeltaTime)
{
	if (!PreviewClient)
	{
		return;
	}

	ImGuiIO& io = ImGui::GetIO();

	// 우클릭 홀드 체크 (Unreal 스타일: RMB 누르고 있을 때만 카메라 컨트롤)
	const bool bRightMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Right);

	// RMB 홀드 중 + 드래그로 카메라 회전
	if (bRightMouseDown && ImGui::IsMouseDragging(ImGuiMouseButton_Right))
	{
		ImVec2 MouseDelta = io.MouseDelta;
		const float MouseSensitivity = 0.15f;

		FVector CurrentRotation = PreviewClient->GetViewRotation();
		CurrentRotation.Y += MouseDelta.x * MouseSensitivity;  // Yaw
		CurrentRotation.X += -MouseDelta.y * MouseSensitivity; // Pitch
		CurrentRotation.X = std::clamp(CurrentRotation.X, -89.9f, 89.9f);
		CurrentRotation.Z = 0.0f;
		PreviewClient->SetViewRotation(CurrentRotation);
	}

	// RMB 홀드 중에만 WASD 이동 활성화 (Unreal 스타일)
	if (bRightMouseDown)
	{
		const float MoveSpeed = 30.0f;
		FVector Direction = FVector::Zero();

		if (ImGui::IsKeyDown(ImGuiKey_W)) Direction += PreviewClient->GetForward();
		if (ImGui::IsKeyDown(ImGuiKey_S)) Direction -= PreviewClient->GetForward();
		if (ImGui::IsKeyDown(ImGuiKey_D)) Direction += PreviewClient->GetRight();
		if (ImGui::IsKeyDown(ImGuiKey_A)) Direction -= PreviewClient->GetRight();
		if (ImGui::IsKeyDown(ImGuiKey_E)) Direction += FVector(0, 0, 1);
		if (ImGui::IsKeyDown(ImGuiKey_Q)) Direction -= FVector(0, 0, 1);

		if (Direction.LengthSquared() > 0.0f)
		{
			Direction.Normalize();
			FVector CurrentLocation = PreviewClient->GetViewLocation();
			CurrentLocation += Direction * MoveSpeed * DeltaTime;
			PreviewClient->SetViewLocation(CurrentLocation);
		}
	}
}

void UFbxViewportWindow::UpdateGizmoHover(const ImVec2& LocalMousePos)
{
	if (!PreviewClient || !HitProxyRTV || !HitProxyStagingTex || !PreviewScene || !PreviewGizmo)
	{
		return;
	}

	// HitProxy 렌더링
	D3D11_VIEWPORT D3DViewport;
	D3DViewport.TopLeftX = 0;
	D3DViewport.TopLeftY = 0;
	D3DViewport.Width = CachedSize.x;
	D3DViewport.Height = CachedSize.y;
	D3DViewport.MinDepth = 0.0f;
	D3DViewport.MaxDepth = 1.0f;

	auto* DeviceContext = URenderer::GetInstance().GetDeviceContext();

	// RenderTarget 백업
	ID3D11RenderTargetView* OldRTV = nullptr;
	ID3D11DepthStencilView* OldDSV = nullptr;
	DeviceContext->OMGetRenderTargets(1, &OldRTV, &OldDSV);

	UINT NumViewports = 1;
	D3D11_VIEWPORT OldViewport;
	DeviceContext->RSGetViewports(&NumViewports, &OldViewport);

	// HitProxy RTV로 전환하고 클리어
	const float ClearColor[4] = {0, 0, 0, 0};
	DeviceContext->ClearRenderTargetView(HitProxyRTV.Get(), ClearColor);
	DeviceContext->ClearDepthStencilView(DSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	DeviceContext->OMSetRenderTargets(1, HitProxyRTV.GetAddressOf(), DSV.Get());
	DeviceContext->RSSetViewports(1, &D3DViewport);

	// ViewProj Constant Buffer 업데이트
	auto& Renderer = URenderer::GetInstance();
	const FCameraConstants& CameraConstants = PreviewClient->GetCameraConstants();

	ID3D11Buffer* ConstantBufferViewProj = Renderer.GetConstantBufferViewProj();
	FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferViewProj, CameraConstants);
	Renderer.GetPipeline()->SetConstantBuffer(1, EShaderType::VS, ConstantBufferViewProj);

	// Gizmo HitProxy만 렌더링 (호버링용 - 간단한 버전)
	RenderGizmoHitProxySimple();

	// RenderTarget 복원
	DeviceContext->OMSetRenderTargets(1, &OldRTV, OldDSV);
	DeviceContext->RSSetViewports(1, &OldViewport);

	if (OldRTV) OldRTV->Release();
	if (OldDSV) OldDSV->Release();

	// HitProxy 텍스처를 Staging으로 복사
	DeviceContext->CopyResource(HitProxyStagingTex.Get(), HitProxyRT.Get());

	// CPU에서 픽셀 읽기
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	if (SUCCEEDED(DeviceContext->Map(HitProxyStagingTex.Get(), 0, D3D11_MAP_READ, 0, &MappedResource)))
	{
		int X = static_cast<int>(LocalMousePos.x);
		int Y = static_cast<int>(LocalMousePos.y);

		if (X >= 0 && X < static_cast<int>(CachedSize.x) && Y >= 0 && Y < static_cast<int>(CachedSize.y))
		{
			uint8_t* PixelData = static_cast<uint8_t*>(MappedResource.pData);
			uint8_t* Pixel = PixelData + (Y * MappedResource.RowPitch) + (X * 4);

			uint32_t HitProxyID = (Pixel[0]) | (Pixel[1] << 8) | (Pixel[2] << 16);

			// Gizmo 호버링만 처리 (HitProxyID 1-6)
			// 드래그 중이 아닐 때만 호버링 업데이트
			if (!PreviewGizmo->IsDragging())
			{
				if (HitProxyID >= 1 && HitProxyID <= 6)
				{
					EGizmoDirection Direction = static_cast<EGizmoDirection>(HitProxyID);
					PreviewGizmo->SetGizmoDirection(Direction);
				}
				else
				{
					PreviewGizmo->SetGizmoDirection(EGizmoDirection::None);
				}
			}
		}

		DeviceContext->Unmap(HitProxyStagingTex.Get(), 0);
	}
}

void UFbxViewportWindow::HandleMouseClick(const ImVec2& LocalMousePos)
{
	if (!PreviewClient || !HitProxyRTV || !HitProxyStagingTex || !PreviewScene)
	{
		return;
	}

	// HitProxy 렌더링
	D3D11_VIEWPORT D3DViewport;
	D3DViewport.TopLeftX = 0;
	D3DViewport.TopLeftY = 0;
	D3DViewport.Width = CachedSize.x;
	D3DViewport.Height = CachedSize.y;
	D3DViewport.MinDepth = 0.0f;
	D3DViewport.MaxDepth = 1.0f;

	auto* DeviceContext = URenderer::GetInstance().GetDeviceContext();

	// RenderTarget 백업
	ID3D11RenderTargetView* OldRTV = nullptr;
	ID3D11DepthStencilView* OldDSV = nullptr;
	DeviceContext->OMGetRenderTargets(1, &OldRTV, &OldDSV);

	UINT NumViewports = 1;
	D3D11_VIEWPORT OldViewport;
	DeviceContext->RSGetViewports(&NumViewports, &OldViewport);

	// HitProxy RTV로 전환하고 클리어
	const float ClearColor[4] = {0, 0, 0, 0};
	DeviceContext->ClearRenderTargetView(HitProxyRTV.Get(), ClearColor);
	DeviceContext->ClearDepthStencilView(DSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	DeviceContext->OMSetRenderTargets(1, HitProxyRTV.GetAddressOf(), DSV.Get());
	DeviceContext->RSSetViewports(1, &D3DViewport);

	// ViewProj Constant Buffer 업데이트 (Preview Viewport의 카메라 사용)
	auto& Renderer = URenderer::GetInstance();
	const FCameraConstants& CameraConstants = PreviewClient->GetCameraConstants();

	ID3D11Buffer* ConstantBufferViewProj = Renderer.GetConstantBufferViewProj();
	FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferViewProj, CameraConstants);
	Renderer.GetPipeline()->SetConstantBuffer(1, EShaderType::VS, ConstantBufferViewProj);

	// Gizmo HitProxy 렌더링 (BoneEdit 모드이고 PreviewGizmo + BoneTransformProxy가 있을 때)
	if (CurrentEditMode == EEditMode::BoneEdit && PreviewGizmo && BoneTransformProxy)
	{
		// 간단한 고정 ID HitProxy 렌더링 (ID 1-6 사용)
		// Bone은 컴포넌트가 아니므로 BoneTransformProxy를 통해 Transform 처리
		RenderGizmoHitProxySimple();
	}

	// Bone HitProxy 렌더링 (SkeletalMeshComponent)
	USkeletalMeshComponent* PreviewComponent = PreviewScene->GetPreviewSkeletalComponent();
	if (PreviewComponent && PreviewComponent->GetSkeletalMesh())
	{
		const FSkeleton* Skeleton = PreviewComponent->GetSkeletalMesh()->GetSkeleton();
		const TArray<FMatrix>& GlobalPose = PreviewComponent->GetGlobalPose();
		const FMatrix& ComponentWorld = PreviewComponent->GetWorldTransformMatrix();

		if (Skeleton && !GlobalPose.IsEmpty())
		{
			// 각 Bone을 HitProxy로 렌더링 (HitProxyID = 100 + BoneIndex)
			auto& Renderer = URenderer::GetInstance();
			constexpr float BoneSphereRadius = 0.15f;  // BatchLines.cpp의 JointSphereRadius와 동일
			constexpr int NumSegments = 8;
			constexpr int NumRings = 6;

			for (int32 BoneIndex = 0; BoneIndex < Skeleton->GetNumBones(); ++BoneIndex)
			{
				FMatrix BoneWorldMatrix = GlobalPose[BoneIndex] * ComponentWorld;
				FVector BoneWorldPos(BoneWorldMatrix._41, BoneWorldMatrix._42, BoneWorldMatrix._43);

				// HitProxyID를 색상으로 인코딩 (100 + BoneIndex)
				uint32_t HitProxyID = 100 + BoneIndex;
				uint8_t R = (HitProxyID) & 0xFF;
				uint8_t G = (HitProxyID >> 8) & 0xFF;
				uint8_t B = (HitProxyID >> 16) & 0xFF;
				FVector4 HitProxyColor(R / 255.0f, G / 255.0f, B / 255.0f, 1.0f);

				// UV Sphere 메시 생성
				TArray<FNormalVertex> Vertices;
				TArray<uint32> Indices;

				for (int ring = 0; ring <= NumRings; ++ring)
				{
					float Phi = static_cast<float>(ring) / NumRings * PI;
					float SinPhi = std::sin(Phi);
					float CosPhi = std::cos(Phi);

					for (int seg = 0; seg <= NumSegments; ++seg)
					{
						float Theta = static_cast<float>(seg) / NumSegments * 2.0f * PI;
						float SinTheta = std::sin(Theta);
						float CosTheta = std::cos(Theta);

						FVector Pos(SinPhi * CosTheta, SinPhi * SinTheta, CosPhi);
						FVector Normal = Pos.GetSafeNormal();
						Pos = Pos * BoneSphereRadius;

						Vertices.Add({Pos, Normal});
					}
				}

				// 인덱스 생성
				for (int ring = 0; ring < NumRings; ++ring)
				{
					for (int seg = 0; seg < NumSegments; ++seg)
					{
						int Current = ring * (NumSegments + 1) + seg;
						int Next = Current + NumSegments + 1;

						Indices.Add(Current);
						Indices.Add(Next);
						Indices.Add(Current + 1);

						Indices.Add(Current + 1);
						Indices.Add(Next);
						Indices.Add(Next + 1);
					}
				}

				// 임시 버퍼 생성
				ID3D11Buffer* TempVB = nullptr;
				ID3D11Buffer* TempIB = nullptr;

				D3D11_BUFFER_DESC VBDesc = {};
				VBDesc.Usage = D3D11_USAGE_DEFAULT;
				VBDesc.ByteWidth = static_cast<UINT>(sizeof(FNormalVertex) * Vertices.Num());
				VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
				D3D11_SUBRESOURCE_DATA VBData = {};
				VBData.pSysMem = Vertices.GetData();
				Renderer.GetDevice()->CreateBuffer(&VBDesc, &VBData, &TempVB);

				D3D11_BUFFER_DESC IBDesc = {};
				IBDesc.Usage = D3D11_USAGE_DEFAULT;
				IBDesc.ByteWidth = static_cast<UINT>(sizeof(uint32) * Indices.Num());
				IBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
				D3D11_SUBRESOURCE_DATA IBData = {};
				IBData.pSysMem = Indices.GetData();
				Renderer.GetDevice()->CreateBuffer(&IBDesc, &IBData, &TempIB);

				// Primitive 설정 및 렌더링 (HitProxy 셰이더 사용)
				FEditorPrimitive PrimitiveInfo;
				PrimitiveInfo.Location = BoneWorldPos;
				PrimitiveInfo.Rotation = FQuat::Identity();
				PrimitiveInfo.Scale = FVector(1.0f, 1.0f, 1.0f);
				PrimitiveInfo.VertexBuffer = TempVB;
				PrimitiveInfo.NumVertices = static_cast<uint32>(Vertices.Num());
				PrimitiveInfo.IndexBuffer = TempIB;
				PrimitiveInfo.NumIndices = static_cast<uint32>(Indices.Num());
				PrimitiveInfo.Color = HitProxyColor;
				PrimitiveInfo.bShouldAlwaysVisible = true;
				PrimitiveInfo.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

				// HitProxy 셰이더 명시적 설정
				PrimitiveInfo.VertexShader = Renderer.GetHitProxyVS();
				PrimitiveInfo.PixelShader = Renderer.GetHitProxyPS();
				PrimitiveInfo.InputLayout = Renderer.GetHitProxyInputLayout();

				FRenderState RenderState = { ECullMode::Back, EFillMode::Solid };
				Renderer.RenderEditorPrimitive(PrimitiveInfo, RenderState, sizeof(FNormalVertex));

				// 버퍼 해제
				TempVB->Release();
				TempIB->Release();
			}

			// Bone 입체 메시 HitProxy 렌더링
			UBatchLines::FBoneMesh BoneMesh;
			UBatchLines::GenerateBoneMeshForHitProxy(
				Skeleton,
				GlobalPose,
				ComponentWorld,
				0.06f,  // WidthScale
				0.35f,  // BaseBias
				BoneMesh
			);

			// Bone 메시 렌더링 (각 삼각형마다 HitProxyID 설정)
			for (uint32 TriIdx = 0; TriIdx < BoneMesh.GetNumTriangles(); ++TriIdx)
			{
				int32 ParentBoneIndex = BoneMesh.BoneIndices[TriIdx];

				// HitProxyID를 색상으로 인코딩 (100 + ParentBoneIndex)
				uint32_t HitProxyID = 100 + ParentBoneIndex;
				uint8_t R = (HitProxyID) & 0xFF;
				uint8_t G = (HitProxyID >> 8) & 0xFF;
				uint8_t B = (HitProxyID >> 16) & 0xFF;
				FVector4 HitProxyColor(R / 255.0f, G / 255.0f, B / 255.0f, 1.0f);

				// 삼각형 3개 정점
				uint32 i0 = BoneMesh.Indices[TriIdx * 3 + 0];
				uint32 i1 = BoneMesh.Indices[TriIdx * 3 + 1];
				uint32 i2 = BoneMesh.Indices[TriIdx * 3 + 2];

				FVector v0 = BoneMesh.Vertices[i0];
				FVector v1 = BoneMesh.Vertices[i1];
				FVector v2 = BoneMesh.Vertices[i2];

				// 삼각형 Normal 계산
				FVector Edge1 = v1 - v0;
				FVector Edge2 = v2 - v0;
				FVector Normal = Edge1.Cross(Edge2).GetSafeNormal();

				// Vertex Buffer 생성
				TArray<FNormalVertex> TriVertices;
				TriVertices.Add({v0, Normal});
				TriVertices.Add({v1, Normal});
				TriVertices.Add({v2, Normal});

				TArray<uint32> TriIndices = {0, 1, 2};

				ID3D11Buffer* TriVB = nullptr;
				ID3D11Buffer* TriIB = nullptr;

				D3D11_BUFFER_DESC VBDesc = {};
				VBDesc.Usage = D3D11_USAGE_DEFAULT;
				VBDesc.ByteWidth = static_cast<UINT>(TriVertices.Num() * sizeof(FNormalVertex));
				VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

				D3D11_SUBRESOURCE_DATA VBData = {};
				VBData.pSysMem = TriVertices.GetData();
				Renderer.GetDevice()->CreateBuffer(&VBDesc, &VBData, &TriVB);

				D3D11_BUFFER_DESC IBDesc = {};
				IBDesc.Usage = D3D11_USAGE_DEFAULT;
				IBDesc.ByteWidth = static_cast<UINT>(TriIndices.Num() * sizeof(uint32));
				IBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

				D3D11_SUBRESOURCE_DATA IBData = {};
				IBData.pSysMem = TriIndices.GetData();
				Renderer.GetDevice()->CreateBuffer(&IBDesc, &IBData, &TriIB);

				// Primitive 설정 및 렌더링
				FEditorPrimitive PrimitiveInfo;
				PrimitiveInfo.Location = FVector(0, 0, 0);  // 이미 월드 좌표
				PrimitiveInfo.Rotation = FQuat::Identity();
				PrimitiveInfo.Scale = FVector(1.0f, 1.0f, 1.0f);
				PrimitiveInfo.VertexBuffer = TriVB;
				PrimitiveInfo.NumVertices = 3;
				PrimitiveInfo.IndexBuffer = TriIB;
				PrimitiveInfo.NumIndices = 3;
				PrimitiveInfo.Color = HitProxyColor;
				PrimitiveInfo.bShouldAlwaysVisible = true;
				PrimitiveInfo.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				PrimitiveInfo.VertexShader = Renderer.GetHitProxyVS();
				PrimitiveInfo.PixelShader = Renderer.GetHitProxyPS();
				PrimitiveInfo.InputLayout = Renderer.GetHitProxyInputLayout();

				FRenderState RenderState = {ECullMode::Back, EFillMode::Solid};
				Renderer.RenderEditorPrimitive(PrimitiveInfo, RenderState, sizeof(FNormalVertex));

				// 버퍼 해제
				TriVB->Release();
				TriIB->Release();
			}
		}
	}

	// RenderTarget 복원
	DeviceContext->OMSetRenderTargets(1, &OldRTV, OldDSV);
	DeviceContext->RSSetViewports(1, &OldViewport);

	if (OldRTV) OldRTV->Release();
	if (OldDSV) OldDSV->Release();

	// HitProxy 텍스처를 Staging으로 복사
	DeviceContext->CopyResource(HitProxyStagingTex.Get(), HitProxyRT.Get());

	// CPU에서 픽셀 읽기
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	if (SUCCEEDED(DeviceContext->Map(HitProxyStagingTex.Get(), 0, D3D11_MAP_READ, 0, &MappedResource)))
	{
		int X = static_cast<int>(LocalMousePos.x);
		int Y = static_cast<int>(LocalMousePos.y);

		if (X >= 0 && X < static_cast<int>(CachedSize.x) && Y >= 0 && Y < static_cast<int>(CachedSize.y))
		{
			uint8_t* PixelData = static_cast<uint8_t*>(MappedResource.pData);
			uint8_t* Pixel = PixelData + (Y * MappedResource.RowPitch) + (X * 4);

			uint32_t HitProxyID = (Pixel[0]) | (Pixel[1] << 8) | (Pixel[2] << 16);

			UE_LOG("FbxViewportWindow: HitProxyID at (%d,%d) = %d (RGB=%d,%d,%d)", X, Y, HitProxyID, Pixel[0], Pixel[1], Pixel[2]);

			// HitProxyID 처리
			if (HitProxyID >= 1 && HitProxyID <= 6)
			{
				// Gizmo 피킹 (HitProxyID = 1~6)
				EGizmoDirection Direction = static_cast<EGizmoDirection>(HitProxyID);
				if (PreviewGizmo)
				{
					PreviewGizmo->SetGizmoDirection(Direction);

					// 드래그 시작 처리 (충돌 지점 계산)
					FVector CollisionPoint = PreviewGizmo->GetGizmoLocation();
					PreviewGizmo->OnMouseDragStart(PreviewClient, CollisionPoint);

					UE_LOG("FbxViewportWindow: Gizmo picked - Direction=%d", static_cast<int>(Direction));
				}
			}
			else if (HitProxyID >= 100)
			{
				// Bone 피킹 (HitProxyID = 100 + BoneIndex)
				int32 BoneIndex = static_cast<int32>(HitProxyID - 100);
				USkeletalMeshComponent* PreviewComponent = PreviewScene->GetPreviewSkeletalComponent();
				if (PreviewComponent && PreviewComponent->GetSkeletalMesh())
				{
					const FSkeleton* Skeleton = PreviewComponent->GetSkeletalMesh()->GetSkeleton();
					if (BoneIndex >= 0 && BoneIndex < Skeleton->GetNumBones())
					{
						SelectBone(BoneIndex);
						UE_LOG("FbxViewportWindow: Bone picked - BoneIndex=%d, Name=%s", BoneIndex, Skeleton->BoneNames[BoneIndex].ToString().data());
					}
				}
			}
			else
			{
				// 빈 공간 클릭 - 선택 해제 (BoneEdit 모드일 때만)
				if (CurrentEditMode == EEditMode::BoneEdit)
				{
					DeselectBone();
					UE_LOG("FbxViewportWindow: Bone deselected (empty space clicked)");
				}
			}
		}

		DeviceContext->Unmap(HitProxyStagingTex.Get(), 0);
	}
}

void UFbxViewportWindow::RenderGizmoHitProxySimple()
{
	if (!PreviewGizmo || !PreviewClient)
	{
		return;
	}

	// Gizmo 위치 계산
	FVector GizmoLocation = PreviewGizmo->GetGizmoLocation();
	EGizmoMode GizmoMode = PreviewGizmo->GetGizmoMode();

	// Screen space scale 계산
	D3D11_VIEWPORT D3DViewport;
	D3DViewport.TopLeftX = 0;
	D3DViewport.TopLeftY = 0;
	D3DViewport.Width = CachedSize.x;
	D3DViewport.Height = CachedSize.y;
	D3DViewport.MinDepth = 0.0f;
	D3DViewport.MaxDepth = 1.0f;

	const float RenderScale = FGizmoMath::CalculateScreenSpaceScale(PreviewClient, D3DViewport, GizmoLocation, 120.0f);

	// Base rotation 계산
	FQuat BaseRot;
	USceneComponent* TargetComp = PreviewGizmo->GetTargetComponent();
	if (!TargetComp)
	{
		return;
	}

	if (GizmoMode == EGizmoMode::Scale)
	{
		BaseRot = TargetComp->GetWorldRotationAsQuaternion();
	}
	else
	{
		BaseRot = PreviewGizmo->IsWorldMode() ? FQuat::Identity() : TargetComp->GetWorldRotationAsQuaternion();
	}

	// Axis rotations (Translate/Scale 공통)
	FQuat AxisRots[3] = {
		FQuat::Identity(),
		FQuat::FromAxisAngle(FVector::UpVector(), FVector::GetDegreeToRadian(90.0f)),
		FQuat::FromAxisAngle(FVector::RightVector(), FVector::GetDegreeToRadian(-90.0f))
	};

	auto& Renderer = URenderer::GetInstance();
	UAssetManager& AssetManager = UAssetManager::GetInstance();

	// Primitive 타입별 메시 가져오기
	EPrimitiveType PrimitiveType;
	if (GizmoMode == EGizmoMode::Translate)
	{
		PrimitiveType = EPrimitiveType::Arrow;
	}
	else if (GizmoMode == EGizmoMode::Rotate)
	{
		// Rotate 모드는 동적 메시 생성이 필요하므로 간단히 스킵
		return;
	}
	else if (GizmoMode == EGizmoMode::Scale)
	{
		PrimitiveType = EPrimitiveType::CubeArrow;
	}
	else
	{
		return;
	}

	ID3D11Buffer* GizmoVertexBuffer = AssetManager.GetVertexBuffer(PrimitiveType);
	ID3D11Buffer* GizmoIndexBuffer = AssetManager.GetIndexBuffer(PrimitiveType);
	uint32 NumVertices = AssetManager.GetNumVertices(PrimitiveType);
	uint32 NumIndices = AssetManager.GetNumIndices(PrimitiveType);

	if (!GizmoVertexBuffer || !GizmoIndexBuffer)
	{
		return;
	}

	FRenderState RenderState = { ECullMode::Back, EFillMode::Solid };

	// X/Y/Z 축 렌더링 (HitProxyID 1/2/3)
	for (int AxisIdx = 0; AxisIdx < 3; ++AxisIdx)
	{
		FQuat AxisRotation = BaseRot * AxisRots[AxisIdx];

		// HitProxyID: 1=Forward(X), 2=Right(Y), 3=Up(Z)
		uint32_t HitProxyID = AxisIdx + 1;
		uint8_t R = (HitProxyID) & 0xFF;
		uint8_t G = (HitProxyID >> 8) & 0xFF;
		uint8_t B = (HitProxyID >> 16) & 0xFF;
		FVector4 HitProxyColor(R / 255.0f, G / 255.0f, B / 255.0f, 1.0f);

		FEditorPrimitive P;
		P.Location = GizmoLocation;
		P.Rotation = AxisRotation;
		P.Scale = FVector(RenderScale, RenderScale, RenderScale);
		P.Color = HitProxyColor;
		P.VertexBuffer = GizmoVertexBuffer;
		P.NumVertices = NumVertices;
		P.IndexBuffer = GizmoIndexBuffer;
		P.NumIndices = NumIndices;
		P.bShouldAlwaysVisible = true;
		P.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		// HitProxy 셰이더 설정
		P.VertexShader = Renderer.GetHitProxyVS();
		P.PixelShader = Renderer.GetHitProxyPS();
		P.InputLayout = Renderer.GetHitProxyInputLayout();

		Renderer.RenderEditorPrimitive(P, RenderState);
	}

	UE_LOG("FbxViewportWindow: RenderGizmoHitProxySimple - Mode=%d, Location=(%.1f,%.1f,%.1f)",
		static_cast<int>(GizmoMode), GizmoLocation.X, GizmoLocation.Y, GizmoLocation.Z);
}

FString UFbxViewportWindow::GetMaterialDisplayName(UMaterial* Material)
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

UTexture* UFbxViewportWindow::GetPreviewTextureForMaterial(const UMaterial* Material)
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

