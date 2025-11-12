#include "pch.h"
#include "Texture/Public/Material.h"
#include "Texture/Public/Texture.h"
#include "Texture/Public/TextureRenderProxy.h"

IMPLEMENT_CLASS(UMaterial, UObject)

UMaterial::UMaterial() = default;

UMaterial::~UMaterial() = default;

void UMaterial::CopyFrom(const UMaterial* Other)
{
	if (!Other) return;

	this->MaterialData = Other->MaterialData;
	this->DiffuseTexture = Other->DiffuseTexture;
	this->AmbientTexture = Other->AmbientTexture;
	this->SpecularTexture = Other->SpecularTexture;
	this->NormalTexture = Other->NormalTexture;
	this->OpacityTexture = Other->OpacityTexture;
	this->BumpTexture = Other->BumpTexture;
}
