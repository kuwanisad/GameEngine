#pragma once
#include "../Core/Math.h"
#include "../Asset/SkeletalMeshInstance.h"

#include <functional>
#include <condition_variable>
#include <atomic>
#include <SDL.h>

struct FShaderPipeline;
struct FVertexContainer;

class URenderingManager;

struct FDirectionalLight
{
	FVector3 Direction;
	FVector3 DiffuseColor;
	FVector3 SpecColor;
};

class URenderer
{
private:
	FShaderPipeline* ShaderPipeline_Sprite;
	FShaderPipeline* ShaderPipeline_Mesh;
	FShaderPipeline* ShaderPipeline_Skin;

	FVertexContainer* VertexContainer_Sprite;

	FMatrix4x4 Matrix_View;
	FMatrix4x4 Matrix_Projection;
	float ScreenWidth;
	float ScreenHeight;

	SDL_Window* Window;
	SDL_GLContext Context;

	FVector3 AmbientLight;
	FDirectionalLight DirectionalLight;

	URenderingManager* RenderingManager;

	std::thread WorkerThread;
	std::mutex QueueMutex;
	std::condition_variable QueueCV;
	TArray<std::function<void()>> PendingCommands;

	bool IsShuttingDown = false;

	//마지막 프레임에 실행된 혹은 남은 작업 수
	std::atomic<uint32_t> PendingJobCount{ 0 };

	bool LoadShaders();

	//참고서의 함수
	void CreateSpriteVerts();

	void WorkerLoop();
	void UploadAndDraw(FSkeletalMeshInstance& Instance, FShaderPipeline* SkinnedShader);

public:
	URenderer();
	~URenderer();

	//참고서의 함수를 수정했음
	bool Init();

	void Shutdown();
	void UnloadData();

	void BeginFrame();
	void RenderFrame();
	void EndFrame();

	void SetViewMatrix(const FMatrix4x4& InView) { Matrix_View = InView; }
	FMatrix4x4 GetViewProjMatrix() const { return Matrix_View * Matrix_Projection; }

	void SetScreenWidth(float InScreenWidth) { ScreenWidth = InScreenWidth; }
	float GetScreenWidth() const { return ScreenWidth; }

	void SetScreenHeight(float InScreenHeight) { ScreenHeight = InScreenHeight; }
	float GetScreenHeight() const { return ScreenHeight; }

	FShaderPipeline* GetSkinnedShader() const { return ShaderPipeline_Skin; }

	void SetLightUniforms(FShaderPipeline* InShader) const;

	void SetAmbientLight(FVector3& InAmbientLight) { AmbientLight = InAmbientLight; }
	FDirectionalLight& GetDirectionalLight() { return DirectionalLight; }

	void SetRenderingManager(URenderingManager* InManager) { RenderingManager = InManager; }

	void SetIsShuttingDown(bool Value) { IsShuttingDown = Value; }

	void DispatchToWorker(FSkeletalMeshInstance& Instance, FAnimSnapshotBase* Snapshot, float DeltaSeconds);

	bool IsPendingJob();
};