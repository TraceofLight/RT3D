#pragma once
#include "Mesh/Public/ActorComponent.h"
#include "ResourceManager.h"

UCLASS()
class USceneComponent : public UActorComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(USceneComponent, UActorComponent)

public:
	USceneComponent();

	void SetParentAttachment(USceneComponent* SceneComponent);
	void RemoveChild(USceneComponent* ChildDeleted);

	void MarkAsDirty();

	void SetRelativeLocation(const FVector& Location);
	void SetRelativeRotation(const FVector& Rotation);
	void SetRelativeScale3D(const FVector& Scale);
	void SetUniformScale(bool bIsUniform);

	bool IsUniformScale() const;

	const FVector& GetRelativeLocation() const;
	const FVector& GetRelativeRotation() const;
	const FVector& GetRelativeScale3D() const;

	const FMatrix& GetWorldTransformMatrix() const;
	const FMatrix& GetWorldTransformMatrixInverse() const;

private:
	mutable bool bIsTransformDirty = true;
	mutable bool bIsTransformDirtyInverse = true;
	mutable FMatrix WorldTransformMatrix;
	mutable FMatrix WorldTransformMatrixInverse;

	USceneComponent* ParentAttachment = nullptr;
	TArray<USceneComponent*> Children;
	FVector RelativeLocation = FVector{ 0,0,0.f };
	FVector RelativeRotation = FVector{ 0,0,0.f };
	FVector RelativeScale3D = FVector{ 0.3f,0.3f,0.3f };
	bool bIsUniformScale = false;
	const float MinScale = 0.01f;
};

UCLASS()
class UPrimitiveComponent : public USceneComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UPrimitiveComponent, USceneComponent)

public:
	UPrimitiveComponent();

	const TArray<FVertex>* GetVerticesData() const;
	ID3D11Buffer* GetVertexBuffer() const;
	const FRenderState& GetRenderState() const { return RenderState; }

	void SetTopology(D3D11_PRIMITIVE_TOPOLOGY InTopology);
	D3D11_PRIMITIVE_TOPOLOGY GetTopology() const;
	//void Render(const URenderer& Renderer) const override;

	bool IsVisible() const { return bVisible; }
	void SetVisibility(bool bVisibility) { bVisible = bVisibility; }

	FVector4 GetColor() const { return Color; }
	void SetColor(const FVector4& InColor) { Color = InColor; }

protected:
	const TArray<FVertex>* Vertices = nullptr;
	FVector4 Color = FVector4{ 0.f,0.f,0.f,0.f };
	ID3D11Buffer* Vertexbuffer = nullptr;
	uint32 NumVertices = 0;
	D3D11_PRIMITIVE_TOPOLOGY Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	FRenderState RenderState = {};
	EPrimitiveType Type = EPrimitiveType::Cube;

	bool bVisible = true;

};

UCLASS()
class UTriangleComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UTriangleComponent, UPrimitiveComponent)

public:
	UTriangleComponent();
};

UCLASS()
class USquareComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(USquareComponent, UPrimitiveComponent)

public:
	USquareComponent();
};

UCLASS()
class UCubeComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UCubeComponent, UPrimitiveComponent)

public:
	UCubeComponent();
};

UCLASS()
class USphereComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(USphereComponent, UPrimitiveComponent)
public:
	USphereComponent();
};

UCLASS()
class ULineComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(ULineComponent, UPrimitiveComponent)
public:
	ULineComponent();
};
