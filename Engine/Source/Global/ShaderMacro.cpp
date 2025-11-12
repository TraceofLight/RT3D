#include "pch.h"
#include "Source/Global/ShaderMacro.h"

#define FEATURE_FLAG_MACRO(FeatureFlag) \
else if (HasFeatureFlag(ShaderFeatureFlag, EShaderFeatureFlag::FeatureFlag))	\
{	\
	Macros.Add(D3D_SHADER_MACRO{ ToString(EShaderFeatureFlag::FeatureFlag).c_str(), "1" });	\
}	\

void GetMacro(TArray<D3D_SHADER_MACRO>& Macros, const EShaderVariant ShaderVariant, const EShaderFeatureFlag ShaderFeatureFlag)
{
	if (ShaderVariant != EShaderVariant::None)
	{
		Macros.Add(D3D_SHADER_MACRO{ ToConstChar(ShaderVariant), "1" });
	}
#define ShaderFeatureFlagEnum(FeatureFlag) \
    if (HasFeatureFlag(ShaderFeatureFlag, EShaderFeatureFlag::FeatureFlag)) \
    { \
        Macros.Add(D3D_SHADER_MACRO{ #FeatureFlag, "1" }); \
    }

	SHADER_FEATURE_FLAG_LIST
#undef ShaderFeatureFlagEnum

	Macros.Add(D3D_SHADER_MACRO{ nullptr,nullptr });
}

std::wstring MakeMacroPath(const std::wstring& FilePath, const EShaderVariant ShaderVariant, const EShaderFeatureFlag ShaderFeatureFlag)
{
	namespace fs = std::filesystem;

	fs::path filePath(FilePath);
	fs::path dir = filePath.parent_path();         // 디렉터리 경로
	std::wstring stem = filePath.stem().wstring(); // filename
	std::wstring extension = filePath.extension().wstring(); // .hlsl

	std::wstringstream wss;
	wss << stem << L"_"
		<< ToConstChar(ShaderVariant)
		<< L"_"
		<< static_cast<uint32_t>(ShaderFeatureFlag)
		<< extension;

	return dir / wss.str();
}
