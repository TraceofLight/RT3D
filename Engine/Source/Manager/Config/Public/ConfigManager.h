#pragma once
#include "Global/Types.h"
#include "Core/Public/Object.h"
#include "Core/Public/Name.h"

UCLASS()
class UConfigManager : public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UConfigManager, UObject)

public:
	void SaveEditorSetting();
	void LoadEditorSetting();

	float GetCellSize() const
	{
		return CellSize;
	}

	float GetCameraSensitivity() const
	{
		return CameraSensitivity;
	}

	void SetCellSize(const float cellSize)
	{
		CellSize = cellSize;
	}

	void SetCameraSensitivity(const float cameraSensitivity)
	{
		CameraSensitivity = cameraSensitivity;
	}


private:
	FName EditorIniFileName;
	float CellSize;
	float CameraSensitivity;
};
