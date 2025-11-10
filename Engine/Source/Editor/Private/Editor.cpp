#include "pch.h"
#include "Editor/Public/Editor.h"

#include "Editor/Public/Axis.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Manager/UI/Public/UIManager.h"
#include "Manager/Input/Public/InputManager.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Manager/Time/Public/TimeManager.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Public/ShapeComponent.h"
#include "Level/Public/Level.h"
#include "Global/Quaternion.h"
#include "Utility/Public/ScopeCycleCounter.h"
#include "Render/UI/Overlay/Public/StatOverlay.h"
#include "Component/Public/DecalComponent.h"
#include "Component/Public/DecalSpotLightComponent.h"
#include "Component/Public/PointLightComponent.h"
#include "Physics/Public/BoundingSphere.h"
#include "Component/Public/DirectionalLightComponent.h"
#include "Component/Public/SpotLightComponent.h"
#include "Component/Public/EditorIconComponent.h"
#include "Component/Public/BillBoardComponent.h"
#include "Manager/UI/Public/ViewportManager.h"
#include "Render/UI/Overlay/Public/D2DOverlayManager.h"
#include "Render/ui/Viewport/Public/ViewportClient.h"
#include "Render/UI/Viewport/Public/Viewport.h"

IMPLEMENT_CLASS(UEditor, UObject)

UEditor::UEditor()
{
	int32 ActiveIndex = UViewportManager::GetInstance().GetActiveIndex();
	ActiveViewportIndex = ActiveIndex;
}

UEditor::~UEditor()
{
	// 레거시 코드 제거: ViewportManager가 뷰포트 관리
}

void UEditor::Update()
{
	UViewportManager& ViewportManager = UViewportManager::GetInstance();
	const UInputManager& Input = UInputManager::GetInstance();

	// 활성 뷰포트 인덱스 업데이트
	int32 ActiveIndex = ViewportManager.GetActiveIndex();
	if (ActiveViewportIndex != ActiveIndex)
	{
		ActiveViewportIndex = ActiveIndex;
	}

	// KTLWeek07: 각 뷰포트에서 마우스 우클릭 시 해당 카메라만 입력 활성화
	int32 HoveredViewportIndex = ViewportManager.GetMouseHoveredViewportIndex();
	bool bIsRightMouseDown = Input.IsKeyDown(EKeyInput::MouseRight);

	// 드래그 시작 시 뷰포트 고정, 드래그 종료 시 해제
	if (bIsRightMouseDown && !bWasRightMouseDown)
	{
		// 우클릭 시작: 현재 호버된 뷰포트를 잠금
		LockedViewportIndexForDrag = HoveredViewportIndex;
	}
	else if (!bIsRightMouseDown && bWasRightMouseDown)
	{
		// 우클릭 종료: 잠금 해제
		LockedViewportIndexForDrag = -1;
	}
	bWasRightMouseDown = bIsRightMouseDown;

	// 드래그 중이면 잠긴 뷰포트 사용, 아니면 호버된 뷰포트 사용
	int32 ActiveViewportIndexForInput = (LockedViewportIndexForDrag >= 0)
		                                    ? LockedViewportIndexForDrag
		                                    : HoveredViewportIndex;

	if (ViewportManager.GetViewportLayout() == EViewportLayout::Quad)
	{
		// Quad 모드: 마우스 우클릭 중이고 해당 뷰포트 위에 있을 때만 그 카메라 입력 활성화
		FViewportClient* ActiveOrthoClient = nullptr;

		// PIE 마우스 detach 상태 확인
		int32 PIEViewportIndex = -1;
		bool bIsPIEMouseDetached = (GEditor && GEditor->IsPIEMouseDetached());
		if (bIsPIEMouseDetached)
		{
			PIEViewportIndex = ViewportManager.GetPIEActiveViewportIndex();
		}

		for (int32 i = 0; i < 4; ++i)
		{
			FViewportClient* Client = ViewportManager.GetClients()[i];
			if (Client)
			{
				// PIE 마우스 detach 상태일 때 PIE 뷰포트는 입력 비활성화 유지
				if (bIsPIEMouseDetached && i == PIEViewportIndex)
				{
					Client->SetInputEnabled(false);
					continue;
				}

				// 마우스 우클릭 중이고 해당 뷰포트가 활성화된 뷰포트면 입력 활성화
				bool bEnableInput = (ActiveViewportIndexForInput == i && bIsRightMouseDown);
				Client->SetInputEnabled(bEnableInput);

				// 오쏘 뷰가 활성화되었고 이동이 있었다면 기록
				if (bEnableInput && Client->IsOrtho())
				{
					ActiveOrthoClient = Client;
				}
			}
		}

		// 오쏘 뷰 드래그 시 모든 오쏘 뷰를 공유 중심점 기준으로 업데이트
		// PIE 마우스 detach 상태에서는 PIE 뷰포트 동기화 스킵
		if (ActiveOrthoClient && bIsRightMouseDown)
		{
			// ViewportClient의 UpdateInput에서 이미 ViewLocation이 업데이트됨
			// 공유 중심점 업데이트
			FVector CurrentLocation = ActiveOrthoClient->GetViewLocation();

			// ViewType에 따라 InitialOffsets 인덱스 결정
			int32 OrthoIdx = -1;
			switch (ActiveOrthoClient->GetViewType())
			{
			case EViewType::OrthoTop: OrthoIdx = 0;
				break;
			case EViewType::OrthoBottom: OrthoIdx = 1;
				break;
			case EViewType::OrthoLeft: OrthoIdx = 2;
				break;
			case EViewType::OrthoRight: OrthoIdx = 3;
				break;
			case EViewType::OrthoFront: OrthoIdx = 4;
				break;
			case EViewType::OrthoBack: OrthoIdx = 5;
				break;
			}

			if (OrthoIdx >= 0 && OrthoIdx < ViewportManager.GetInitialOffsets().Num())
			{
				// 공유 중심점 = 현재 위치 - 초기 오프셋
				ViewportManager.SetOrthoGraphicCameraPoint(
					CurrentLocation - ViewportManager.GetInitialOffsets()[OrthoIdx]);

				// 모든 오쏘 뷰를 공유 중심점 기준으로 업데이트
				// PIE 마우스 detach 상태일 때는 PIE 뷰포트 제외
				int32 PIEViewportIndex = -1;
				bool bShouldSkipPIEViewport = (GEditor && GEditor->IsPIEMouseDetached());
				if (bShouldSkipPIEViewport)
				{
					PIEViewportIndex = ViewportManager.GetPIEActiveViewportIndex();
				}

				for (int32 i = 0; i < 4; ++i)
				{
					// PIE 마우스 detach 상태에서 PIE 뷰포트는 스킵
					if (bShouldSkipPIEViewport && i == PIEViewportIndex)
					{
						continue;
					}

					if (ViewportManager.GetClients()[i] && ViewportManager.GetClients()[i]->IsOrtho())
					{
						FViewportClient* Client = ViewportManager.GetClients()[i];
						int32 ClientOrthoIdx = -1;
						switch (Client->GetViewType())
						{
						case EViewType::OrthoTop: ClientOrthoIdx = 0;
							break;
						case EViewType::OrthoBottom: ClientOrthoIdx = 1;
							break;
						case EViewType::OrthoLeft: ClientOrthoIdx = 2;
							break;
						case EViewType::OrthoRight: ClientOrthoIdx = 3;
							break;
						case EViewType::OrthoFront: ClientOrthoIdx = 4;
							break;
						case EViewType::OrthoBack: ClientOrthoIdx = 5;
							break;
						}

						if (ClientOrthoIdx >= 0 && ClientOrthoIdx < ViewportManager.GetInitialOffsets().Num())
						{
							FVector NewLocation = ViewportManager.GetOrthoGraphicCameraPoint() + ViewportManager.
								GetInitialOffsets()[ClientOrthoIdx];
							Client->SetViewLocation(NewLocation);
						}
					}
				}
			}
		}
	}
	else
	{
		// 싱글 모드: 뷰포트 위에서 마우스 우클릭 시 입력 활성화
		if (ActiveViewportIndexForInput >= 0 && ViewportManager.GetClients()[ActiveViewportIndexForInput])
		{
			FViewportClient* Client = ViewportManager.GetClients()[ActiveViewportIndexForInput];

			// PIE 마우스 detach 상태일 때는 입력 비활성화
			if (GEditor && GEditor->IsPIEMouseDetached())
			{
				Client->SetInputEnabled(false);
			}
			else
			{
				// Single 모드에서는 ActiveViewportIndexForInput이 유효한 뷰포트면 입력 활성화
				bool bEnableInput = bIsRightMouseDown;
				Client->SetInputEnabled(bEnableInput);
			}
		}
	}

	UpdateBatchLines();
	BatchLines.UpdateVertexBuffer();

	UpdateCameraAnimation();

	// Pilot Mode 키 바인딩 처리
	UInputManager& InputManager = UInputManager::GetInstance();

	// Ctrl + Shift + P: Pilot Mode 진입
	if (InputManager.IsKeyPressed(EKeyInput::P) &&
		InputManager.IsKeyDown(EKeyInput::Ctrl) &&
		InputManager.IsKeyDown(EKeyInput::Shift))
	{
		if (!bIsPilotMode)
		{
			TogglePilotMode();
		}
	}

	// Alt + G: Pilot Mode 해제
	if (InputManager.IsKeyPressed(EKeyInput::G) &&
		InputManager.IsKeyDown(EKeyInput::Alt))
	{
		if (bIsPilotMode)
		{
			ExitPilotMode();
		}
	}

	// Pilot Mode 업데이트
	if (bIsPilotMode)
	{
		UpdatePilotMode();
	}

	// 뷰포트 카메라 입력 처리
	UpdateViewportCameraInput();

	ProcessMouseInput();
}

