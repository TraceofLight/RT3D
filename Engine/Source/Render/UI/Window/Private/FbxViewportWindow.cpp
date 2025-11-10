#include "pch.h"
#include "Render/UI/Window/Public/FbxViewportWindow.h"
#include "Core/Public/NewObject.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Manager/Input/Public/InputManager.h"
#include "Manager/UI/Public/ViewportManager.h"
#include "Render/UI/Viewport/Public/ViewportClient.h"
#include "ImGui/imgui.h"
#include "Level/Public/Level.h"
#include "Level/Public/World.h"
#include "Actor/Public/StaticMeshActor.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Public/AmbientLightComponent.h"
#include "Component/Public/UUIDTextComponent.h"

IMPLEMENT_CLASS(UFbxViewportWindow, UUIWindow)

void UFbxViewportWindow::LoadFbxFile(const path& File)
{
	const std::string FileName = File.string();
	UE_LOG_WARNING("FbxViewportWindow: LoadFbxFile is not implemented yet (%s).", FileName.c_str());
}

void UFbxViewportWindow::Initialize()
{
    PreviewViewport = new FViewport();
    PreviewClient   = new FViewportClient();
    PreviewViewport->SetViewportClient(PreviewClient);
    PreviewClient->SetOwningViewport(PreviewViewport);
    PreviewClient->SetViewType(EViewType::Perspective);
    PreviewClient->SetViewMode(EViewModeIndex::VMI_BlinnPhong);

	PreviewClient->EnableEditorCamera(true);
	PreviewClient->SetViewLocation(FVector(0, -300, 150));
	PreviewClient->SetViewRotation(FVector(-15, 0, 0));

	CreatePreviewWorld();
	InjectTestMeshIntoPreviewWorld();

    UE_LOG("FbxViewportWindow: initialized");
}

void UFbxViewportWindow::Release()
{
	RemoveInjectedTestMesh();
	DestroyPreviewWorld();

    if (PreviewViewport)
    {
        PreviewViewport->SetViewportClient(nullptr);
        SafeDelete(PreviewViewport);
    }
    SafeDelete(PreviewClient);
}

void UFbxViewportWindow::Tick(float DeltaTime)
{
	if (!PreviewViewport || !PreviewClient) return;
	if (PreviewWorld)
	{
		PreviewWorld->Tick(DeltaTime);
	}
	PreviewViewport->PumpMouseFromInputManager();
	PreviewClient->UpdateEditorCamera(DeltaTime);
}

void UFbxViewportWindow::EnsureRenderTargets(const ImVec2& size)
{
	const UINT w = (UINT)std::max(1.0f, size.x);
	const UINT h = (UINT)std::max(1.0f, size.y);
	if ((UINT)CachedSize.x == w && (UINT)CachedSize.y == h && SRV) return;

	CachedSize = ImVec2((float)w, (float)h);
	SRV.Reset(); RTV.Reset(); ColorRT.Reset();
	DSV.Reset(); DepthTex.Reset();

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

	// 프리뷰 뷰포트에도 크기 반영
	if (PreviewViewport)
		PreviewViewport->SetRect({0,0,(int)w,(int)h});
}

void UFbxViewportWindow::OnPostRenderWindow()
{
	const ImVec2 avail = ImGui::GetContentRegionAvail();
	if (avail.x < 1 || avail.y < 1) return;

	EnsureRenderTargets(avail);      // 전용 RT/DSV 준비

	// ImGui에 전용 SRV를 그리기  (절대 백버퍼 SRV를 쓰지 말 것!)
	ImGui::InvisibleButton("FBXViewportArea", avail);
	const ImVec2 p0 = ImGui::GetItemRectMin();
	const ImVec2 p1 = ImGui::GetItemRectMax();
	ImDrawList* dl = ImGui::GetWindowDrawList();
	dl->AddImage((ImTextureID)SRV.Get(), p0, p1);

	bHovered = ImGui::IsItemHovered();
	if (PreviewClient) PreviewClient->SetInputEnabled(bHovered);

	// 프리뷰 렌더 호출 (아래 3단계 참고)
	URenderer::GetInstance().RenderExternalViewport(
		PreviewViewport, PreviewClient, RTV.Get(), DSV.Get(), /*WorldOverride*/PreviewWorld);
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

void UFbxViewportWindow::InjectTestMeshIntoPreviewWorld()
{
	if (bPreviewInjected || !PreviewWorld) { return; }

	PreviewTestActor = PreviewWorld->SpawnActor(AStaticMeshActor::StaticClass());
	if (!PreviewTestActor)
	{
		UE_LOG_ERROR("FbxViewportWindow: Failed to spawn preview actor in mini world.");
		return;
	}

	// TODO: Attach FBX mesh component once importer is ready
	bPreviewInjected = true;
	UE_LOG("FbxViewportWindow: spawned preview actor inside mini world.");
}

void UFbxViewportWindow::RemoveInjectedTestMesh()
{
	if (!bPreviewInjected || !PreviewWorld)
	{
		return;
	}

	// 필요시 Level에서 등록 해제 API 호출
	if (PreviewTestActor)
	{
		PreviewWorld->DestroyActor(PreviewTestActor);
	}

	PreviewTestMesh  = nullptr;
	PreviewTestActor = nullptr;
	PreviewAmLight   = nullptr;
	bPreviewInjected = false;
}

void UFbxViewportWindow::CreatePreviewWorld()
{
	if (PreviewWorld)
	{
		return;
	}

	PreviewWorld = NewObject<UWorld>(this);
	if (!PreviewWorld)
	{
		UE_LOG_ERROR("FbxViewportWindow: Failed to allocate preview world.");
		return;
	}

	PreviewWorld->SetWorldType(EWorldType::EditorPreview);
	PreviewWorld->CreateNewLevel();
	UE_LOG("FbxViewportWindow: preview world created.");
}

void UFbxViewportWindow::DestroyPreviewWorld()
{
	if (!PreviewWorld)
	{
		return;
	}

	PreviewWorld->EndPlay();
	SafeDelete(PreviewWorld);
	PreviewWorld = nullptr;
	UE_LOG("FbxViewportWindow: preview world destroyed.");
}
