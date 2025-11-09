#include "pch.h"
#include "Render/UI/Window/Public/FbxViewportWindow.h"
#include "Render/Renderer/Public/Renderer.h"
#include "ImGui/imgui.h"

IMPLEMENT_CLASS(UFbxViewportWindow, UUIWindow)

void UFbxViewportWindow::BuildPreviewSceneAfterImport()
{
	// Ground Plane
	{
		GroundActor = new AActor();
		auto* smc = GroundActor->AddComponent<UStaticMeshComponent>();
		smc->SetMesh(UPrimitives::MakePlane(5000, 5000)); // 너비/깊이
		smc->SetMaterial(UMaterial::GetDefaultGrid());
		smc->SetWorldTransform(FTransform(FVector(0,0,0)));
		PreviewScene->StaticComps.Add(smc);
	}

	// Directional Light
	{
		Sun = new ADirectionalLight();
		auto* lc = Sun->GetLightComponent();
		lc->SetColor(FLinearColor(1.0, 0.98, 0.9));
		lc->SetIntensity(3.0f);
		Sun->SetRotation(FRotator(-45, 35, 0));
		PreviewScene->Lights.Add(lc);
	}

	// Skeletal Actor는 LoadFbxFile() 완료 후 넣는다
}

void UFbxViewportWindow::RouteInputToClient()
{
	if (!bHovered || !PreviewClient) return;

	auto& Input = UInputManager::GetInstance();

	// 퍼스 카메라 동일 로직(요약)
	if (Input.IsKeyDown(EKeyInput::MouseRight))
	{
		// 회전
		FVector Rot = PreviewClient->GetViewRotation();
		const FVector md = Input.GetMouseDelta();
		Rot.Y += md.X * KeySensitivityDegPerPixel * 3;
		Rot.X += -md.Y * KeySensitivityDegPerPixel * 3;
		Rot.X = clamp(Rot.X, -89.9f, 89.9f);
		PreviewClient->SetViewRotation(Rot);

		// 이동
		const float Speed = UViewportManager::GetInstance().GetEditorCameraSpeed();
		// Forward/Right from rot (기존 코드 재사용)
		// ...
		// PreviewClient->SetViewLocation(NewLoc);
	}

	// 피킹: 좌클릭 시 레이 생성 → 프리뷰 씬으로 캡쳐
	if (Input.IsKeyPressed(EKeyInput::MouseLeft))
	{
		const FVector2 Mouse = Input.GetMousePosition();
		// 1) 창 내부 좌표 → NDC → 레이
		const D3D11_VIEWPORT vp = PreviewViewport->GetRenderRect();
		const FRay ray = DeprojectToRay(Mouse, vp, PreviewClient->GetView(), PreviewClient->GetProj());
		// 2) 씬 피킹: 메쉬/본(선분) 충돌 중 최소 히트
		FPickHit hit;
		if (PreviewScene->Pick(ray, hit))
		{
			// 선택 반영(본이면 본 인덱스, 메시면 프림 ID)
			PreviewClient->SetSelection(hit);
		}
	}

	// 휠 줌(오빗/또는 이동 레벨 변환)
	// ...
}

void UFbxViewportWindow::LoadFbxFile(const path& File)
{
	// 엔진 FBX 임포터를 사용(예: UFbxImporter::Import(File))
	UFbxImportResult R = UFbxImporter::Import(File);

	if (R.Type == EFbxType::Skeletal)
	{
		SkeletalActor = new AActor();
		auto* sk = SkeletalActor->AddComponent<USkeletalMeshComponent>();
		sk->SetSkeleton(R.Skeleton);
		sk->SetSkeletalMesh(R.SkeletalMesh);
		sk->SetWorldTransform(FTransform(FVector(0,0,0)));
		PreviewScene->SkelComps.Add(sk);

		// 모델 바운딩 기준으로 카메라 자리잡기
		const FBounds B = sk->GetBoundsWS();
		const FVector Focus = B.Center;
		const float   Dist  = std::max(300.f, B.Radius * 2.5f);

		PreviewClient->SetViewLocation(Focus + FVector(Dist, Dist, Dist*0.6f));
		PreviewClient->SetViewRotation(FVector(-20, 225, 0)); // pitch,yaw,roll
		PreviewClient->SetFOV(50.f);
	}
	else if (R.Type == EFbxType::Static)
	{
		// 정적 모델도 허용(스켈레탈 없는 FBX)
		auto* act = new AActor();
		auto* smc = act->AddComponent<UStaticMeshComponent>();
		smc->SetMesh(R.StaticMesh);
		smc->SetWorldTransform(FTransform(FVector(0,0,0)));
		PreviewScene->StaticComps.Add(smc);
	}

	// Ground/Light 등 기본 씬요소 보장
	if (!PreviewScene->GroundActor || !PreviewScene->Sun)
		BuildPreviewSceneAfterImport();
}

void UFbxViewportWindow::Initialize()
{
    // 1) 프리뷰 뷰포트/클라이언트 생성
    PreviewViewport = new FViewport();
    PreviewClient   = new FViewportClient();
    PreviewViewport->SetViewportClient(PreviewClient);
    PreviewClient->SetOwningViewport(PreviewViewport);

    // 카메라 기본(퍼스펙티브)
    PreviewClient->SetViewType(EViewType::Perspective);
    PreviewClient->SetViewMode(EViewModeIndex::VMI_BlinnPhong);

    // 2) 프리뷰 씬(미니 월드) 생성
    PreviewScene = new FPreviewScene(); // 아래 1-3 참고

    UE_LOG("FbxViewportWindow: initialized");
}