void UEditor::Collect2DRender(FViewportClient* InClient, const D3D11_VIEWPORT& InViewport, bool bIsPIEViewport)
{
	// D2D 드로잉 정보 수집 시작
	FD2DOverlayManager& OverlayManager = FD2DOverlayManager::GetInstance();
	OverlayManager.BeginCollect(InClient, InViewport);

	if (!bIsPIEViewport)
	{
		// FAxis 렌더링 명령 수집
		FAxis::CollectDrawCommands(OverlayManager, InClient, InViewport);
	}

	// Gizmo 회전 각도 오버레이 수집
	Gizmo.CollectRotationAngleOverlay(OverlayManager, InClient, InViewport);

	// StatOverlay 렌더링 명령 수집
	UStatOverlay::GetInstance().Render();
}

void UEditor::RenderEditorGeometry()
{
	// 3D 지오메트리 렌더링 (Grid, AABB, Light Lines, Octree 등)
	BatchLines.Render();
}

void UEditor::RenderGizmo(FViewportClient* InClient, const D3D11_VIEWPORT& InViewport)
{
	Gizmo.RenderGizmo(InClient, InViewport);

	// 모든 DirectionalLight의 빛 방향 기즈모 렌더링 (선택 여부 무관)
	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (EditorWorld && EditorWorld->GetLevel())
	{
		ULevel* CurrentLevel = EditorWorld->GetLevel();
		const TArray<ULightComponent*>& LightComponents = CurrentLevel->GetLightComponents();
		for (ULightComponent* LightComp : LightComponents)
		{
			// Pilot Mode: 현재 조종 중인 Actor의 컴포넌트는 화살표 렌더링 스킵
			if (bIsPilotMode && PilotedActor && LightComp->GetOwner() == PilotedActor)
			{
				continue;
			}

			if (UDirectionalLightComponent* DirLight = Cast<UDirectionalLightComponent>(LightComp))
			{
				DirLight->RenderLightDirectionGizmo(InClient, InViewport);
			}
			if (USpotLightComponent* SpotLight = Cast<USpotLightComponent>(LightComp))
			{
				SpotLight->RenderLightDirectionGizmo(InClient, InViewport);
			}
		}
	}
}

void UEditor::RenderGizmoForHitProxy(FViewportClient* InClient, const D3D11_VIEWPORT& InViewport)
{
	if (Gizmo.HasComponent())
	{
		Gizmo.RenderForHitProxy(InClient, InViewport);
	}
}

void UEditor::UpdateBatchLines()
{
	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!EditorWorld || !EditorWorld->GetLevel()) { return; }

	uint64 ShowFlags = EditorWorld->GetLevel()->GetShowFlags();

	if (ShowFlags & EEngineShowFlags::SF_Octree)
	{
		BatchLines.UpdateOctreeVertices(EditorWorld->GetLevel()->GetStaticOctree());
	}
	else
	{
		// If we are not showing the octree, clear the lines, so they don't persist
		BatchLines.ClearOctreeLines();
	}

	if (UActorComponent* Component = GetSelectedComponent())
	{
		// Handle ShapeComponent collision visualization
		if (UShapeComponent* ShapeComponent = Cast<UShapeComponent>(Component))
		{
			if (ShowFlags & EEngineShowFlags::SF_Collision)
			{
				// Polymorphic rendering - each shape knows how to render itself
				ShapeComponent->RenderDebugShape(BatchLines);
				return;
			}
		}

		if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component))
		{
			if (ShowFlags & EEngineShowFlags::SF_Bounds)
			{
				if (PrimitiveComponent->GetBoundingBox()->GetType() == EBoundingVolumeType::AABB)
				{
					FVector WorldMin, WorldMax;
					PrimitiveComponent->GetWorldAABB(WorldMin, WorldMax);
					FAABB AABB(WorldMin, WorldMax);
					BatchLines.UpdateBoundingBoxVertices(&AABB);
				}
				else
				{
					BatchLines.UpdateBoundingBoxVertices(PrimitiveComponent->GetBoundingBox());

					// 만약 선택된 타입이 decalspotlightcomponent라면
					if (Component->IsA(UDecalSpotLightComponent::StaticClass()))
					{
						BatchLines.UpdateDecalSpotLightVertices(Cast<UDecalSpotLightComponent>(Component));
					}
				}
				return;
			}
		}
		if (ULightComponent* LightComponent = Cast<ULightComponent>(Component))
		{
			if (UPointLightComponent* PointLightComponent = Cast<UPointLightComponent>(Component))
			{
				if (USpotLightComponent* SpotLightComponent = Cast<USpotLightComponent>(Component))
				{
					const FVector Center = SpotLightComponent->GetWorldLocation();
					const float Radius = SpotLightComponent->GetAttenuationRadius();
					const float OuterRadian = SpotLightComponent->GetOuterConeAngle();
					const float InnerRadian = SpotLightComponent->GetInnerConeAngle();
					FQuaternion Rotation = SpotLightComponent->GetWorldRotationAsQuaternion();
					BatchLines.UpdateConeVertices(Center, Radius, OuterRadian, InnerRadian, Rotation);
					return;
				}
				const FVector Center = PointLightComponent->GetWorldLocation();
				const float Radius = PointLightComponent->GetAttenuationRadius();

				FBoundingSphere PointSphere(Center, Radius);
				BatchLines.UpdateBoundingBoxVertices(&PointSphere);
				return;
			}
		}
	}

	BatchLines.DisableRenderBoundingBox();
}

