#include "../../Public/Rendering/Mesh.h"

UMesh::UMesh() :
	VertexContainer(nullptr)
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

void UMesh::SetVertexContainer(FVertexContainer* InVertexContainer)
{
	VertexContainer = InVertexContainer;
}

float UMesh::GetSpecular()
{
	return 0.0f;
}