void UFbxViewportWindow::Release()
{
    // 렌더 타겟 해제
    SRV.Reset(); RTV.Reset(); ColorRT.Reset();
    DSV.Reset(); DepthTex.Reset();

    // 클라이언트/뷰포트
    if (PreviewViewport)
    {
        PreviewViewport->SetViewportClient(nullptr);
        SafeDelete(PreviewViewport);
    }
    SafeDelete(PreviewClient);

    // 씬
    SafeDelete(PreviewScene);
}

void UFbxViewportWindow::EnsureRenderTargets(const ImVec2& Size)
{
    const int w = (int)std::max(1.0f, Size.x);
    const int h = (int)std::max(1.0f, Size.y);
    if ((int)CachedViewportSize.x == w && (int)CachedViewportSize.y == h && SRV) return;

    CachedViewportSize = ImVec2((float)w, (float)h);
    SRV.Reset(); RTV.Reset(); ColorRT.Reset();
    DSV.Reset(); DepthTex.Reset();

    auto* Device = URenderer::GetInstance().GetDevice();
    auto* DeviceContext = URenderer::GetInstance().GetDeviceContext();

    // Color RT
    D3D11_TEXTURE2D_DESC td = {};
    td.Width  = w; td.Height = h;
    td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc = {1,0};
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    Device->CreateTexture2D(&td, nullptr, ColorRT.ReleaseAndGetAddressOf());
    Device->CreateRenderTargetView(ColorRT.Get(), nullptr, RTV.ReleaseAndGetAddressOf());
    Device->CreateShaderResourceView(ColorRT.Get(), nullptr, SRV.ReleaseAndGetAddressOf());

    // Depth
    td.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    Device->CreateTexture2D(&td, nullptr, DepthTex.ReleaseAndGetAddressOf());
    Device->CreateDepthStencilView(DepthTex.Get(), nullptr, DSV.ReleaseAndGetAddressOf());

    // 뷰포트 크기 동기화
    FRect R{0,0,w,h};
    PreviewViewport->SetRect(R);
}

void UFbxViewportWindow::OnPostRenderWindow()
{
    // 1) 공간 확보
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x < 1 || avail.y < 1)
    {
        ImGui::TextDisabled("No space to render viewport");
        return;
    }

    // 2) RT 보장 + 뷰포트 렌더
    EnsureRenderTargets(avail);
    RenderPreview();               // (아래 RenderPreview 참고)

    // 3) ImGui에 SRV로 붙여 그리기
    ImGui::InvisibleButton("FbxViewportHit", avail);
    const bool hovered = ImGui::IsItemHovered();
    const ImVec2 p0    = ImGui::GetItemRectMin();
    const ImVec2 p1    = ImGui::GetItemRectMax();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddImage((ImTextureID)SRV.Get(), p0, p1);

    bHovered = hovered;

	ImGui::TextColored(ImVec4(0.6f, 0.85f, 1.0f, 1.0f), "FBX Viewport Preview");
	ImGui::Separator();
	ImGui::TextWrapped("Viewport Manager integration will send a live FBX scene to this popup later.");
	ImGui::Spacing();

	RenderPlaceholderViewport();
}

void UFbxViewportWindow::RenderPreview()
{
    // 프리뷰 카메라/클라이언트 입력 처리
    RouteInputToClient();

    // Renderer에 “외부 뷰포트” 렌더 호출
    URenderer::GetInstance().RenderExternalViewport(
        PreviewViewport, PreviewClient, RTV.Get(), DSV.Get(),
        PreviewScene // 씬 핸들(아래 1-3)
    );
}

UFbxViewportWindow::UFbxViewportWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "FBX Viewport";
	Config.DefaultSize = ImVec2(720, 480);
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

void UFbxViewportWindow::RenderPlaceholderViewport() const
{
	const ImVec2 CanvasSize = ImGui::GetContentRegionAvail();
	if (CanvasSize.x <= 1.0f || CanvasSize.y <= 1.0f)
	{
		return;
	}

	const ImVec2 CanvasMin = ImGui::GetCursorScreenPos();
	const ImVec2 CanvasMax(CanvasMin.x + CanvasSize.x, CanvasMin.y + CanvasSize.y);

	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	DrawList->AddRectFilled(CanvasMin, CanvasMax, IM_COL32(20, 20, 20, 255));
	DrawList->AddRect(CanvasMin, CanvasMax, IM_COL32(95, 95, 95, 255), 4.0f, 0, 2.0f);

	static constexpr const char* PlaceholderText = "FBX viewport rendering is not wired yet.";
	const ImVec2 TextSize = ImGui::CalcTextSize(PlaceholderText);
	const ImVec2 TextPos(CanvasMin.x + (CanvasSize.x - TextSize.x) * 0.5f,
		CanvasMin.y + (CanvasSize.y - TextSize.y) * 0.5f);
	DrawList->AddText(TextPos, IM_COL32(200, 200, 200, 255), PlaceholderText);

	ImGui::InvisibleButton("FbxViewportPlaceholder", CanvasSize);
}
