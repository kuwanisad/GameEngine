#include "../../Public/Rendering/RenderingManager.h"
#include "../../Public/Rendering/Shader.h"
#include "../../Public/Rendering/RenderingStatics.h"
#include "../../Public/rendering/Renderer.h"

void URenderingManager::RenderFrame(FShaderPipeline* Pipeline)
{
    Pipeline->SetActive();
    Pipeline->SetMatrix(URenderingStatics::Shader_uViewProj.c_str(), GetRenderer()->GetViewProjMatrix());

}

bool URenderingManager::IsHandleValid(const FInstanceHandle& Handle)
{
    return false;
}

FSkeletalMeshInstance* URenderingManager::Resolve(const FInstanceHandle& Handle)
{
    return nullptr;
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
