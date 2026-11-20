#include "../../Public/Rendering/RenderingManager.h"
#include "../../Public/Rendering/Shader.h"
#include "../../Public/Rendering/RenderingStatics.h"
#include "../../Public/rendering/Renderer.h"
#include "../../Public/Asset/Skeleton.h"
#include "../../Public/Asset/AssetManager.h"
#include "../../Public/Animation/AnimInstance.h"

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

    std::shared_ptr<FSkeletalMeshInstance>& Instance = SkeletalMeshInstances[Handle.Index];
    if (!Instance->IsSlotOccupied || Instance->Generation != Handle.Generation)
    {
        return nullptr; //이미 다른 객체가 점유
    }

    return Instance.get();
}

void URenderingManager::ProcessInstances()
{
    auto& Instances = GetSkeletalMeshInstances();

    for (int32 i = 0; i < Instances.size(); ++i)
    {
        auto& CurrentInstance = Instances[i];

        if (CurrentInstance->IsSlotOccupied && CurrentInstance->IsAlive)
        {
            if (CurrentInstance->RenderFence.IsFenceComplete())
            {
                delete CurrentInstance->Graph;
                CurrentInstance->Graph = nullptr;
                delete CurrentInstance->CurrentSnapshot;
                CurrentInstance->CurrentSnapshot = nullptr;
                CurrentInstance->IsSlotOccupied = false;
                CurrentInstance->Generation++;   //워커 스레드가 이전 번호면 구버전으로 인식하도록
                EmptyIndices_SkeletalMeshInstances.push_back(CurrentInstance->SelfIndex);
            }
        }
    }
}

bool URenderingManager::Init(float ScreenWidth, float ScreenHeight)
{
    IsInitCalled = true;

    Renderer = new URenderer();
    Renderer->SetRenderingManager(this);

    Renderer->SetScreenWidth(ScreenWidth);
    Renderer->SetScreenHeight(ScreenHeight);

    if (Renderer->Init() == false)
    {
        delete Renderer;
        Renderer = nullptr;
        IsInitCalled = false;
        return false;
    }

    IsShuttingDown = false;
    Renderer->SetIsShuttingDown(IsShuttingDown);

    return true;
}

void URenderingManager::Shutdown()
{
    if (Renderer != nullptr)
    {
        Renderer->UnloadData();
        Renderer->Shutdown();
        delete Renderer;
        Renderer = nullptr;
    }

    IsInitCalled = false;
}

void URenderingManager::Draw()
{
    Renderer->BeginFrame();
    Renderer->RenderFrame();
    Renderer->EndFrame();
}

bool URenderingManager::ScheduleRendering(float DeltaSeconds)
{
    AccumulatedDeltaSeconds += DeltaSeconds;

    if (Renderer->IsPendingJob())
    {
        DelayedFrameCount++;
        if (DelayedFrameCount > URenderingStatics::MaxGlobalDelayedFrames)
        {
            while (Renderer->IsPendingJob())
            {
                std::this_thread::yield();
            }
            DelayedFrameCount = 0;
            return true;
        }

        return false;
    }

    DelayedFrameCount = 0;
    return true;
}

void URenderingManager::FlushAndDispatch()
{
    ProcessInstances();

    for (uint32 i = 0; i < GetSkeletalMeshInstances().size(); ++i)
    {
        std::shared_ptr<FSkeletalMeshInstance>& Instance = SkeletalMeshInstances[i];
        if (Instance->IsSlotOccupied == false || !Instance->IsAlive)
        {
            continue;
        }
        FAnimSnapshotBase* NewSnapshot = Instance->OwningAnimInstance->GetSnapshot();
        Renderer->DispatchToWorker(*Instance, NewSnapshot, AccumulatedDeltaSeconds);
    }

    AccumulatedDeltaSeconds = 0.f;
}

void URenderingManager::RegisterSpriteComp(USpriteComponent* SpriteComp)
{
    SpriteComps.Add(SpriteComp);
}

void URenderingManager::UnregisterSpriteComp(USpriteComponent* SpriteComp)
{
    SpriteComps.Remove(SpriteComp);
}

void URenderingManager::RegisterMeshComp(UStaticMeshComponent* MeshComp)
{
    MeshComps.Add(MeshComp);
}

void URenderingManager::UnregisterMeshComp(UStaticMeshComponent* MeshComp)
{
    MeshComps.Remove(MeshComp);
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
        SkeletalMeshInstances.resize(static_cast<std::vector<FSkeletalMeshInstance, std::allocator<FSkeletalMeshInstance>>::size_type>(Index) + 1);
    }

    std::shared_ptr<FSkeletalMeshInstance>& NewInstance = SkeletalMeshInstances[Index];
    NewInstance->SelfIndex = Index;
    NewInstance->IsAlive = true;
    NewInstance->IsSlotOccupied = true;
    NewInstance->SkeletonHandle = SkeletonHandle;
    NewInstance->SkeletalMeshHandle = SkeletalMeshHandle;
    NewInstance->OwningAnimInstance = OwningAnimInstance;
    NewInstance->Graph = UAnimGraphRegistry::Get().CreateGraph(AnimGraphHandle);
    NewInstance->CurrentSnapshot = nullptr;
    NewInstance->RenderingPalette.assign(URenderingStatics::PaletteBufferCount, TArray<FMatrix4x4>(URenderingStatics::MaxBoneMatrixCount, FMatrix4x4::Identity));
    NewInstance->ReadIndex = 0;
    NewInstance->IsHasPendingSwap = false;

    return FInstanceHandle{ Index, NewInstance->Generation };
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
