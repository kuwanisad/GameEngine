#include "../../Public/Rendering/RenderingStatics.h"

int URenderingStatics::VertexLerpBoneCount = 4;
engine_size_t URenderingStatics::VertexDefaultSize = sizeof(float) * 8;

engine_size_t URenderingStatics::VertexPosSize = sizeof(float) * 3;
engine_size_t URenderingStatics::VertexNormSize = sizeof(float) * 3;
engine_size_t URenderingStatics::VertexUVSize = sizeof(float) * 2;

FString URenderingStatics::WhiteTextureName = "WhiteTexture";

FString URenderingStatics::Shader_uViewProj = "uViewProj";
FString URenderingStatics::Shader_uWorldTransform = "uWorldTransform";
