#include "../../Public/Component/StaticMeshComponent.h"
#include "../../Public/Actor/Actor.h"
#include "../../Public/Rendering/Renderer.h"
#include "../../Public/Rendering/RenderingManager.h"
#include "../../Public/Rendering/Mesh.h"
#include "../../Public/Rendering/Shader.h"
#include "../../Public/Rendering/RenderingStatics.h"
#include "../../Public/Rendering/Texture.h"
#include "../../Public/Rendering/VertexContainer.h"
#include "../../Public/Core/Engine.h"

UStaticMeshComponent::UStaticMeshComponent(AActor* _Owner) : UPrimitiveComponent(_Owner),
	Mesh(nullptr),
	TextureIndex(0),
	Visible(true)
{
}

UStaticMeshComponent::~UStaticMeshComponent()
{
}

void UStaticMeshComponent::Init()
{
	// [체이닝 필수 - BoxComponent와 동일한 버그] 베이스를 안 부르면 Owner->AddComponent(this)와
	// 틱 등록이 둘 다 빠짐 - 특히 Owner->Components에 안 들어가면 AActor::~AActor()의 정리
	// 루프가 이 컴포넌트를 절대 delete 못 해서, 액터가 죽을 때 RenderingManager::UnregisterMeshComp
	// (OnDestroy 안에 있음)가 안 불리고, 그 결과 RenderingManager가 이미 delete된 컴포넌트를
	// 계속 그리려다 댕글링 포인터 역참조로 죽을 수 있음(지연된 크래시 - 액터가 죽는 순간에야 터짐).
	UActorComponent::Init();

	UEngine::GetEngine()->GetRenderingManager()->RegisterMeshComp(this);
}

void UStaticMeshComponent::OnDestroy()
{
	UPrimitiveComponent::OnDestroy();
	UEngine::GetEngine()->GetRenderingManager()->UnregisterMeshComp(this);

}

void UStaticMeshComponent::Draw(FShaderPipeline* Pipeline)
{
	if (Mesh != nullptr)
	{
		UTexture* tex = Mesh->GetTexture(TextureIndex);
		FVertexContainer* vc = Mesh->GetVertexContainer();

		if (tex == nullptr || vc == nullptr)
		{
			return;
		}

		Pipeline->SetMatrix(URenderingStatics::Shader_uWorldTransform.c_str(), GetComponentTransform());
		Pipeline->SetFloat(URenderingStatics::Shader_uSpecPower.c_str(), Mesh->GetSpecular());

		tex->SetActive();
		vc->SetActive();

		glDrawElements(GL_TRIANGLES, vc->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
	}
}