void UEditor::ProcessMouseInput()
{
	const UInputManager& InputManager = UInputManager::GetInstance();
	const FVector& MousePos = InputManager.GetMousePosition();

	// KTLWeek07: 활성 뷰포트의 정보 가져오기
	auto& ViewportManager = UViewportManager::GetInstance();
	ActiveViewportIndex = ViewportManager.GetActiveIndex();
	FViewport* ActiveViewport = ViewportManager.GetViewports()[ActiveViewportIndex];
	if (!ActiveViewport) { return; }

	// PIE 뷰포트에서는 에디터 입력(오브젝트 선택, 기즈모 조작) 처리 안 함
	if (GEditor->IsPIESessionActive() && ActiveViewportIndex == ViewportManager.GetPIEActiveViewportIndex())
	{
		return;
	}

	const D3D11_VIEWPORT& ViewportInfo = ActiveViewport->GetRenderRect();

	// ViewportClient 가져오기
	FViewportClient* Client = ViewportManager.GetClients()[ActiveViewportIndex];
	if (!Client)
	{
		return;
	}

	AActor* ActorPicked = GetSelectedActor();
	if (ActorPicked)
	{
		// 피킹 전 현재 ViewportClient Transform으로 기즈모 스케일 업데이트
		Gizmo.UpdateScale(Client, ViewportInfo);
	}

	const float NdcX = ((MousePos.X - ViewportInfo.TopLeftX) / ViewportInfo.Width) * 2.0f - 1.0f;
	const float NdcY = -(((MousePos.Y - ViewportInfo.TopLeftY) / ViewportInfo.Height) * 2.0f - 1.0f);

	// ViewportClient Transform으로 World Ray 계산
	FMatrix ViewMatrix = Client->GetViewMatrix();
	FMatrix ProjectionMatrix = Client->GetProjectionMatrix(ViewportInfo.Width / ViewportInfo.Height);
	FMatrix ViewProjectionMatrixInv = (ViewMatrix * ProjectionMatrix).Inverse();

	// NDC를 World로 변환 (w=1로 동차 좌표 사용)
	FVector4 NearPoint = FVector4(NdcX, NdcY, 0.0f, 1.0f);
	FVector4 FarPoint = FVector4(NdcX, NdcY, 1.0f, 1.0f);

	FVector4 WorldNear4 = FMatrix::VectorMultiply(NearPoint, ViewProjectionMatrixInv);
	FVector4 WorldFar4 = FMatrix::VectorMultiply(FarPoint, ViewProjectionMatrixInv);

	// 원근 나누기 (perspective divide): w로 나눠서 3D 좌표로 변환
	FVector WorldNear = FVector(WorldNear4.X / WorldNear4.W, WorldNear4.Y / WorldNear4.W, WorldNear4.Z / WorldNear4.W);
	FVector WorldFar = FVector(WorldFar4.X / WorldFar4.W, WorldFar4.Y / WorldFar4.W, WorldFar4.Z / WorldFar4.W);

	FVector Direction = (WorldFar - WorldNear);
	Direction.Normalize();

	FRay WorldRay;
	const FVector ViewLocation = Client->GetViewLocation();
	WorldRay.Origin = FVector4(ViewLocation.X, ViewLocation.Y, ViewLocation.Z, 1.0f);
	WorldRay.Direction = FVector4(Direction.X, Direction.Y, Direction.Z, 0.0f);

	FVector CollisionPoint;
	float ActorDistance = -1;

	// 기즈모 World / Local 모드 전환
	if (InputManager.IsKeyDown(EKeyInput::Ctrl) && InputManager.IsKeyPressed(EKeyInput::Backtick))
	{
		Gizmo.IsWorldMode() ? Gizmo.SetLocal() : Gizmo.SetWorld();
	}
	if (InputManager.IsKeyPressed(EKeyInput::Space))
	{
		Gizmo.ChangeGizmoMode();
	}

	// W/E/R 키로 기즈모 모드 직접 전환 (우클릭 중이 아닐 때만)
	bool bIsRightMouseDown = InputManager.IsKeyDown(EKeyInput::MouseRight);
	if (!bIsRightMouseDown)
	{
		if (InputManager.IsKeyPressed(EKeyInput::W))
		{
			Gizmo.SetGizmoMode(EGizmoMode::Translate);
		}
		if (InputManager.IsKeyPressed(EKeyInput::E))
		{
			Gizmo.SetGizmoMode(EGizmoMode::Rotate);
		}
		if (InputManager.IsKeyPressed(EKeyInput::R))
		{
			Gizmo.SetGizmoMode(EGizmoMode::Scale);
		}
	}
	if (InputManager.IsKeyReleased(EKeyInput::MouseLeft))
	{
		Gizmo.EndDrag();

		// 복사 모드 종료
		if (bIsInCopyMode)
		{
			bIsInCopyMode = false;
			CopiedActor = nullptr;
			CopiedComponent = nullptr;
		}
	}

	// 스플리터 드래그 여부 체크
	const bool bSplitterDragging = ViewportManager.IsAnySplitterDragging();

	// 드래그 중: 기즈모 컨트롤 로직 실행
	if (Gizmo.IsDragging() && IsValid<USceneComponent>(Gizmo.GetSelectedComponent()))
	{
		switch (Gizmo.GetGizmoMode())
		{
		case EGizmoMode::Translate:
			{
				FVector GizmoDragLocation = GetGizmoDragLocation(Client, WorldRay);
				Gizmo.SetLocation(GizmoDragLocation);
				break;
			}
		case EGizmoMode::Rotate:
			{
				FQuaternion GizmoDragRotation = GetGizmoDragRotation(Client, WorldRay);
				Gizmo.SetComponentRotation(GizmoDragRotation);
				break;
			}
		case EGizmoMode::Scale:
			{
				FVector GizmoDragScale = GetGizmoDragScale(Client, WorldRay);
				Gizmo.SetComponentScale(GizmoDragScale);
			}
		}
	}
	// 드래그 중이 아님: 클릭/호버링 처리
	else if (!ImGui::GetIO().WantCaptureMouse && !bSplitterDragging)
	{
		// 1. 클릭 감지 (먼저!)
		bool bIsDoubleClick = InputManager.IsMouseDoubleClicked(EKeyInput::MouseLeft);
		bool bIsSingleClick = !bIsDoubleClick && InputManager.IsKeyPressed(EKeyInput::MouseLeft);

		// 2. 클릭 처리: HitProxy로 기즈모/오브젝트 구분
		if (bIsDoubleClick || bIsSingleClick)
		{
			// 뷰포트 클릭 시 LastClickedViewportIndex 업데이트 (PIE 시작 시 사용)
			ViewportManager.SetLastClickedViewportIndex(ActiveViewportIndex);

			// HitProxy를 통한 컴포넌트 피킹
			UPrimitiveComponent* PrimitiveCollided = nullptr;
			const int32 MouseX = static_cast<int32>(MousePos.X - ViewportInfo.TopLeftX);
			const int32 MouseY = static_cast<int32>(MousePos.Y - ViewportInfo.TopLeftY);

			TStatId StatId("Picking");
			FScopeCycleCounter PickCounter(StatId);
			PrimitiveCollided = ObjectPicker.PickPrimitive(Client, MouseX, MouseY);
			ActorPicked = PrimitiveCollided ? PrimitiveCollided->GetOwner() : nullptr;
			float ElapsedMs = static_cast<float>(PickCounter.Finish());
			UStatOverlay::GetInstance().RecordPickingStats(ElapsedMs);

			// HitProxy 결과로 기즈모 클릭 여부 확인
			// HitProxy가 기즈모를 반환했는지 직접 체크 (기즈모는 별도 HitProxy ID 가짐)
			// TODO: HitProxy ID로 기즈모 구분하는 방식으로 개선 필요
			// 현재는 임시로 호버링 상태 사용
			EGizmoDirection PreviousDirection = Gizmo.GetGizmoDirection();

			// 클릭 시점에 기즈모 호버링 중이었는지 체크 (이전 프레임 호버링 상태 사용)
			bool bClickedGizmo = (PreviousDirection != EGizmoDirection::None);

			// 기즈모 클릭: 드래그 시작
			if (bClickedGizmo)
			{
				// Alt + 드래그: 객체 복사 (Scale 모드에서는 비활성화)
				bool bAltPressed = InputManager.IsKeyDown(EKeyInput::Alt);
				bool bIsScaleMode = (Gizmo.GetGizmoMode() == EGizmoMode::Scale);

				if (bAltPressed && GetSelectedActor() && GetSelectedComponent() && !bIsScaleMode)
				{
					// RootComponent 선택 = Actor 선택
					bool bIsActorSelection = (GetSelectedComponent() == GetSelectedActor()->GetRootComponent());

					if (bIsActorSelection)
					{
						// Actor 복사
						AActor* NewActor = DuplicateActor(GetSelectedActor());
						if (NewActor)
						{
							SelectActor(NewActor);
							CopiedActor = NewActor;
							bIsInCopyMode = true;
						}
					}
					else
					{
						// Component 복사
						UActorComponent* NewComponent = DuplicateComponent(GetSelectedComponent(), GetSelectedActor());
						if (NewComponent)
						{
							SelectActorAndComponent(GetSelectedActor(), NewComponent);
							CopiedComponent = NewComponent;
							bIsInCopyMode = true;
						}
					}
				}

				// 기즈모 드래그 시작
				Gizmo.OnMouseDragStart(Client, CollisionPoint);
			}
			// 오브젝트 클릭: 오브젝트 선택
			else
			{
				if (ActorPicked && PrimitiveCollided)
				{
					UActorComponent* ComponentToSelect = PrimitiveCollided;

					// Visualization 컴포넌트가 피킹된 경우, 부모 컴포넌트를 선택
					if (PrimitiveCollided->IsVisualizationComponent())
					{
						if (USceneComponent* ScenePrim = Cast<USceneComponent>(PrimitiveCollided))
						{
							if (USceneComponent* Parent = ScenePrim->GetAttachParent())
							{
								ComponentToSelect = Parent;
							}
						}
					}

					if (bIsDoubleClick)
					{
						// 더블클릭: Component 피킹 모드로 진입
						SelectActorAndComponent(ActorPicked, ComponentToSelect);
						bIsActorSelected = false;
					}
					else // bIsSingleClick
					{
						AActor* CurrentSelectedActor = GetSelectedActor();

						if (!CurrentSelectedActor)
						{
							// 선택 없음 상태: Actor 선택
							SelectActor(ActorPicked);
							bIsActorSelected = true;
						}
						else if (bIsActorSelected)
						{
							// Actor 선택 상태에서 단일 클릭
							if (CurrentSelectedActor == ActorPicked)
							{
								// 같은 Actor: Actor 선택 유지
								SelectActor(ActorPicked);
								bIsActorSelected = true;
							}
							else
							{
								// 다른 Actor: 새로운 Actor 선택
								SelectActor(ActorPicked);
								bIsActorSelected = true;
							}
						}
						else
						{
							// Component 피킹 모드에서 단일 클릭
							if (CurrentSelectedActor == ActorPicked)
							{
								// 같은 Actor 내 컴포넌트: Component 전환
								SelectActorAndComponent(ActorPicked, ComponentToSelect);
								bIsActorSelected = false;
							}
							else
							{
								// 다른 Actor: Component 피킹 모드 해제 -> Actor 선택
								SelectActor(ActorPicked);
								bIsActorSelected = true;
							}
						}
					}
				}
				else
				{
					// 빈 공간 클릭: 선택 해제
					SelectActor(nullptr);
					bIsActorSelected = true;
				}
			}
		}
		// 3. 클릭이 아닌 경우: Ray 기반 호버링
		else
		{
			if (GetSelectedActor() && Gizmo.HasComponent())
			{
				// 기즈모 호버링 (Ray Based)
				ObjectPicker.PickGizmo(Client, WorldRay, Gizmo, CollisionPoint);
			}
			else
			{
				Gizmo.SetGizmoDirection(EGizmoDirection::None);
			}
		}
	}

	if (InputManager.IsKeyPressed(EKeyInput::F))
	{
		FocusOnSelectedActor();
	}
}

