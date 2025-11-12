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
#include "ImGui/imgui.h"

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

	// BoneTransformProxy 생성
	if (!BoneTransformProxy)
	{
		BoneTransformProxy = NewObject<UBoneTransformProxy>(this);
	}

	BoneTransformProxy->SetBoneInfo(PreviewComponent, BoneIndex);
	BoneTransformProxy->SyncTransformFromBone();

	// PreviewClient의 Gizmo에 타겟 설정
	if (PreviewClient && PreviewClient->GetGizmo())
	{
		PreviewClient->GetGizmo()->SetSelectedComponent(BoneTransformProxy);
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
		// PreviewClient의 Gizmo 타겟 해제
		if (PreviewClient && PreviewClient->GetGizmo())
		{
			PreviewClient->GetGizmo()->SetSelectedComponent(nullptr);
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
	PreviewComponent->UseReferencePose();

	if (SkeletalWidget)
	{
		SkeletalWidget->SetTargetComponent(PreviewComponent);
	}
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
	if (!bPreviewReady)
	{
		return;
	}

	UpdateSkeletalWidgetTargets();

	const ImVec2 totalAvail = ImGui::GetContentRegionAvail();
	if (totalAvail.x < 1 || totalAvail.y < 1) return;

	const bool bHasInspector = (SkeletalWidget && PreviewScene && PreviewScene->GetPreviewSkeletalComponent());
	const float desiredInspectorWidth = 320.0f;
	float inspectorWidth = bHasInspector ? desiredInspectorWidth : 0.0f;
	float viewportWidth = totalAvail.x;
	const float spacing = bHasInspector ? ImGui::GetStyle().ItemSpacing.x : 0.0f;

	if (inspectorWidth > 0.0f && totalAvail.x > (inspectorWidth + spacing + 50.0f))
	{
		viewportWidth = totalAvail.x - inspectorWidth - spacing;
	}
	else
	{
		inspectorWidth = 0.0f;
	}

	RenderPreviewViewport(ImVec2(viewportWidth, totalAvail.y));

	if (inspectorWidth > 0.0f)
	{
		ImGui::SameLine();
		RenderSkeletalInspector(ImVec2(inspectorWidth, totalAvail.y));
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
		if (ImGui::IsKeyPressed(ImGuiKey_Space))
		{
			PreviewClient->InputKey(EKeyInput::Space, true);
		}

		// 마우스 버튼 Pressed 처리
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			PreviewViewport->HandleMouseDown(0, LocalX, LocalY);
			// ViewportClient의 HandleClick 호출 (기즈모/오브젝트 피킹)
			PreviewClient->HandleClick(LocalX, LocalY);
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
			PreviewClient->InputKey(EKeyInput::MouseLeft, false);
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
			// 기즈모 드래그 처리
			ImVec2 MouseDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.0f);
			PreviewClient->ProcessGizmoDrag(FVector2(MouseDelta.x, MouseDelta.y));
			ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
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
	if (CurrentEditMode == EEditMode::BoneEdit && PreviewScene)
	{
		UGizmo* PreviewGizmo = PreviewScene->GetPreviewGizmo();
		if (PreviewGizmo && PreviewGizmo->HasComponent())
		{
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

			// Gizmo 렌더링 (PreviewClient의 Gizmo 사용)
			if (PreviewClient && PreviewClient->UsesTransformGizmo())
			{
				UGizmo* Gizmo = PreviewClient->GetGizmo();
				if (Gizmo && Gizmo->HasComponent())
				{
					// FbxViewportWindow의 RTV/DSV 설정
					DeviceContext->OMSetRenderTargets(1, RTV.GetAddressOf(), DSV.Get());
					DeviceContext->RSSetViewports(1, &D3DViewport);

					Gizmo->UpdateScale(PreviewClient, D3DViewport);
					Gizmo->RenderGizmo(PreviewClient, D3DViewport);

					// RenderTarget 복원
					DeviceContext->OMSetRenderTargets(1, &OldRTV, OldDSV);
					DeviceContext->RSSetViewports(1, &OldViewport);

					if (OldRTV) OldRTV->Release();
					if (OldDSV) OldDSV->Release();
				}
			}
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

	// 이 줄 추가: 뷰포트 밖으로 나갈 수 있도록 설정
	Config.WindowFlags |= ImGuiWindowFlags_NoNav;  // 네비게이션 비활성화

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

void UFbxViewportWindow::HandleMouseClick(const ImVec2& LocalMousePos)
{
	if (!PreviewScene || !PreviewClient || !HitProxyRTV || !HitProxyStagingTex)
	{
		return;
	}

	UGizmo* PreviewGizmo = PreviewScene->GetPreviewGizmo();
	if (!PreviewGizmo)
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

	// Gizmo HitProxy 렌더링
	PreviewGizmo->RenderForHitProxy(PreviewClient, D3DViewport);

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

			UE_LOG("FbxViewportWindow: HandleMouseClick at (%.0f, %.0f), HitProxyID=%u",
				LocalMousePos.x, LocalMousePos.y, HitProxyID);

			// TODO: HitProxyID로 Gizmo 액션 처리
		}

		DeviceContext->Unmap(HitProxyStagingTex.Get(), 0);
	}
}

