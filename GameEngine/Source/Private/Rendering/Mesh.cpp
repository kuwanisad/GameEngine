#include "../../Public/Rendering/Mesh.h"
#include "../../Public/Rendering/VertexContainer.h"

UMesh::UMesh() :
	VertexContainer(nullptr),
	Specular(0)
{
}

UMesh::~UMesh()
{
}

void UMesh::SetName(const FString& InName)
{
	Name = InName;
}

void UMesh::Unload()
{
	if (VertexContainer != nullptr)
	{
		delete VertexContainer;
		VertexContainer = nullptr;
	}
}

UTexture* UMesh::GetTexture(const int32 TextureIndex) const
{
	if (TextureIndex < 0 || Textures.size() <= TextureIndex)
	{
		return nullptr;
	}

	return Textures[TextureIndex];
}

void UMesh::SetVertexContainer(FVertexContainer* InVertexContainer)
{
	VertexContainer = InVertexContainer;
}