FVector UEditor::GetGizmoDragLocation(FViewportClient* InClient, FRay& WorldRay)
{
	const EGizmoDirection Direction = Gizmo.GetGizmoDirection();

	// Direction이 None이면 현재 위치 반환
	if (Direction == EGizmoDirection::None)
	{
		return Gizmo.GetGizmoLocation();
	}

	// 드래그를 시작한 뷰포트와 현재 뷰포트가 다르면 드래그 무시
	if (Gizmo.GetDragStartViewportClient() != InClient)
	{
		return Gizmo.GetGizmoLocation();
	}

	// 스크린 공간 축 방향 벡터와 마우스 델타 내적 사용
	const UInputManager& InputManager = UInputManager::GetInstance();
	const FVector& GlobalMousePos = InputManager.GetMousePosition();

	// 드래그 시작한 뷰포트의 정보로 로컬 좌표 계산
	auto& ViewportManager = UViewportManager::GetInstance();
	const auto& Viewports = ViewportManager.GetViewports();
	int32 DragViewportIndex = -1;
	for (int32 i = 0; i < Viewports.Num(); ++i)
	{
		if (ViewportManager.GetClients()[i] == InClient)
		{
			DragViewportIndex = i;
			break;
		}
	}

	if (DragViewportIndex == -1)
	{
		return Gizmo.GetGizmoLocation();
	}

	const D3D11_VIEWPORT& DragViewportInfo = Viewports[DragViewportIndex]->GetRenderRect();
	const FVector2 CurrentScreenPos(
		GlobalMousePos.X - DragViewportInfo.TopLeftX,
		GlobalMousePos.Y - DragViewportInfo.TopLeftY
	);

	const FVector2 PrevScreenPos = Gizmo.GetPreviousScreenPos();
	const FVector2 DragDelta = CurrentScreenPos - PrevScreenPos;

	// 스크린 공간 축 방향 벡터 (드래그 중인 ViewportClient 기준으로 재계산)
	FVector2 ScreenAxisX, ScreenAxisY, ScreenAxisZ, ScreenOrigin;
	Gizmo.CalculateScreenAxes(InClient, DragViewportInfo, ScreenAxisX, ScreenAxisY, ScreenAxisZ, ScreenOrigin);

	// 각 축별 드래그량 계산 (내적)
	float DragX = 0.0f;
	float DragY = 0.0f;
	float DragZ = 0.0f;

	// Center: 모든 축 자유 이동
	if (Direction == EGizmoDirection::Center)
	{
		DragX = FVector2::DotProduct(ScreenAxisX, DragDelta);
		DragY = FVector2::DotProduct(ScreenAxisY, DragDelta);
		DragZ = FVector2::DotProduct(ScreenAxisZ, DragDelta);
	}
	// 평면 드래그: 두 축 동시 이동
	else if (Direction == EGizmoDirection::XY_Plane)
	{
		DragX = FVector2::DotProduct(ScreenAxisX, DragDelta);
		DragY = FVector2::DotProduct(ScreenAxisY, DragDelta);
	}
	else if (Direction == EGizmoDirection::XZ_Plane)
	{
		DragX = FVector2::DotProduct(ScreenAxisX, DragDelta);
		DragZ = FVector2::DotProduct(ScreenAxisZ, DragDelta);
	}
	else if (Direction == EGizmoDirection::YZ_Plane)
	{
		DragY = FVector2::DotProduct(ScreenAxisY, DragDelta);
		DragZ = FVector2::DotProduct(ScreenAxisZ, DragDelta);
	}
	// 단일 축 드래그
	else if (Direction == EGizmoDirection::Forward)
	{
		DragX = FVector2::DotProduct(ScreenAxisX, DragDelta);
	}
	else if (Direction == EGizmoDirection::Right)
	{
		DragY = FVector2::DotProduct(ScreenAxisY, DragDelta);
	}
	else if (Direction == EGizmoDirection::Up)
	{
		DragZ = FVector2::DotProduct(ScreenAxisZ, DragDelta);
	}

	// 월드 공간 축 방향
	FVector WorldAxisX = FVector(1, 0, 0);
	FVector WorldAxisY = FVector(0, 1, 0);
	FVector WorldAxisZ = FVector(0, 0, 1);

	// Local 모드: 컴포넌트 회전 적용
	if (!Gizmo.IsWorldMode())
	{
		const FQuaternion CompRot = Gizmo.GetTargetComponent()->GetWorldRotationAsQuaternion();
		WorldAxisX = CompRot.RotateVector(WorldAxisX);
		WorldAxisY = CompRot.RotateVector(WorldAxisY);
		WorldAxisZ = CompRot.RotateVector(WorldAxisZ);
	}

	// 스크린 공간 드래그 -> 월드 공간 이동 변환
	// 언리얼 방식: 뷰포트 거리와 FOV 기반으로 DragScale 계산
	const FVector GizmoLocation = Gizmo.GetGizmoLocation();
	const FVector CameraLocation = InClient->GetViewLocation();
	const float DistanceToGizmo = (GizmoLocation - CameraLocation).Length();

	// 기즈모 드래그 스케일 계산
	// UnitsPerPixel = Distance * tan(FOV/2) / (ViewportHeight/2)
	const float ViewportHeight = static_cast<float>(DragViewportInfo.Height);
	const float FOV = InClient->GetFOV();
	const float FOVRadians = FVector::GetDegreeToRadian(FOV);
	const float UnitsPerPixel = (DistanceToGizmo * std::tanf(FOVRadians * 0.5f)) / (ViewportHeight * 0.5f);

	// 스크린 축 방향과 월드 축 방향의 일관성 보장
	// 카메라가 축의 반대편에 있을 때 방향이 뒤집히므로 보정 필요
	// 방법: 스크린 공간에서 축이 카메라 뒤쪽을 향하면 월드 축 반전
	const FVector CamForward = InClient->GetForward();

	// 각 축이 카메라 방향과 반대인지 체크 (내적이 음수면 반대)
	const float DotX = WorldAxisX.Dot(CamForward);
	const float DotY = WorldAxisY.Dot(CamForward);
	const float DotZ = WorldAxisZ.Dot(CamForward);

	// 카메라 뒤쪽을 향하는 축은 스크린 드래그 방향이 반대이므로 보정
	const float SignX = (DotX < 0.0f) ? -1.0f : 1.0f;
	const float SignY = (DotY < 0.0f) ? -1.0f : 1.0f;
	const float SignZ = (DotZ < 0.0f) ? -1.0f : 1.0f;

	// 픽셀 단위 드래그를 월드 단위로 변환
	const FVector WorldDelta =
		WorldAxisX * (DragX * UnitsPerPixel * SignX) +
		WorldAxisY * (DragY * UnitsPerPixel * SignY) +
		WorldAxisZ * (DragZ * UnitsPerPixel * SignZ);

	// 이전 스크린 좌표 저장
	Gizmo.SetPreviousScreenPos(CurrentScreenPos);

	return Gizmo.GetGizmoLocation() + WorldDelta;
}

FQuaternion UEditor::GetGizmoDragRotation(FViewportClient* InClient, FRay& WorldRay)
{
	// 드래그를 시작한 뷰포트와 현재 뷰포트가 다르면 드래그 무시
	if (Gizmo.GetDragStartViewportClient() != InClient)
	{
		return Gizmo.GetComponentRotation();
	}

	const FVector GizmoLocation = Gizmo.GetGizmoLocation();
	const FVector LocalGizmoAxis = Gizmo.GetGizmoAxis();
	const FQuaternion StartRotQuat = Gizmo.GetDragStartActorRotationQuat();

	// 월드 공간 회전축
	FVector WorldRotationAxis = LocalGizmoAxis;
	if (!Gizmo.IsWorldMode())
	{
		WorldRotationAxis = StartRotQuat.RotateVector(LocalGizmoAxis);
	}

	// 스크린 공간 회전 계산
	// 드래그 시작한 뷰포트의 정보 가져오기
	const UInputManager& InputManager = UInputManager::GetInstance();
	const FVector& GlobalMousePos = InputManager.GetMousePosition();

	auto& ViewportManager = UViewportManager::GetInstance();
	const auto& Viewports = ViewportManager.GetViewports();
	int32 DragViewportIndex = -1;
	for (int32 i = 0; i < Viewports.Num(); ++i)
	{
		if (ViewportManager.GetClients()[i] == InClient)
		{
			DragViewportIndex = i;
			break;
		}
	}

	if (DragViewportIndex == -1)
	{
		return Gizmo.GetComponentRotation();
	}

	const D3D11_VIEWPORT& DragViewportInfo = Viewports[DragViewportIndex]->GetRenderRect();
	const float ViewportWidth = static_cast<float>(DragViewportInfo.Width);
	const float ViewportHeight = static_cast<float>(DragViewportInfo.Height);

	// ViewportClient로부터 View/Projection 행렬 가져오기
	const float AspectRatio = ViewportWidth / ViewportHeight;
	const FMatrix ViewMatrix = InClient->GetViewMatrix();
	const FMatrix ProjectionMatrix = InClient->GetProjectionMatrix(AspectRatio);
	const FMatrix ViewProj = ViewMatrix * ProjectionMatrix;
	FVector4 GizmoScreenPos4 = FVector4(GizmoLocation, 1.0f) * ViewProj;

	if (GizmoScreenPos4.W > 0.0f)
	{
		// NDC로 변환
		GizmoScreenPos4 *= (1.0f / GizmoScreenPos4.W);

		// NDC → 뷰포트 로컬 좌표
		const FVector2 GizmoScreenPos(
			(GizmoScreenPos4.X * 0.5f + 0.5f) * ViewportWidth,
			((-GizmoScreenPos4.Y) * 0.5f + 0.5f) * ViewportHeight
		);

		// 현재 마우스 스크린 좌표 (뷰포트 로컬)
		const FVector2 CurrentScreenPos(
			GlobalMousePos.X - DragViewportInfo.TopLeftX,
			GlobalMousePos.Y - DragViewportInfo.TopLeftY
		);

		// UE5 표준: 드래그 시작 지점에서 기즈모로의 방향 (Origin = DragStartPos in UE)
		const FVector2 DragStartScreenPos = Gizmo.GetDragStartScreenPos();
		const FVector2 DirectionToMousePos = (DragStartScreenPos - GizmoScreenPos).GetSafeNormal();

		// Tangent 방향: DirectionToMousePos에 수직 (시계방향 회전)
		FVector2 TangentDir = FVector2(-DirectionToMousePos.Y, DirectionToMousePos.X);

		// 스크린 공간 드래그 벡터
		const FVector2 PrevScreenPos = Gizmo.GetPreviousScreenPos();
		const FVector2 DragDelta = CurrentScreenPos - PrevScreenPos;
		const FVector2 DragDir = FVector2(DragDelta.X, -DragDelta.Y); // Y축 반전

		// 마우스가 실제로 움직였는지 체크
		const float DragDistSq = DragDir.LengthSquared();
		constexpr float MinDragDistSq = 0.1f * 0.1f;

		if (DragDistSq > MinDragDistSq)
		{
			// UE5 표준: Dot Product로 회전 각도 계산
			float PixelDelta = TangentDir.Dot(DragDir);

			// 픽셀을 각도(라디안)로 변환 (언리얼에선 1픽셀 = 1도 사용, 어차피 감도 조절용 매직 넘버라 그대로 차용)
			constexpr float PixelsToDegrees = 1.0f;
			float DeltaAngleDegrees = PixelDelta * PixelsToDegrees;
			float DeltaAngle = FVector::GetDegreeToRadian(DeltaAngleDegrees);

			// 카메라 시점 방향에 따른 회전 방향 보정
			// 카메라가 회전축의 반대편에 있으면 부호 반전
			const FVector CameraLocation = InClient->GetViewLocation();
			const FVector CamToGizmo = (GizmoLocation - CameraLocation).GetSafeNormal();
			const float AxisDotCam = WorldRotationAxis.Dot(CamToGizmo);
			if (AxisDotCam < 0.0f)
			{
				DeltaAngle = -DeltaAngle;
			}

			// 누적 각도 업데이트
			float NewAngle = Gizmo.GetCurrentRotationAngle() + DeltaAngle;

			// 360deg Clamp
			constexpr float TwoPi = 2.0f * PI;
			if (NewAngle > TwoPi)
			{
				NewAngle = fmodf(NewAngle, TwoPi);
			}
			else if (NewAngle < -TwoPi)
			{
				NewAngle = fmodf(NewAngle, -TwoPi);
			}

			Gizmo.SetCurrentRotationAngle(NewAngle);
		}

		// 현재 스크린 좌표 저장
		Gizmo.SetPreviousScreenPos(CurrentScreenPos);

		// 최종 회전 Quaternion 계산 (스냅 적용)
		float FinalAngle = Gizmo.GetCurrentRotationAngle();
		if (UViewportManager::GetInstance().IsRotationSnapEnabled())
		{
			const float SnapAngleDegrees = UViewportManager::GetInstance().GetRotationSnapAngle();
			const float SnapAngleRadians = FVector::GetDegreeToRadian(SnapAngleDegrees);
			FinalAngle = std::round(Gizmo.GetCurrentRotationAngle() / SnapAngleRadians) * SnapAngleRadians;
		}

		// 기즈모 드래그 방향과 회전 방향을 일치시키기 위해 부호 반전
		const FQuaternion DeltaRotQuat = FQuaternion::FromAxisAngle(LocalGizmoAxis, FinalAngle);
		if (Gizmo.IsWorldMode())
		{
			return DeltaRotQuat * StartRotQuat;
		}
		else
		{
			return StartRotQuat * DeltaRotQuat;
		}
	}

	return Gizmo.GetComponentRotation();
}

