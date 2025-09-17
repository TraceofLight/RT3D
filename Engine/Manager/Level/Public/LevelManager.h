#pragma once
#include "Core/Public/Object.h"
//#include "Editor/Public/Editor.h"

class UEditor;
class ULevel;
struct FLevelMetadata;

UCLASS()
class ULevelManager :
	public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(ULevelManager, UObject)

public:
	void Update() const;
	void CreateDefaultLevel();
	void RegisterLevel(const FName& InName, ULevel* InLevel);
	void LoadLevel(const FName& InName);
	void Shutdown();

	// Getter
	ULevel* GetCurrentLevel() const { return CurrentLevel; }

	// Save & Load System
	bool SaveCurrentLevel(const FString& InFilePath) const;
	bool LoadLevel(const FString& InLevelName, const FString& InFilePath);
	bool CreateNewLevel(const FString& InLevelName);
	static path GetLevelDirectory();
	static path GenerateLevelFilePath(const FString& InLevelName);

	// Metadata Conversion Functions
	static FLevelMetadata ConvertLevelToMetadata(ULevel* InLevel);
	static bool LoadLevelFromMetadata(ULevel* InLevel, const FLevelMetadata& InMetadata);

	UEditor* GetEditor()
	{
		return Editor;
	}



private:
	ULevel* CurrentLevel;
	TMap<FName, ULevel*> Levels;
	UEditor* Editor;
};
