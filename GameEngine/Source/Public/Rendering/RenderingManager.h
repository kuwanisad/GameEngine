#pragma once
#include "../Core/CoreMinimal.h"
#include "../Asset/AssetHandle.h"
#include "../Asset/SkeletalMeshInstance.h"
#include "../Animation/AnimGraphRegistry.h"
#include "../Utilities/TSlotArray.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <queue>

using DelayedFrame = short;

class URenderer;
class UEngine;
class USpriteComponent;
class UStaticMeshComponent;

struct FShaderPipeline;

/*
메인 스레드에서 랜더링을 요청하여 비동기로 진행됨.
 메인 스레드가 여러 틱 흘러도 렌더링이 지연되면 메인 스레드도 틱이 흐르지 않음
*/
class URenderingManager
{
private:
	static int32 PaletteBufferCount;
	static bool IsStartUpCalled;

	URenderer* Renderer;
	void RenderFrame(FShaderPipeline* Pipeline);

	bool IsHandleValid(const FInstanceHandle& Handle);
	FSkeletalMeshInstance* ResolveInstanceHandle(const FInstanceHandle& Handle);

	void ProcessInstance(FSkeletalMeshInstance& Instance);
	void DispatchToWorker(FSkeletalMeshInstance& Instance, FAnimSnapshotBase* Snapshot, float DeltaSeconds);
	void UploadAndDraw(FSkeletalMeshInstance& Instance, FShaderPipeline* SkinnedShader);
	void WorkerLoop();
	
	TArray<FSkeletalMeshInstance> SkeletalMeshInstances;
	TArray<uint32> EmptyIndices_SkeletalMesh;

	TArray<FSkeletalMeshInstance> SkeletalMeshInstances;
	TArray<uint32> EmptyIndices_SkeletalMeshInstances;

	TPointerSlotArray<USpriteComponent*> SpriteComps;

	TPointerSlotArray<UStaticMeshComponent*> MeshComps;

	std::thread WorkerThread;
	std::mutex QueueMutex;
	std::condition_variable QueueCV;
	TArray<std::function<void()>> PendingCommands;
	bool IsShuttingDown = false;

	//마지막 프레임에 실행된 혹은 남은 작업 수
	std::atomic<uint32_t> PendingJobCount{ 0 };

	//밀린 프레임
	std::atomic<DelayedFrame> DelayedFrameCount{ 0 };

	//이 값과 DelayedFrameCount를 같이 써서 렌더링 지연을 구현해야 함
	static constexpr DelayedFrame MaxGlobalDelayedFrames = 1;

	//렌더링이 진행될 때 누적된 델타
	float AccumulatedDeltaSeconds = 0.0f;

	uint32 GetArrayEmptySlot();

public:
	//모델이 사용하는 본 최대 개수. 이걸 변경하려면 셰이더도 맞춰야 함
	static constexpr int32 MaxBoneMatrices = 96;

	static void SetPaletteBufferCount(int32 InPaletteBufferCount);
	static int32 GetPaletteBufferCount() { return PaletteBufferCount; }

	bool Init(UEngine* InEngine, float ScreenWidth, float ScreenHeight);
	bool Shutdown();

	URenderer* GetRenderer() const { return Renderer; }

	void Draw();

	void RegisterSpriteComp(USpriteComponent* SpriteComp);
	void UnregisterSpriteComp(USpriteComponent* SpriteComp);

	void RegisterMeshComp(UStaticMeshComponent* MeshComp);
	void UnregisterMeshComp(UStaticMeshComponent* MeshComp);

	FInstanceHandle RegisterInstance(UAnimInstance* OwningAnimInstance, FAssetHandle SkeletonHandle, FAssetHandle SkeletalMeshHandle, FAnimGraphHandle AnimGraphHandle);
	void UnregisterInstance(FInstanceHandle Handle);

	void UpdateSkeletalMeshTransform(FInstanceHandle Handle, const FVector3& Location, const FQuaternion& Rotation, const FVector3& Scale);
};