FVector UEditor::GetGizmoDragScale(FViewportClient* InClient, FRay& WorldRay)
{
	const EGizmoDirection Direction = Gizmo.GetGizmoDirection();
	if (Direction == EGizmoDirection::None)
	{
		return Gizmo.GetComponentScale();
	}

	// 드래그를 시작한 뷰포트와 현재 뷰포트가 다르면 드래그 무시
	if (Gizmo.GetDragStartViewportClient() != InClient)
	{
		return Gizmo.GetComponentScale();
	}

	// 스크린 공간 드래그로 스케일 계산
	const UInputManager& InputManager = UInputManager::GetInstance();
	const FVector& GlobalMousePos = InputManager.GetMousePosition();

	// 드래그 시작한 뷰포트의 정보로 로컬 좌표 계산
	auto& ViewportManager = UViewportManager::GetInstance();
	const auto& Viewports = ViewportManager.GetViewports();
	int32 DragViewportIndex = -1;
	for (int32 i = 0; i < Viewports.Num(); ++i)
	{
		if (ViewportManager.GetClients()[i] == InClient)
		{
			DragViewportIndex = i;
			break;
		}
	}

	if (DragViewportIndex == -1)
	{
		return Gizmo.GetComponentScale();
	}

	const D3D11_VIEWPORT& DragViewportInfo = Viewports[DragViewportIndex]->GetRenderRect();
	const FVector2 CurrentScreenPos(
		GlobalMousePos.X - DragViewportInfo.TopLeftX,
		GlobalMousePos.Y - DragViewportInfo.TopLeftY
	);

	// Scale은 드래그 시작 위치부터의 누적 델타 사용 (Translate와 다름)
	const FVector2 DragStartScreenPos = Gizmo.GetDragStartScreenPos();
	const FVector2 DragDelta = CurrentScreenPos - DragStartScreenPos;

	// 스크린 공간 축 방향 벡터 (드래그 중인 ViewportClient 기준으로 재계산)
	FVector2 ScreenAxisX, ScreenAxisY, ScreenAxisZ, ScreenOrigin;
	Gizmo.CalculateScreenAxes(InClient, DragViewportInfo, ScreenAxisX, ScreenAxisY, ScreenAxisZ, ScreenOrigin);

	// 각 축별 드래그량 계산 (내적)
	float DragX = 0.0f;
	float DragY = 0.0f;
	float DragZ = 0.0f;

	// Center: 균일 스케일 (모든 축 동일)
	if (Direction == EGizmoDirection::Center)
	{
		// 세 축 중 가장 큰 드래그값 사용
		const float DX = FVector2::DotProduct(ScreenAxisX, DragDelta);
		const float DY = FVector2::DotProduct(ScreenAxisY, DragDelta);
		const float DZ = FVector2::DotProduct(ScreenAxisZ, DragDelta);
		const float MaxDrag = max(max(abs(DX), abs(DY)), abs(DZ));
		const float Sign = (DX + DY + DZ) >= 0.0f ? 1.0f : -1.0f;

		DragX = DragY = DragZ = MaxDrag * Sign;
	}
	// 평면 스케일: 두 축 동일 비율로 스케일 (동배율)
	else if (Direction == EGizmoDirection::XY_Plane)
	{
		const float DX = FVector2::DotProduct(ScreenAxisX, DragDelta);
		const float DY = FVector2::DotProduct(ScreenAxisY, DragDelta);
		const float MaxDrag = max(abs(DX), abs(DY));
		const float Sign = (DX + DY) >= 0.0f ? 1.0f : -1.0f;
		DragX = DragY = MaxDrag * Sign;
	}
	else if (Direction == EGizmoDirection::XZ_Plane)
	{
		const float DX = FVector2::DotProduct(ScreenAxisX, DragDelta);
		const float DZ = FVector2::DotProduct(ScreenAxisZ, DragDelta);
		const float MaxDrag = max(abs(DX), abs(DZ));
		const float Sign = (DX + DZ) >= 0.0f ? 1.0f : -1.0f;
		DragX = DragZ = MaxDrag * Sign;
	}
	else if (Direction == EGizmoDirection::YZ_Plane)
	{
		const float DY = FVector2::DotProduct(ScreenAxisY, DragDelta);
		const float DZ = FVector2::DotProduct(ScreenAxisZ, DragDelta);
		const float MaxDrag = max(abs(DY), abs(DZ));
		const float Sign = (DY + DZ) >= 0.0f ? 1.0f : -1.0f;
		DragY = DragZ = MaxDrag * Sign;
	}
	// 단일 축 스케일
	else if (Direction == EGizmoDirection::Forward)
	{
		DragX = FVector2::DotProduct(ScreenAxisX, DragDelta);
	}
	else if (Direction == EGizmoDirection::Right)
	{
		DragY = FVector2::DotProduct(ScreenAxisY, DragDelta);
	}
	else if (Direction == EGizmoDirection::Up)
	{
		DragZ = FVector2::DotProduct(ScreenAxisZ, DragDelta);
	}

	// 픽셀 단위 드래그를 스케일 변화로 변환
	// 거리에 비례하는 스케일 민감도
	const FVector GizmoLocation = Gizmo.GetGizmoLocation();
	const FVector CameraLocation = InClient->GetViewLocation();
	const float DistanceToGizmo = (GizmoLocation - CameraLocation).Length();

	// 스케일 민감도: 100 픽셀 = 1x 스케일 변화 (거리 기반 조정)
	constexpr float BaseScaleSensitivity = 0.01f;
	const float ScaleSensitivity = BaseScaleSensitivity * (DistanceToGizmo / 100.0f);

	// 스크린 축 방향과 월드 축 방향의 일관성 보장
	const FVector CamForward = InClient->GetForward();

	// 월드 공간 축 방향 (Scale 모드는 항상 컴포넌트 회전 적용)
	FVector WorldAxisX = FVector(1, 0, 0);
	FVector WorldAxisY = FVector(0, 1, 0);
	FVector WorldAxisZ = FVector(0, 0, 1);

	// Scale 모드는 World/Local 관계없이 항상 컴포넌트 로컬 축 사용
	const FQuaternion CompRot = Gizmo.GetTargetComponent()->GetWorldRotationAsQuaternion();
	WorldAxisX = CompRot.RotateVector(WorldAxisX);
	WorldAxisY = CompRot.RotateVector(WorldAxisY);
	WorldAxisZ = CompRot.RotateVector(WorldAxisZ);

	// 각 축이 카메라 방향과 반대인지 체크 (내적이 음수면 반대)
	const float DotX = WorldAxisX.Dot(CamForward);
	const float DotY = WorldAxisY.Dot(CamForward);
	const float DotZ = WorldAxisZ.Dot(CamForward);

	// 카메라 뒤쪽을 향하는 축은 스크린 드래그 방향이 반대이므로 보정
	const float SignX = (DotX < 0.0f) ? -1.0f : 1.0f;
	const float SignY = (DotY < 0.0f) ? -1.0f : 1.0f;
	const float SignZ = (DotZ < 0.0f) ? -1.0f : 1.0f;

	// 드래그 픽셀을 스케일 변화율로 변환 (방향 보정 적용)
	const float ScaleDeltaX = DragX * ScaleSensitivity * SignX;
	const float ScaleDeltaY = DragY * ScaleSensitivity * SignY;
	const float ScaleDeltaZ = DragZ * ScaleSensitivity * SignZ;

	// 시작 스케일에서 변화량 적용
	const FVector DragStartScale = Gizmo.GetDragStartActorScale();
	FVector NewScale;
	NewScale.X = DragStartScale.X + ScaleDeltaX * DragStartScale.X;
	NewScale.Y = DragStartScale.Y + ScaleDeltaY * DragStartScale.Y;
	NewScale.Z = DragStartScale.Z + ScaleDeltaZ * DragStartScale.Z;

	return NewScale;
}

void UEditor::SelectActor(AActor* InActor)
{
	if (InActor == SelectedActor.Get()) return;

	// 이전 선택 해제 (모든 컴포넌트)
	if (SelectedActor)
	{
		for (UActorComponent* Component : SelectedActor->GetOwnedComponents())
		{
			if (Component)
			{
				Component->OnDeselected();
			}
		}
	}

	SelectedActor = InActor;

	// Gizmo 상태 리셋 (드래그/호버 상태 초기화)
	Gizmo.EndDrag();

	if (SelectedActor)
	{
		// Actor 선택 시 모든 컴포넌트 하이라이팅
		for (UActorComponent* Component : SelectedActor->GetOwnedComponents())
		{
			if (Component)
			{
				Component->OnSelected();
			}
		}

		// Gizmo는 RootComponent에 부착
		SelectedComponent = InActor->GetRootComponent();
		Gizmo.SetSelectedComponent(Cast<USceneComponent>(GetSelectedComponent()));
		UUIManager::GetInstance().OnSelectedComponentChanged(GetSelectedComponent());
	}
	else
	{
		SelectedComponent = nullptr;
		Gizmo.SetSelectedComponent(nullptr);
		UUIManager::GetInstance().OnSelectedComponentChanged(nullptr);
	}
}

