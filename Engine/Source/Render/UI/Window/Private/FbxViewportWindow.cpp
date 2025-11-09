#include "pch.h"
#include "Render/UI/Window/Public/FbxViewportWindow.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Manager/Input/Public/InputManager.h"
#include "Manager/UI/Public/ViewportManager.h"
#include "Render/UI/Viewport/Public/ViewportClient.h"
#include "ImGui/imgui.h"

IMPLEMENT_CLASS(UFbxViewportWindow, UUIWindow)

void UFbxViewportWindow::RouteInputToClient()
{
	if (!bHovered || !PreviewClient || !PreviewViewport)
	{
		return;
	}

	auto& Input = UInputManager::GetInstance();

	if (Input.IsKeyDown(EKeyInput::MouseRight))
	{
		FVector Rot = PreviewClient->GetViewRotation();
		const FVector md = Input.GetMouseDelta();
		Rot.Y += md.X * KeySensitivityDegPerPixel * 3;
		Rot.X += -md.Y * KeySensitivityDegPerPixel * 3;
		Rot.X = clamp(Rot.X, -89.9f, 89.9f);
		PreviewClient->SetViewRotation(Rot);

		// TODO: hook movement once the preview viewport mirrors editor controls.
	}

	if (Input.IsKeyPressed(EKeyInput::MouseLeft))
	{
		// Picking is intentionally left as a stub until preview rendering is available.
	}
}

void UFbxViewportWindow::LoadFbxFile(const path& File)
{
	const std::string FileName = File.string();
	UE_LOG_WARNING("FbxViewportWindow: LoadFbxFile is not implemented yet (%s).", FileName.c_str());
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

    UE_LOG("FbxViewportWindow: initialized");
}

void UFbxViewportWindow::Tick(float /*DeltaTime*/)
{
	// Preview viewport updates are handled during RenderPreview.
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
}

void UFbxViewportWindow::EnsureRenderTargets(const ImVec2& Size)
{
    const int w = (int)std::max(1.0f, Size.x);
    const int h = (int)std::max(1.0f, Size.y);
    if ((int)CachedViewportSize.x == w && (int)CachedViewportSize.y == h && SRV)
    {
        return;
    }

    CachedViewportSize = ImVec2((float)w, (float)h);
    SRV.Reset(); RTV.Reset(); ColorRT.Reset();
    DSV.Reset(); DepthTex.Reset();

    auto* Device = URenderer::GetInstance().GetDevice();
    if (!Device)
    {
        return;
    }

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

    if (!PreviewViewport)
    {
        return;
    }

    // 뷰포트 크기 동기화
    FRect R{0,0,w,h};
    PreviewViewport->SetRect(R);

    D3D11_VIEWPORT View = {};
    View.TopLeftX = 0.0f;
    View.TopLeftY = 0.0f;
    View.Width = static_cast<float>(w);
    View.Height = static_cast<float>(h);
    View.MinDepth = 0.0f;
    View.MaxDepth = 1.0f;
    PreviewViewport->SetRenderRect(View);

    if (PreviewClient)
    {
        const FPoint NewViewportSize{ w, h };
        PreviewClient->OnResize(NewViewportSize);
    }
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
    RenderPreview();

    if (!SRV)
    {
        ImGui::TextDisabled("FBX viewport render target is not ready.");
        return;
    }

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
    RouteInputToClient();

    if (!PreviewViewport || !PreviewClient || !RTV || !DSV)
    {
        return;
    }

    static bool bLoggedWorldWarning = false;
    if (!GWorld || !GWorld->GetLevel())
    {
        if (!bLoggedWorldWarning)
        {
            UE_LOG_WARNING("FbxViewportWindow: GWorld or its level is not ready for rendering.");
            bLoggedWorldWarning = true;
        }
        return;
    }
    bLoggedWorldWarning = false;

    URenderer::GetInstance().RenderExternalViewport(
        PreviewViewport,
        PreviewClient,
        RTV.Get(),
        DSV.Get(),
        GWorld
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
