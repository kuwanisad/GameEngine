#include "../../Public/Asset/SkeletalMesh.h"
#include "../../Public/Rendering/Texture.h"
#include "../../Public/Rendering/VertexContainer.h"
#include "../../Public/Rendering/RenderingStatics.h"
#include "../../Public/Asset/FbxImporter.h"

#include <SDL_log.h>

#include <cstring>
#include <algorithm>

USkeletalMesh::USkeletalMesh()
	: Vertexes(nullptr), 
	Skeleton(nullptr), 
	DefaultTexture(nullptr)
{
}

USkeletalMesh::~USkeletalMesh()
{
}

void USkeletalMesh::Unload()
{
	delete Vertexes;
	Vertexes = nullptr;

	//UAssetManager에서 해제를 만들어야 함
	// delete Skeleton;
	//Skeleton = nullptr;

	if (DefaultTexture)
	{
		DefaultTexture->Unload();
		//UAssetManager에서 해제를 만들어야 함
		//delete DefaultTexture;
		DefaultTexture = nullptr;
	}
}
