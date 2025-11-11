#include "pch.h"
#include "Render/UI/Window/Public/FbxViewportWindow.h"
#include "Render/UI/Window/Public/PreviewScene.h"
#include "Render/UI/Widget/Public/SkeletalMeshComponentWidget.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Component/Mesh/Public/SkeletalMeshComponent.h"
#include "Manager/Input/Public/InputManager.h"
#include "Manager/UI/Public/ViewportManager.h"
#include "Render/UI/Viewport/Public/ViewportClient.h"
#include "Manager/Time/Public/TimeManager.h"
#include "Runtime/CoreUObject/Public/NewObject.h"
#include "ImGui/imgui.h"

IMPLEMENT_CLASS(UFbxViewportWindow, UUIWindow)

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

    UE_LOG("FbxViewportWindow: initialized");
}

void UFbxViewportWindow::Cleanup()
{
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
        PreviewClient = new FViewportClient();
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

	bHovered = ImGui::IsItemHovered();
	if (PreviewClient) PreviewClient->SetInputEnabled(bHovered);

	PreviewViewport->PumpMouseFromInputManager();
	const float DeltaTime = UTimeManager::GetInstance().GetDeltaTime();
	PreviewClient->UpdateEditorCamera(DeltaTime);

	UWorld* SceneWorld = PreviewScene ? PreviewScene->GetWorld() : nullptr;

	if (!PreviewViewport || !PreviewClient) return;

	URenderer::GetInstance().RenderExternalViewport(
		PreviewViewport, PreviewClient, RTV.Get(), DSV.Get(), SceneWorld);

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
	Config.UpdateWindowFlags();

	SetConfig(Config);
	SetWindowState(EUIWindowState::Hidden);
}

