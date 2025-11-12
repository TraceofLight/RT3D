#pragma once
#include <string>
#include "Source/Global/FString.h"
#include "Source/Global/Types.h"

#define SHADER_VARIANT_LIST	\
ShaderVariantEnum(LIGHTING_MODEL_GOURAUD)	\
ShaderVariantEnum(LIGHTING_MODEL_LAMBERT)	\
ShaderVariantEnum(LIGHTING_MODEL_BLINNPHONG)	\
ShaderVariantEnum(LIGHTING_MODEL_NORMAL)	\

enum class EShaderVariant
{
	None = 0,
#define ShaderVariantEnum(name) name,
	SHADER_VARIANT_LIST
#undef ShaderVariantEnum
};

inline const char* ToConstChar(EShaderVariant InShaderVariant)
{
	switch (InShaderVariant)
	{
	case EShaderVariant::None:
		return "None";
#define ShaderVariantEnum(name) case EShaderVariant::name: return #name;
		SHADER_VARIANT_LIST
#undef ShaderVariantEnum
	}
	return "UnKnown";
}


//FeatureFlag는 총 32개 (0~31)까지 사용가능
#define SHADER_FEATURE_FLAG_LIST \
ShaderFeatureFlagEnum(DUMMY_EXAMPLE, 0)	\
ShaderFeatureFlagEnum(DUMMY_EXAMPLE1, 1)	\

enum class EShaderFeatureFlag
{
	None = 0,
#define ShaderFeatureFlagEnum(name, bit) name = 1 << bit,
	SHADER_FEATURE_FLAG_LIST
#undef ShaderFeatureFlagEnum
};

inline EShaderFeatureFlag operator |(const EShaderFeatureFlag Flag1, const EShaderFeatureFlag Flag2)
{
	return static_cast<EShaderFeatureFlag>(static_cast<uint32>(Flag1) + static_cast<uint32>(Flag2));
}

inline bool HasFeatureFlag (const EShaderFeatureFlag Flag, const EShaderFeatureFlag ShaderFeatureFlag)
{
	return static_cast<uint32>(Flag) & static_cast<uint32>(ShaderFeatureFlag);
}

void GetMacro(TArray<D3D_SHADER_MACRO>& Macros, const EShaderVariant ShaderVariant, const EShaderFeatureFlag ShaderFeatureFlag);
std::wstring MakeMacroPath(const std::wstring& FilePath, const EShaderVariant ShaderVariant, const EShaderFeatureFlag ShaderFeatureFlag);
