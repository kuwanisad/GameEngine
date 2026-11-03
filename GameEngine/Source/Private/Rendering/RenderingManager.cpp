#include "../../Public/Rendering/RenderingManager.h"
#include "../../Public/Rendering/Shader.h"
#include "../../Public/Rendering/RenderingStatics.h"
#include "../../Public/rendering/Renderer.h"

void URenderingManager::RenderFrame(FShaderPipeline* Pipeline)
{
    Pipeline->SetActive();
    Pipeline->SetMatrices(URenderingStatics::Shader_uViewProj.c_str(), GetRenderer()->GetViewProjMatrix());

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

void URenderingManager::AddSprite(USpriteComponent* SpriteComp)
{
}

void URenderingManager::RemoveSprite(USpriteComponent* SpriteComp)
{
}

void URenderingManager::AddMeshComp(UStaticMeshComponent* MeshComp)
{
}

void URenderingManager::RemoveMeshComp(UStaticMeshComponent* MeshComp)
{
}
