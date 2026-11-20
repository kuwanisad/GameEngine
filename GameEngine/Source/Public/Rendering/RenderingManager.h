#pragma once
#include "../Core/CoreMinimal.h"
#include "../Asset/AssetHandle.h"
#include "../Asset/SkeletalMeshInstance.h"
#include "../Animation/AnimGraphRegistry.h"
#include "../Utilities/TPointerSlotArray.h"
#include "../Rendering/RenderingStatics.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <queue>

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
	bool IsInitCalled;

	bool IsShuttingDown;

	URenderer* Renderer;
	void RenderFrame(FShaderPipeline* Pipeline);

	bool IsHandleValid(const FInstanceHandle& Handle);
	FSkeletalMeshInstance* ResolveInstanceHandle(const FInstanceHandle& Handle);

	TArray<std::shared_ptr<FSkeletalMeshInstance>> SkeletalMeshInstances;
	TArray<uint32> EmptyIndices_SkeletalMeshInstances;

	TPointerSlotArray<USpriteComponent*> SpriteComps;

	TPointerSlotArray<UStaticMeshComponent*> MeshComps;

	//밀린 프레임
	std::atomic<DelayedFrame> DelayedFrameCount{ 0 };

	//렌더링이 진행될 때 누적된 델타
	float AccumulatedDeltaSeconds = 0.0f;

	void ProcessInstances();

public:
	bool Init(float ScreenWidth, float ScreenHeight);
	void Shutdown();

	URenderer* GetRenderer() const { return Renderer; }

	void Draw();

	bool ScheduleRendering(float DeltaSeconds);
	void FlushAndDispatch();

	void RegisterSpriteComp(USpriteComponent* SpriteComp);
	void UnregisterSpriteComp(USpriteComponent* SpriteComp);

	void RegisterMeshComp(UStaticMeshComponent* MeshComp);
	void UnregisterMeshComp(UStaticMeshComponent* MeshComp);

	FInstanceHandle RegisterInstance(UAnimInstance* OwningAnimInstance, FAssetHandle SkeletonHandle, FAssetHandle SkeletalMeshHandle, FAnimGraphHandle AnimGraphHandle);
	void UnregisterInstance(FInstanceHandle Handle);

	void UpdateSkeletalMeshTransform(FInstanceHandle Handle, const FVector3& Location, const FQuaternion& Rotation, const FVector3& Scale);

	const TPointerSlotArray<UStaticMeshComponent*>& GetStaticMeshComps() const { return MeshComps; }
	const TPointerSlotArray<USpriteComponent*>& GetSpriteComps() const { return SpriteComps; }

	TArray<std::shared_ptr<FSkeletalMeshInstance>>& GetSkeletalMeshInstances() { return SkeletalMeshInstances; }
	TArray<uint32>& GetSkeletalMeshInstancesEmptyIndices() { return EmptyIndices_SkeletalMeshInstances; }
};