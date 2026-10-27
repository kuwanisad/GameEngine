#pragma once
#include "../Core/CoreMinimal.h"

class UTexture;
class UMesh;
class FShaderPipeline;

struct FVertexContainer;

class UAssetManager
{
private:
	static UAssetManager* AssetManager;

	//텍스처, 셰이더를 매시가 멤버로 가질 수 있지만 근본적으로 서로 다른 에셋이므로...
	TMap<FString, UTexture*> Textures;
	TMap<FString, FShaderPipeline*> ShaderPipelines;
	TMap<FString, UMesh*> Meshes;

	bool CompileShader(const FString& FileName, GLenum ShaderType, GLuint& OutShader);
	bool IsShaderCompiled(GLuint Shader);
	bool IsShaderProgramValid(const FShaderPipeline* ShaderPipeline);
	bool IsShaderProgramValid(GLuint ProgramID);

public:
	static UAssetManager* Get();
	
	UMesh* LoadMesh(const FString& FileName);
	bool UnloadMesh(const UMesh* Mesh);
	UMesh* GetMesh(const FString& Name);

	FShaderPipeline* LoadShaderPipeline(const FString& PipelineName, const FString& VertName, const FString& FragName);
	bool UnloadShaderPipeline(FShaderPipeline* ShaderPipeline);
	FShaderPipeline* GetShaderPipeline(const FString& Name);

	UTexture* LoadTexture(const FString& FileName);
	UTexture* CreateWhite1x1();
	bool UnloadTexture(const UTexture* Texture);
	UTexture* GetTexture(const FString& Name);
};