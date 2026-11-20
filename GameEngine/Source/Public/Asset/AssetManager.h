#pragma once
#include "../Core/CoreMinimal.h"
#include "../Asset/AssetHandle.h"
#include "../Rendering/RenderingStatics.h"

#include <GL/glew.h>

class UTexture;
class UMesh;
struct FShaderPipeline;
class USkeleton;
class UAnimSequence;
class USkeletalMesh;

struct FVertexContainer;

class UAssetManager
{
private:
	static UAssetManager* AssetManager;

	FAssetHandle NextAssetHandle = 1;

	TMap<FAssetHandle, USkeleton*> Skeletons;
	TMap<FAssetHandle, UAnimSequence*> AnimSequences;
	TMap<FAssetHandle, USkeletalMesh*> SkeletalMeshes;

	//텍스처, 셰이더를 매시가 멤버로 가질 수 있지만 근본적으로 서로 다른 에셋이므로...
	TMap<FString, UTexture*> Textures;
	TMap<FString, FShaderPipeline*> ShaderPipelines;
	TMap<FString, UMesh*> Meshes;

	FAssetHandle GetNewAssetHandle();

	bool CompileShader(const FString& FileName, GLenum ShaderType, GLuint& OutShader);
	bool IsShaderCompiled(GLuint Shader);
	bool IsShaderProgramValid(const FShaderPipeline* ShaderPipeline);
	bool IsShaderProgramValid(GLuint ProgramID);

	void PackSkeletonVertexBuffer(const TArray<FSoftSkinVertex>& SoftVertices, TArray<uint8>& OutBuffer);
public:
	static UAssetManager* Get();
	static FAssetHandle InvalidHandle;
	
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

	FAssetHandle LoadSkeleton(const FString& FileName);
	FAssetHandle LoadAnimSequence(const FString& FileName, const UReferenceSkeleton& TargetSkeleton, float SampleRate);
	FAssetHandle LoadSkeletalMesh(const FString& FileName);

	const USkeleton* GetSkeleton(FAssetHandle Handle) const;
	UAnimSequence* GetAnimSequence(FAssetHandle Handle) const;
	USkeletalMesh* GetSkeletalMesh(FAssetHandle Handle) const;
};