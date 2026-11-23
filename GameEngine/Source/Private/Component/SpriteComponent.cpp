#include "../../Public/Component/SpriteComponent.h"
#include "../../Public/Rendering/RenderingManager.h"
#include "../../Public/Rendering/Renderer.h"
#include "../../Public/Rendering/Texture.h"
#include "../../Public/Rendering/RenderingStatics.h"
#include "../../Public/Rendering/Shader.h"
#include "../../Public/Core/Engine.h"
#include "../../Public/Actor/Actor.h"

USpriteComponent::USpriteComponent(AActor* InOwner, int InRenderingOrder)
	: UActorComponent(InOwner),
	Sprite(nullptr)
	,Width(0)
	,Height(0)
{
}

USpriteComponent::~USpriteComponent()
{
}

void USpriteComponent::Init()
{
	// [체이닝 필수 - BoxComponent/StaticMeshComponent와 동일한 버그] 베이스를 안 부르면
	// Owner->AddComponent(this)가 빠져서 AActor 소멸자가 이 컴포넌트를 절대 못 지움 -
	// 액터가 죽을 때 OnDestroy()(RenderingManager::UnregisterSpriteComp)가 안 불려서
	// RenderingManager에 댕글링 포인터가 남게 됨.
	UActorComponent::Init();

	UEngine::GetEngine()->GetRenderingManager()->RegisterSpriteComp(this);
}

void USpriteComponent::OnDestroy()
{
	UEngine::GetEngine()->GetRenderingManager()->UnregisterSpriteComp(this);
}

void USpriteComponent::Draw(FShaderPipeline* Pipeline)
{
	if (Sprite == nullptr)
	{
		return;
	}

	FMatrix4x4 scale = FMatrix4x4::TransformScaleToMatrix(FVector3(Width, Height, 1.f));

	FMatrix4x4 world = scale * Owner->GetActorTransform();

	Pipeline->SetMatrix(URenderingStatics::Shader_uWorldTransform.c_str(), world);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

void USpriteComponent::SetTexture(UTexture* InTexture)
{
	Sprite = InTexture;
	Width = InTexture->GetWidth();
	Height = InTexture->GetHeight();
}
