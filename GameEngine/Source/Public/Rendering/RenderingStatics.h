#pragma once
#include "../Core/CoreMinimal.h"

class URenderingStatics
{
public:
	static int VertexLerpBoneCount;
	static engine_size_t VertexDefaultSize;
	static engine_size_t VertexPosSize;
	static engine_size_t VertexNormSize;
	static engine_size_t VertexUVSize;

	static FString WhiteTextureName;

	static FString Shader_uViewProj;
	static FString Shader_uWorldTransform;
};