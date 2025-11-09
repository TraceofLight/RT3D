#pragma once
#include "Core/Public/Object.h"
#include "Core/Public/Archive.h"

class UTexture;

/**
 * @note: This struct is exactly same as the one defined in ObjImporter.h.
 * This is intentionally introduced for abstractional layer of material object.
 */
struct FMaterial
{
	FString Name;
	FVector Ambient;
	FVector Diffuse{0.1f, 0.1f, 0.1f};
	FVector Specular;
	FVector Emissive;

	float Shininess;
	float RefractiveIndex;
	float Opacity;
	int32 IlluminationModel;

	// 텍스처 맵 경로
	FString AmbientTexturePath;
	FString DiffuseTexturePath;
	FString SpecularTexturePath;
	FString ShininessTexturePath;
	FString OpacityTexturePath;
	FString NormalTexturePath;
};

inline FArchive& operator<<(FArchive& Ar, FMaterial& Material)
{
	Ar << Material.Name;
	Ar << Material.Ambient;
	Ar << Material.Diffuse;
	Ar << Material.Specular;
	Ar << Material.Emissive;
	Ar << Material.Shininess;
	Ar << Material.RefractiveIndex;
	Ar << Material.Opacity;
	Ar << Material.IlluminationModel;
	Ar << Material.AmbientTexturePath;
	Ar << Material.DiffuseTexturePath;
	Ar << Material.SpecularTexturePath;
	Ar << Material.ShininessTexturePath;
	Ar << Material.OpacityTexturePath;
	Ar << Material.NormalTexturePath;
	return Ar;
}

UCLASS()
class UMaterial : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UMaterial, UObject)

public:
	UMaterial();
	~UMaterial() override;

	// 머티리얼 데이터 접근자
	FVector GetAmbientColor() const { return MaterialData.Ambient; }
	FVector GetDiffuseColor() const { return MaterialData.Diffuse; }
	FVector GetSpecularColor() const { return MaterialData.Specular; }
	FVector GetEmissiveColor() const { return MaterialData.Emissive; }
	float GetShininess() const { return MaterialData.Shininess; }
	float GetRefractiveIndex() const { return MaterialData.RefractiveIndex; }
	float GetOpacity() const { return MaterialData.Opacity; }

	// 텍스처 접근자
	UTexture* GetDiffuseTexture() const { return DiffuseTexture; }
	UTexture* GetAmbientTexture() const { return AmbientTexture; }
	UTexture* GetSpecularTexture() const { return SpecularTexture; }
	UTexture* GetNormalTexture() const { return NormalTexture; }
	UTexture* GetOpacityTexture() const { return OpacityTexture; }
	UTexture* GetBumpTexture() const { return BumpTexture; }

	const FMaterial& GetMaterialData() const { return MaterialData; }

	void SetAmbientColor(const FVector& InColor) { MaterialData.Ambient = InColor; }
	void SetDiffuseColor(const FVector& InColor) { MaterialData.Diffuse = InColor; }
	void SetSpecularColor(const FVector& InColor) { MaterialData.Specular = InColor; }
	void SetEmissiveColor(const FVector& InColor) { MaterialData.Emissive = InColor; }

	void SetDiffuseTexture(UTexture* InTexture) { DiffuseTexture = InTexture; }
	void SetAmbientTexture(UTexture* InTexture) { AmbientTexture = InTexture; }
	void SetSpecularTexture(UTexture* InTexture) { SpecularTexture = InTexture; }
	void SetNormalTexture(UTexture* InTexture) { NormalTexture = InTexture; }
	void SetOpacityTexture(UTexture* InTexture) { OpacityTexture = InTexture; }
	void SetBumpTexture(UTexture* InTexture) { BumpTexture = InTexture; }

	void SetMaterialData(const FMaterial& InMaterialData) { MaterialData = InMaterialData; }

private:
	FMaterial MaterialData;

	UTexture* DiffuseTexture = nullptr;
	UTexture* AmbientTexture = nullptr;
	UTexture* SpecularTexture = nullptr;
	UTexture* NormalTexture = nullptr;
	UTexture* OpacityTexture = nullptr;
	UTexture* BumpTexture = nullptr;
};
