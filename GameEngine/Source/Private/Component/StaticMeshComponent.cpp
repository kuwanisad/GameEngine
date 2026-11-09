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