void UEditor::SelectActorAndComponent(AActor* InActor, UActorComponent* InComponent)
{
	// 이전 Actor의 모든 컴포넌트 선택 해제
	if (SelectedActor && SelectedActor != InActor)
	{
		for (UActorComponent* Component : SelectedActor->GetOwnedComponents())
		{
			if (Component)
			{
				Component->OnDeselected();
			}
		}
	}
	else if (SelectedActor == InActor)
	{
		// 같은 Actor 내에서 컴포넌트만 변경: 기존 모든 컴포넌트 해제
		for (UActorComponent* Component : SelectedActor->GetOwnedComponents())
		{
			if (Component)
			{
				Component->OnDeselected();
			}
		}
	}

	SelectedActor = InActor;

	// Gizmo 상태 리셋 (드래그/호버 상태 초기화)
	Gizmo.EndDrag();

	SelectComponent(InComponent);
}

void UEditor::SelectComponent(UActorComponent* InComponent)
{
	if (InComponent == SelectedComponent.Get()) return;

	// Component 선택 시 단일 컴포넌트만 하이라이팅
	if (SelectedComponent.IsValid())
	{
		SelectedComponent->OnDeselected();
	}

	// Gizmo 상태 리셋 (드래그/호버 상태 초기화)
	Gizmo.EndDrag();

	SelectedComponent = InComponent;
	if (SelectedComponent.IsValid())
	{
		SelectedComponent->OnSelected();
		Gizmo.SetSelectedComponent(Cast<USceneComponent>(GetSelectedComponent()));
	}
	else
	{
		Gizmo.SetSelectedComponent(nullptr);
	}
	UUIManager::GetInstance().OnSelectedComponentChanged(GetSelectedComponent());
}

bool UEditor::GetComponentFocusTarget(UActorComponent* Component, FVector& OutCenter, float& OutRadius)
{
	if (!Component)
	{
		UE_LOG_WARNING("Editor: GetComponentFocusTarget: Component is null");
		return false;
	}

	USceneComponent* SceneComp = Cast<USceneComponent>(Component);
	if (!SceneComp)
	{
		UE_LOG_WARNING("Editor: GetComponentFocusTarget: Not a SceneComponent");
		return false;
	}

	// LightComponent: 10x10x10 박스 가정한 AABB
	if (ULightComponent* LightComp = Cast<ULightComponent>(SceneComp))
	{
		OutCenter = LightComp->GetWorldLocation();
		// 10x10x10 박스의 반경 = sqrt(10^2 + 10^2 + 10^2) / 2 = 8.66
		OutRadius = 8.66f;
		return true;
	}

	// PrimitiveComponent: AABB 기반 계산
	if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(SceneComp))
	{
		FVector Min, Max;
		PrimComp->GetWorldAABB(Min, Max);
		OutCenter = (Min + Max) * 0.5f;

		FVector Size = Max - Min;
		OutRadius = Size.Length() * 0.5f;

		// 최소 반경 보장 (너무 작은 오브젝트 대응)
		OutRadius = max(OutRadius, 10.0f);
	}
	// SceneComponent: WorldLocation 기반
	else
	{
		OutCenter = SceneComp->GetWorldLocation();
		OutRadius = 30.0f;
	}

	return true;
}

bool UEditor::GetActorFocusTarget(AActor* Actor, FVector& OutCenter, float& OutRadius)
{
	if (!Actor)
	{
		UE_LOG_WARNING("Editor: GetActorFocusTarget: Actor is null");
		return false;
	}

	// Actor의 모든 Primitive Component를 수집하여 전체 AABB 계산
	TArray<UPrimitiveComponent*> PrimitiveComponents;

	for (UActorComponent* Comp : Actor->GetOwnedComponents())
	{
		if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Comp))
		{
			// Visualization Component는 제외 (실제 렌더링되는 메쉬만 포함)
			if (PrimComp->IsVisualizationComponent())
			{
				continue;
			}
			PrimitiveComponents.Add(PrimComp);
		}
	}

	// Primitive가 없으면 RootComponent 위치 사용 (Light, ScriptComponent 등)
	if (PrimitiveComponents.IsEmpty())
	{
		if (USceneComponent* RootComp = Actor->GetRootComponent())
		{
			OutCenter = RootComp->GetWorldLocation();
			// 10x10x10 박스 가정한 AABB 반경
			OutRadius = 8.66f; // sqrt(10^2 + 10^2 + 10^2) / 2
			return true;
		}

		UE_LOG_WARNING("Editor: GetActorFocusTarget: No valid component found");
		return false;
	}

	// 모든 컴포넌트의 AABB를 합쳐서 전체 바운딩 계산
	FVector GlobalMin(FLT_MAX, FLT_MAX, FLT_MAX);
	FVector GlobalMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (UPrimitiveComponent* Prim : PrimitiveComponents)
	{
		FVector Min, Max;
		Prim->GetWorldAABB(Min, Max);

		GlobalMin.X = min(GlobalMin.X, Min.X);
		GlobalMin.Y = min(GlobalMin.Y, Min.Y);
		GlobalMin.Z = min(GlobalMin.Z, Min.Z);

		GlobalMax.X = max(GlobalMax.X, Max.X);
		GlobalMax.Y = max(GlobalMax.Y, Max.Y);
		GlobalMax.Z = max(GlobalMax.Z, Max.Z);
	}

	OutCenter = (GlobalMin + GlobalMax) * 0.5f;

	FVector Size = GlobalMax - GlobalMin;
	OutRadius = Size.Length() * 0.5f;

	UE_LOG("Editor: GetActorFocusTarget: Mesh Actor Center=(%.1f,%.1f,%.1f) Radius=%.1f",
	       OutCenter.X, OutCenter.Y, OutCenter.Z, OutRadius);
	return true;
}

void UEditor::FocusOnSelectedActor()
{
	auto& ViewportManager = UViewportManager::GetInstance();
	auto& Viewports = ViewportManager.GetViewports();
	auto& Clients = ViewportManager.GetClients();

	if (Viewports.IsEmpty() || Clients.IsEmpty())
	{
		return;
	}

	const int32 ViewportCount = Viewports.Num();

	// 마지막 클릭한 뷰포트 가져오기
	const int32 LastClickedIdx = ViewportManager.GetLastClickedViewportIndex();
	if (LastClickedIdx < 0 || LastClickedIdx >= ViewportCount || !Clients[LastClickedIdx])
	{
		return;
	}

	FViewportClient* LastClickedClient = Clients[LastClickedIdx];
	const bool bIsOrtho = LastClickedClient->IsOrtho();

	FVector Center;
	float BoundingRadius;
	bool bSuccess = false;

	// Component 선택 모드: 선택된 Component에 포커싱
	if (!bIsActorSelected && SelectedComponent)
	{
		if (bIsOrtho)
		{
			// Ortho 뷰: 로컬 원점(0,0,0) 사용
			if (USceneComponent* SceneComp = Cast<USceneComponent>(GetSelectedComponent()))
			{
				Center = SceneComp->GetWorldLocation();
				BoundingRadius = 50.0f;
				bSuccess = true;
			}
		}
		else
		{
			// Perspective 뷰: AABB 중심 사용
			bSuccess = GetComponentFocusTarget(GetSelectedComponent(), Center, BoundingRadius);
		}
	}

	// Actor 선택 모드: Actor 전체에 포커싱
	else if (SelectedActor)
	{
		if (bIsOrtho)
		{
			// Ortho 뷰: RootComponent의 월드 위치 사용
			if (USceneComponent* RootComp = SelectedActor->GetRootComponent())
			{
				Center = RootComp->GetWorldLocation();
				BoundingRadius = 100.0f;
				bSuccess = true;
			}
		}
		else
		{
			// Perspective 뷰: AABB 중심 사용
			bSuccess = GetActorFocusTarget(SelectedActor.Get(), Center, BoundingRadius);
		}
	}

	if (!bSuccess)
	{
		return;
	}

	CameraStartLocation.SetNum(ViewportCount);
	CameraStartRotation.SetNum(ViewportCount);
	CameraTargetLocation.SetNum(ViewportCount);
	CameraTargetRotation.SetNum(ViewportCount);
	OrthoZoomStart.SetNum(ViewportCount);
	OrthoZoomTarget.SetNum(ViewportCount);

	// 애니메이션 타입 설정 (Ortho 뷰면 Top, Perspective 뷰면 Perspective)
	AnimatingViewType = bIsOrtho ? EViewType::OrthoTop : EViewType::Perspective;

	for (int32 i = 0; i < ViewportCount; ++i)
	{
		FViewportClient* Client = Clients[i];
		if (!Client) continue;

		// 모든 뷰포트의 현재 상태 저장 (애니메이션 필터링은 나중에)
		CameraStartLocation[i] = Client->GetViewLocation();
		CameraStartRotation[i] = Client->GetViewRotation();

		// 오쏘 뷰포트면 현재 줌 값도 저장
		const bool bIsClientOrtho = Client->IsOrtho();
		if (bIsClientOrtho)
		{
			OrthoZoomStart[i] = Client->GetOrthoZoom();
		}

		// 마지막 클릭한 뷰포트와 동일한 타입만 목표 위치 계산
		if (bIsClientOrtho != bIsOrtho)
		{
			// 타입이 다르면 현재 위치를 목표로 (애니메이션 안 함)
			CameraTargetLocation[i] = CameraStartLocation[i];
			CameraTargetRotation[i] = CameraStartRotation[i];
			OrthoZoomTarget[i] = OrthoZoomStart[i];
			continue;
		}

		if (!bIsClientOrtho) // Perspective
		{
			// ViewRotation으로부터 Forward 벡터 계산
			FVector Radians = FVector::GetDegreeToRadian(Client->GetViewRotation());
			FMatrix RotationMatrix = FMatrix::CreateFromYawPitchRoll(Radians.Y, Radians.X, Radians.Z);
			FVector Forward = FMatrix::VectorMultiply(FVector::ForwardVector(), RotationMatrix);
			Forward.Normalize();

			const float FovY = Client->GetFOV();
			const float HalfFovRadian = FVector::GetDegreeToRadian(FovY * 0.5f);

			// BoundingRadius 기준으로 거리 계산
			float Distance = BoundingRadius / sinf(HalfFovRadian);

			// EditorIcon이나 Billboard는 작은 스프라이트이므로 더 가까이
			if (UEditorIconComponent* IconComp = Cast<UEditorIconComponent>(GetSelectedComponent()))
			{
				Distance = min(Distance, 200.0f);
			}
			else if (UBillBoardComponent* BillboardComp = Cast<UBillBoardComponent>(GetSelectedComponent()))
			{
				Distance = min(Distance, 200.0f);
			}

			CameraTargetLocation[i] = Center - Forward * Distance;

			// 목표 회전은 현재 회전 유지 (카메라 각도가 바뀌지 않음)
			CameraTargetRotation[i] = Client->GetViewRotation();

			// Perspective는 줌 애니메이션 없음
			OrthoZoomTarget[i] = OrthoZoomStart[i];
		}
		else // Orthographic
		{
			// ViewRotation으로부터 Forward 벡터 계산
			FVector Radians = FVector::GetDegreeToRadian(Client->GetViewRotation());
			FMatrix RotationMatrix = FMatrix::CreateFromYawPitchRoll(Radians.Y, Radians.X, Radians.Z);
			FVector Forward = FMatrix::VectorMultiply(FVector::ForwardVector(), RotationMatrix);
			Forward.Normalize();

			// 현재 카메라에서 물체 중심까지의 Forward 방향 투영 거리 계산
			const FVector ToCenterVec = Center - CameraStartLocation[i];
			const float DistanceToCenter = ToCenterVec.Dot(Forward);

			// 물체 중심을 바라보도록 카메라 위치 조정 (Forward 방향 유지)
			CameraTargetLocation[i] = Center - Forward * abs(DistanceToCenter);

			// 회전은 현재 유지
			CameraTargetRotation[i] = CameraStartRotation[i];

			// 오쏘 줌 목표값 설정 (애니메이션으로 전환)
			OrthoZoomTarget[i] = 500.0f;
		}
	}

	// 애니메이션 시작
	bIsCameraAnimating = true;
	CameraAnimationTime = 0.0f;
}

