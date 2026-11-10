#include "../../Public/Rendering/RenderingManager.h"
#include "../../Public/Rendering/Shader.h"
#include "../../Public/Rendering/RenderingStatics.h"
#include "../../Public/rendering/Renderer.h"

void URenderingManager::RenderFrame(FShaderPipeline* Pipeline)
{
    Pipeline->SetActive();
    Pipeline->SetMatrix(URenderingStatics::Shader_uViewProj.c_str(), GetRenderer()->GetViewProjMatrix());

    //TODO
}

bool URenderingManager::IsHandleValid(const FInstanceHandle& Handle)
{
    return Handle.Index < SkeletalMeshInstances.size();
}

FSkeletalMeshInstance* URenderingManager::ResolveInstanceHandle(const FInstanceHandle& Handle)
{
    if (IsHandleValid(Handle) == false)
    {
        return nullptr;
    }

    FSkeletalMeshInstance& Instance = SkeletalMeshInstances[Handle.Index];
    if (!Instance.IsSlotOccupied || Instance.Generation != Handle.Generation)
    {
        return nullptr; //이미 다른 객체가 점유
    }

    return &Instance;
}

void URenderingManager::ProcessInstance(FSkeletalMeshInstance& Instance)
{
}

void URenderingManager::DispatchToWorker(FSkeletalMeshInstance& Instance, FAnimSnapshotBase* Snapshot, float DeltaSeconds)
{
}

void URenderingManager::UploadAndDraw(FSkeletalMeshInstance& Instance, FShaderPipeline* SkinnedShader)
{
}

void URenderingManager::WorkerLoop()
{
}

void URenderingManager::SetPaletteBufferCount(int32 InPaletteBufferCount)
{
    PaletteBufferCount = InPaletteBufferCount;
}

bool URenderingManager::Init(UEngine* InEngine, float ScreenWidth, float ScreenHeight)
{
    return false;
}

bool URenderingManager::Shutdown()
{
    return false;
}

void URenderingManager::Draw()
{
}

void URenderingManager::RegisterSpriteComp(USpriteComponent* SpriteComp)
{
}

void URenderingManager::UnregisterSpriteComp(USpriteComponent* SpriteComp)
{
}

void URenderingManager::RegisterMeshComp(UStaticMeshComponent* MeshComp)
{
}

void URenderingManager::UnregisterMeshComp(UStaticMeshComponent* MeshComp)
{
}

FInstanceHandle URenderingManager::RegisterInstance(UAnimInstance* OwningAnimInstance, FAssetHandle SkeletonHandle, FAssetHandle SkeletalMeshHandle, FAnimGraphHandle AnimGraphHandle)
{
    uint32 Index;

    if (EmptyIndices_SkeletalMeshInstances.empty() == false)
    {
        Index = EmptyIndices_SkeletalMeshInstances.front();
        EmptyIndices_SkeletalMeshInstances.pop_back();
    }
    else
    {
        Index = EmptyIndices_SkeletalMeshInstances.size();
        SkeletalMeshInstances.emplace_back(FSkeletalMeshInstance());
    }

    SkeletalMeshInstances.emplace_back(FSkeletalMeshInstance());

    FSkeletalMeshInstance& NewInstance = SkeletalMeshInstances[Index];
    NewInstance.SelfIndex = Index;
    NewInstance.IsAlive = true;
    NewInstance.IsSlotOccupied = true;
    NewInstance.SkeletonHandle = SkeletonHandle;
    NewInstance.SkeletalMeshHandle = SkeletalMeshHandle;
    NewInstance.OwningAnimInstance = OwningAnimInstance;
    NewInstance.Graph = UAnimGraphRegistry::Get().CreateGraph(AnimGraphHandle);
    NewInstance.CurrentSnapshot = nullptr;
    NewInstance.RenderingPalette.assign(PaletteBufferCount, TArray<FMatrix4x4>(MaxBoneMatrices, FMatrix4x4::Identity));
    NewInstance.ReadIndex = 0;
    NewInstance.IsHasPendingSwap = false;

    return FInstanceHandle{ Index, NewInstance.Generation };
}

void URenderingManager::UnregisterInstance(FInstanceHandle Handle)
{
    FSkeletalMeshInstance* Instance = ResolveInstanceHandle(Handle);
    if (Instance != nullptr)
    {
        // 워커 스레드가 쓰는 중일 수 있어서 RenderFrame에서 처리함
        Instance->IsAlive = false;
    }
}

void URenderingManager::UpdateSkeletalMeshTransform(FInstanceHandle Handle, const FVector3& Location, const FQuaternion& Rotation, const FVector3& Scale)
{
    FSkeletalMeshInstance* Instance = ResolveInstanceHandle(Handle);
    if (Instance != nullptr)
    {
        Instance->LatestWorldPosition = Location;
        Instance->LatestWorldRotation = Rotation;
        Instance->LatestWorldScale = Scale;
    }
}