void UEditor::UpdateCameraAnimation()
{
	if (!bIsCameraAnimating)
	{
		return;
	}

	auto& ViewportManager = UViewportManager::GetInstance();
	auto& Clients = ViewportManager.GetClients();
	const UInputManager& Input = UInputManager::GetInstance();

	if (Clients.IsEmpty())
	{
		bIsCameraAnimating = false;
		return;
	}

	// 우클릭 드래그 시작 시 애니메이션 중단
	if (Input.IsKeyPressed(EKeyInput::MouseRight))
	{
		bIsCameraAnimating = false;
		return;
	}

	CameraAnimationTime += DT;
	float Progress = CameraAnimationTime / CAMERA_ANIMATION_DURATION;

	bool bAnimationCompleted = false;
	if (Progress >= 1.0f)
	{
		Progress = 1.0f;
		bIsCameraAnimating = false;
		bAnimationCompleted = true;
	}

	float SmoothProgress;
	if (Progress < 0.5f)
	{
		SmoothProgress = 8.0f * Progress * Progress * Progress * Progress;
	}
	else
	{
		float ProgressFromEnd = Progress - 1.0f;
		SmoothProgress = 1.0f - 8.0f * ProgressFromEnd * ProgressFromEnd * ProgressFromEnd * ProgressFromEnd;
	}

	const size_t AnimationVectorSize = CameraStartLocation.Num();
	const bool bAnimatingOrtho = (AnimatingViewType != EViewType::Perspective);

	for (int Index = 0; Index < Clients.Num() && Index < AnimationVectorSize; ++Index)
	{
		FViewportClient* Client = Clients[Index];
		if (!Client) continue;

		// 애니메이션 시작 시 결정된 타입과 일치하는 경우만 처리
		const bool bIsClientOrtho = Client->IsOrtho();
		if (bIsClientOrtho != bAnimatingOrtho)
		{
			continue;
		}

		FVector CurrentLocation = CameraStartLocation[Index] + (CameraTargetLocation[Index] - CameraStartLocation[
			Index]) * SmoothProgress;
		Client->SetViewLocation(CurrentLocation);

		// Perspective 뷰만 회전 애니메이션 적용
		// Orthographic 뷰는 회전이 고정되어야 함
		if (!bIsClientOrtho) // Perspective
		{
			FVector CurrentRotation = CameraStartRotation[Index] + (CameraTargetRotation[Index] - CameraStartRotation[
				Index]) * SmoothProgress;
			Client->SetViewRotation(CurrentRotation);
		}
		// Orthographic 뷰는 줌 애니메이션 적용
		else
		{
			float CurrentZoom = Lerp<float>(OrthoZoomStart[Index], OrthoZoomTarget[Index], SmoothProgress);
			Client->SetOrthoZoom(CurrentZoom);
		}
	}

	// 애니메이션 완료 시 오쏘 뷰의 InitialOffsets, SharedOrthoZoom 및 공유 센터 업데이트
	if (bAnimationCompleted && bAnimatingOrtho)
	{
		// SharedOrthoZoom 업데이트 (목표 줌 값으로 설정)
		ViewportManager.SetSharedOrthoZoom(500.0f);

		// 먼저 애니메이션 타겟 위치 기반으로 새로운 공유 센터 계산
		// 첫 번째 오쏘 뷰를 기준으로 공유 센터 설정
		FVector NewSharedCenter = FVector::ZeroVector();
		bool bCenterCalculated = false;

		for (int Index = 0; Index < Clients.Num(); ++Index)
		{
			FViewportClient* Client = Clients[Index];
			if (!Client || !Client->IsOrtho()) continue;

			// ViewType에 따른 InitialOffsets 인덱스 결정
			int32 OrthoIdx = -1;
			switch (Client->GetViewType())
			{
			case EViewType::OrthoTop: OrthoIdx = 0;
				break;
			case EViewType::OrthoBottom: OrthoIdx = 1;
				break;
			case EViewType::OrthoLeft: OrthoIdx = 2;
				break;
			case EViewType::OrthoRight: OrthoIdx = 3;
				break;
			case EViewType::OrthoFront: OrthoIdx = 4;
				break;
			case EViewType::OrthoBack: OrthoIdx = 5;
				break;
			}

			if (OrthoIdx >= 0 && OrthoIdx < ViewportManager.GetInitialOffsets().Num())
			{
				const FVector& OldOffset = ViewportManager.GetInitialOffsets()[OrthoIdx];
				const FVector NewLocation = Client->GetViewLocation();

				// 첫 번째 오쏘 뷰 기준으로 새로운 공유 센터 계산
				if (!bCenterCalculated)
				{
					NewSharedCenter = NewLocation - OldOffset;
					bCenterCalculated = true;
				}

				// 새로운 오프셋 = 새 위치 - 새 공유 센터
				FVector NewOffset = NewLocation - NewSharedCenter;
				ViewportManager.UpdateInitialOffset(OrthoIdx, NewOffset);
			}
		}

		// 공유 센터 업데이트
		if (bCenterCalculated)
		{
			ViewportManager.SetOrthoGraphicCameraPoint(NewSharedCenter);
		}
	}
}

/**
 * @brief Component 복사 (같은 Actor 내에 추가)
 * @note 단일 Component만 복사하며, child component는 복사하지 않음
 * @param InSourceComponent 복사할 원본 Component
 * @param InParentActor 복사된 Component가 추가될 Actor
 * @return 복사된 Component (실패 시 nullptr)
 */
UActorComponent* UEditor::DuplicateComponent(UActorComponent* InSourceComponent, AActor* InParentActor)
{
	if (!InSourceComponent || !InParentActor)
	{
		return nullptr;
	}

	// Component 복사
	UActorComponent* NewComponent = Cast<UActorComponent>(InSourceComponent->Duplicate());
	if (!NewComponent)
	{
		return nullptr;
	}

	// Owner 설정 및 Parent Actor에 등록
	NewComponent->SetOwner(InParentActor);
	InParentActor->GetOwnedComponents().Add(NewComponent);

	// DuplicateSubObjects 호출하여 서브 객체 복사
	InSourceComponent->CallDuplicateSubObjects(NewComponent);

	// SceneComponent인 경우 계층 구조 설정
	USceneComponent* NewSceneComponent = Cast<USceneComponent>(NewComponent);
	if (NewSceneComponent)
	{
		USceneComponent* SourceSceneComponent = Cast<USceneComponent>(InSourceComponent);
		if (SourceSceneComponent && SourceSceneComponent->GetAttachParent())
		{
			// 원본 Component의 부모에 새 Component도 부착
			NewSceneComponent->AttachToComponent(SourceSceneComponent->GetAttachParent());
		}
		else
		{
			// 부모가 없으면 Root Component에 부착
			if (InParentActor->GetRootComponent())
			{
				NewSceneComponent->AttachToComponent(InParentActor->GetRootComponent());
			}
		}
	}

	// Component 등록
	InParentActor->RegisterComponent(NewComponent);

	return NewComponent;
}

/**
 * @brief Actor 전체 복사
 * @param InSourceActor 복사할 원본 Actor
 * @return 복사된 Actor (실패 시 nullptr)
 */
AActor* UEditor::DuplicateActor(AActor* InSourceActor)
{
	if (!InSourceActor)
	{
		return nullptr;
	}

	// EditorWorld 가져오기
	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!EditorWorld)
	{
		return nullptr;
	}

	// Level 가져오기
	ULevel* CurrentLevel = EditorWorld->GetLevel();
	if (!CurrentLevel)
	{
		return nullptr;
	}

	// Actor 전체 복사 (EditorOnly Component 포함)
	AActor* NewActor = Cast<AActor>(InSourceActor->DuplicateForEditor());
	if (!NewActor)
	{
		return nullptr;
	}

	// Outer 설정 (Level이 Actor의 Outer, Actor가 Component들의 Outer)
	NewActor->SetOuter(CurrentLevel);
	for (UActorComponent* Component : NewActor->GetOwnedComponents())
	{
		if (Component)
		{
			Component->SetOuter(NewActor);
		}
	}

	// Level에 Actor 추가
	CurrentLevel->AddActorToLevel(NewActor);
	CurrentLevel->AddLevelComponent(NewActor);

	// BeginPlay 호출 (에디터에서 생성된 Actor는 즉시 활성화)
	NewActor->BeginPlay();

	// LightComponent & DecalComponent Visualization Icon 생성
	// 순회 중 OwnedComponents 수정 방지를 위해 먼저 수집 후 생성
	TArray<ULightComponent*> LightComponents;
	TArray<UDecalComponent*> DecalComponents;
	for (UActorComponent* Component : NewActor->GetOwnedComponents())
	{
		if (ULightComponent* LightComp = Cast<ULightComponent>(Component))
		{
			LightComponents.Add(LightComp);
		}
		if (UDecalComponent* DecalComp = Cast<UDecalComponent>(Component))
		{
			DecalComponents.Add(DecalComp);
		}
	}

	for (ULightComponent* LightComp : LightComponents)
	{
		LightComp->EnsureVisualizationIcon();
	}

	for (UDecalComponent* DecalComp : DecalComponents)
	{
		DecalComp->EnsureVisualizationIcon();
	}

	return NewActor;
}

/**
 * @brief Pilot Mode 진입 함수
 */
void UEditor::TogglePilotMode()
{
	// 진입 조건 검사
	if (!SelectedActor.IsValid())
	{
		UE_LOG_WARNING("Pilot Mode: No actor selected");
		return;
	}

	if (!bIsActorSelected)
	{
		UE_LOG_WARNING("Pilot Mode: Component selection mode. Switch to Actor selection first");
		return;
	}

	// 마지막으로 클릭한 뷰포트의 카메라 사용
	UViewportManager& ViewportManager = UViewportManager::GetInstance();
	auto& Clients = ViewportManager.GetClients();
	const int32 LastClickedIdx = ViewportManager.GetLastClickedViewportIndex();

	if (LastClickedIdx < 0 || LastClickedIdx >= Clients.Num())
	{
		UE_LOG_WARNING("Pilot Mode: Invalid viewport index");
		return;
	}

	FViewportClient* TargetClient = Clients[LastClickedIdx];
	if (!TargetClient)
	{
		UE_LOG_WARNING("Pilot Mode: Invalid viewport client");
		return;
	}

	// Pilot Mode 진입
	bIsPilotMode = true;
	PilotedActor = SelectedActor.Get();
	PilotModeViewportIndex = LastClickedIdx;

	// 현재 뷰포트 위치 저장, 이후 해제 시 복원 예정
	PilotModeStartCameraLocation = TargetClient->GetViewLocation();
	PilotModeStartCameraRotation = TargetClient->GetViewRotation();

	// Actor의 Transform을 뷰포트에 적용
	if (USceneComponent* RootComp = PilotedActor->GetRootComponent())
	{
		FVector ActorLocation = RootComp->GetWorldLocation();
		FQuaternion ActorRotationQuat = RootComp->GetWorldRotationAsQuaternion();

		// Quaternion을 (Pitch, Yaw, Roll) Euler angles로 변환
		FVector EulerAngles = ActorRotationQuat.ToEuler();
		FVector ViewRotation = FVector(EulerAngles.Y, EulerAngles.Z, EulerAngles.X); // (Pitch, Yaw, Roll)

		TargetClient->SetViewLocation(ActorLocation);
		TargetClient->SetViewRotation(ViewRotation);

		// 기즈모를 원래 Actor 위치에 고정
		PilotModeFixedGizmoLocation = ActorLocation;
		Gizmo.SetFixedLocation(PilotModeFixedGizmoLocation);

		UE_LOG_INFO("Pilot Mode: Entered (Actor: %s)", PilotedActor->GetName().ToString().data());
	}
}

/**
 * @brief 뷰포트 카메라 입력 처리
 * Perspective: WASDQE 이동 + 마우스 드래그 회전
 * Ortho: 마우스 드래그로 InDelta 직접 적용하여 패닝
 */
void UEditor::UpdateViewportCameraInput()
{
	const UInputManager& Input = UInputManager::GetInstance();
	auto& ViewportManager = UViewportManager::GetInstance();

	// 활성 뷰포트 및 InputEnabled 체크
	for (int32 i = 0; i < ViewportManager.GetClients().Num(); ++i)
	{
		FViewportClient* Client = ViewportManager.GetClients()[i];
		if (!Client || !Client->GetInputEnabled())
		{
			continue;
		}

		// Perspective 뷰포트 처리
		if (!Client->IsOrtho())
		{
			FVector Direction = FVector::Zero();

			// WASDQE 키 이동
			if (Input.IsKeyDown(EKeyInput::A)) { Direction += -Client->GetRight() * 2; }
			if (Input.IsKeyDown(EKeyInput::D)) { Direction += Client->GetRight() * 2; }
			if (Input.IsKeyDown(EKeyInput::W)) { Direction += Client->GetForward() * 2; }
			if (Input.IsKeyDown(EKeyInput::S)) { Direction += -Client->GetForward() * 2; }
			if (Input.IsKeyDown(EKeyInput::Q)) { Direction += FVector(0, 0, -2); }
			if (Input.IsKeyDown(EKeyInput::E)) { Direction += FVector(0, 0, 2); }

			if (Direction.LengthSquared() > MATH_EPSILON)
			{
				Direction.Normalize();
			}

			constexpr float MoveSpeed = 20.0f;
			FVector NewLocation = Client->GetViewLocation() + Direction * MoveSpeed * DT;
			Client->SetViewLocation(NewLocation);

			// 마우스 드래그로 회전
			const FVector MouseDelta = Input.GetMouseDelta();
			constexpr float KeySensitivityDegPerPixel = 0.1f;

			const float YawDelta = MouseDelta.X * KeySensitivityDegPerPixel * 2;
			const float PitchDelta = -MouseDelta.Y * KeySensitivityDegPerPixel * 2;

			FVector CurrentRotation = Client->GetViewRotation();
			CurrentRotation.Y += YawDelta;   // Yaw
			CurrentRotation.X += PitchDelta; // Pitch
			CurrentRotation.Z = 0.0f;        // Roll

			// Pitch 클램핑
			constexpr float MaxPitch = 89.9f;
			CurrentRotation.X = clamp(CurrentRotation.X, -MaxPitch, MaxPitch);

			Client->SetViewRotation(CurrentRotation);
		}
		// Ortho 뷰포트 처리 (InDelta 직접 적용)
		else
		{
			const FVector MouseDelta = Input.GetMouseDelta();

			// InDelta: 마우스 이동량을 뷰포트 로컬 좌표로 직접 적용
			const FVector2 InDelta(MouseDelta.X, MouseDelta.Y);

			// Right/Up 벡터 가져오기
			const FVector Right = Client->GetRight();
			const FVector Up = Client->GetUp();

			// InDelta를 바로 적용 (스케일은 OrthoZoom 기반으로 조정)
			const float OrthoZoom = Client->GetOrthoZoom();
			constexpr float OrthoZoomFactor = 0.01f; // 조정 계수
			const float DragScale = OrthoZoom * OrthoZoomFactor;

			FVector PanDelta = Right * -InDelta.X * DragScale + Up * InDelta.Y * DragScale;
			FVector NewLocation = Client->GetViewLocation() + PanDelta;
			Client->SetViewLocation(NewLocation);
		}
	}
}

/**
 * @brief Pilot Mode 업데이트
 */
void UEditor::UpdatePilotMode()
{
	if (!bIsPilotMode || !PilotedActor)
	{
		return;
	}

	// Actor가 삭제되었는지 확인
	if (PilotedActor->IsPendingDestroy())
	{
		ExitPilotMode();
		return;
	}

	// 뷰포트의 현재 Transform을 Actor에 적용
	UViewportManager& ViewportManager = UViewportManager::GetInstance();
	auto& Clients = ViewportManager.GetClients();

	if (PilotModeViewportIndex >= 0 && PilotModeViewportIndex < Clients.Num())
	{
		FViewportClient* PilotClient = Clients[PilotModeViewportIndex];
		if (PilotClient)
		{
			USceneComponent* RootComp = PilotedActor->GetRootComponent();

			if (RootComp)
			{
				FVector ViewportLocation = PilotClient->GetViewLocation();
				FVector ViewportRotation = PilotClient->GetViewRotation(); // (Pitch, Yaw, Roll)

				// ViewRotation을 Quaternion으로 변환
				FVector Radians = FVector::GetDegreeToRadian(ViewportRotation);
				FMatrix RotationMatrix = FMatrix::CreateFromYawPitchRoll(Radians.Y, Radians.X, Radians.Z);
				FQuaternion RotationQuat = FQuaternion::FromRotationMatrix(RotationMatrix);

				// Actor에 Transform 적용
				RootComp->SetWorldLocation(ViewportLocation);
				RootComp->SetWorldRotation(RotationQuat);
			}
		}
	}
}

/**
 * @brief Pilot Mode 해제
 */
void UEditor::ExitPilotMode()
{
	if (!bIsPilotMode)
	{
		return;
	}

	// 뷰포트를 시작 위치로 복원
	UViewportManager& ViewportManager = UViewportManager::GetInstance();
	auto& Clients = ViewportManager.GetClients();

	if (PilotModeViewportIndex >= 0 && PilotModeViewportIndex < Clients.Num())
	{
		FViewportClient* PilotClient = Clients[PilotModeViewportIndex];
		if (PilotClient)
		{
			PilotClient->SetViewLocation(PilotModeStartCameraLocation);
			PilotClient->SetViewRotation(PilotModeStartCameraRotation);
		}
	}

	if (PilotedActor)
	{
		UE_LOG_INFO("Pilot Mode: Exited (Actor: %s)", PilotedActor->GetName().ToString().data());
	}

	// 기즈모 고정 위치 해제
	Gizmo.ClearFixedLocation();

	bIsPilotMode = false;
	PilotedActor = nullptr;
	PilotModeViewportIndex = -1;
}

/**
 * @brief Pilot Mode 종료 요청을 위한 외부 호출 함수
 */
void UEditor::RequestExitPilotMode()
{
	ExitPilotMode();
}
