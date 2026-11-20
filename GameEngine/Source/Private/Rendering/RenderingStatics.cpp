#include "../../Public/Rendering/RenderingStatics.h"
#include "../../Public/Asset/Skeleton.h"
#include "../../Public/Core/Math.h"

#include <algorithm>

#include <SDL_log.h>

int URenderingStatics::VertexLerpBoneCount = 4;
engine_size_t URenderingStatics::VertexDefaultSize = sizeof(float) * 8;

engine_size_t URenderingStatics::VertexPosSize = sizeof(float) * 3;
engine_size_t URenderingStatics::VertexNormSize = sizeof(float) * 3;
engine_size_t URenderingStatics::VertexUVSize = sizeof(float) * 2;

FString URenderingStatics::WhiteTextureName = "WhiteTexture";

FString URenderingStatics::Shader_uViewProj = "uViewProj";
FString URenderingStatics::Shader_uWorldTransform = "uWorldTransform";

FString URenderingStatics::Shader_uSpecPower = "uSpecPower";

FString URenderingStatics::Shader_Light_uCameraPos = "uCameraPos";
FString URenderingStatics::Shader_Light_uAmbientLight = "uAmbientLight";
FString URenderingStatics::Shader_Light_uDirLight_mDirection = "uDirLight.mDirection";
FString URenderingStatics::Shader_Light_uDirLight_mDiffuseColor = "uDirLight.mDiffuseColor";
FString URenderingStatics::Shader_Light_uDirLight_mSpecColor = "uDirLight.mSpecColor";

//VertexContainer의 PosNormSkinTex 참고
int32 URenderingStatics::VertexPositionOffset = 0;
int32 URenderingStatics::VertexNormalOffset = VertexPositionOffset + 12;
int32 URenderingStatics::BoneIndexOffset = VertexNormalOffset + 12;
int32 URenderingStatics::BoneWeightOffset = BoneIndexOffset + 4;
int32 URenderingStatics::TextureUVOffset = BoneWeightOffset + 4;
int32 URenderingStatics::TotalVertexOffset = TextureUVOffset + 8;

int32 URenderingStatics::PaletteBufferCount = 2;


float URenderingStatics::PerspectiveFOV_DegreeY = Math::ToRadians(70.0f);
float URenderingStatics::PerspectiveFOV_Near = 25.f;
float URenderingStatics::PerspectiveFOV_Far = 10000.f;

void URenderingStatics::NormalizeAndPackInfluences(const TArray<FBoneInfluence>& Influences, int32 VertexCount, TArray<FPackedSkinWeights>& OutPackedWeights)
{
	OutPackedWeights.clear();
	OutPackedWeights.resize(VertexCount);

	// 버텍스에 영향을 주는 본들 수집
	TArray<TArray<FBoneInfluence>> VertexArr(VertexCount);
	for (const FBoneInfluence& Influence : Influences)
	{
		if (Influence.VertexIndex >= 0 && Influence.VertexIndex < VertexCount)
		{
			VertexArr[Influence.VertexIndex].emplace_back(Influence);
		}
	}

	//디버깅용
	int32 ZeroInfluenceVertexCount = 0;

	for (int32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
	{
		TArray<FBoneInfluence>& Candidates = VertexArr[VertexIndex];

		//영향이 큰 순으로 상위 4개만 씀
		std::sort(Candidates.begin(), Candidates.end(),
			[](const FBoneInfluence& A, const FBoneInfluence& B) { return A.Weight > B.Weight; });
		int32 SelectedCandidates = Math::Min(static_cast<int32>(Candidates.size()), VertexLerpBoneCount);

		float WeightSum = 0.f;
		for (int32 i = 0; i < SelectedCandidates; ++i)
		{
			WeightSum += Candidates[i].Weight;
		}

		if (WeightSum <= 0.0f)
		{
			// 이 정점에 영향 주는 본이 하나도 없는 경우
			// 스킨 행렬이 전부 0이 되면 정점이 원점이 돼서 루트 본으로 적용
			OutPackedWeights[VertexIndex].InfluenceBones[0] = 0;
			OutPackedWeights[VertexIndex].InfluenceWeights[0] = 255;
			++ZeroInfluenceVertexCount;
			continue;
		}

		for (int32 i = 0; i < SelectedCandidates; ++i)
		{
			float Normalized = Candidates[i].Weight / WeightSum;
			OutPackedWeights[VertexIndex].InfluenceBones[i] = static_cast<uint8>(Candidates[i].BoneIndex);
			OutPackedWeights[VertexIndex].InfluenceWeights[i] = static_cast<uint8>(std::lround(Normalized * 255.f));
		}

		//개별값을 반올림하면 합이 255가 안 될 수 있어 오차를 처리해야 함 
		int32 WeightSum2 = 0;
		for (int32 i = 0; i < SelectedCandidates; ++i)
		{
			WeightSum2 += OutPackedWeights[VertexIndex].InfluenceWeights[i];
		}

		int32 RoundingDelta = 255 - WeightSum2;
		if (RoundingDelta != 0)
		{
			int32 AdjustedValue = static_cast<int32>(OutPackedWeights[VertexIndex].InfluenceWeights[0]) + RoundingDelta;
			AdjustedValue = std::max(0, std::min(255, AdjustedValue));
			OutPackedWeights[VertexIndex].InfluenceWeights[0] = static_cast<uint8>(AdjustedValue);
		}
	}

	if (ZeroInfluenceVertexCount > 0)
	{
		SDL_Log("URenderingStatics::NormalizeAndPackInfluences: %d vertices had no bone influence (defaulted to root bone)", ZeroInfluenceVertexCount);
	}
}

bool URenderingStatics::BuildSkeletalMesh(FSkeletalMeshLODModel& OutLODModel, const TArray<FBoneInfluence>& Influences, const TArray<FVertex>& Wedges, const TArray<FTriangle>& Faces, const TArray<FVector3>& Points)
{
	if (Points.empty() || Wedges.empty() || Faces.empty())
	{
		return false;
	}

	TArray<FPackedSkinWeights> PointSkinWeights;
	NormalizeAndPackInfluences(Influences, static_cast<int32>(Points.size()), PointSkinWeights);

	FSkelMeshSection Section;
	Section.SoftVertices.reserve(Wedges.size());
	Section.Indices.reserve(Faces.size() * 3);

	for (const FTriangle& Face : Faces)
	{
		for (int32 Corner = 0; Corner < 3; ++Corner)
		{
			int32 WedgeIndex = Face.WedgeIndex[Corner];
			const FVertex& Wedge = Wedges[WedgeIndex];

			FSoftSkinVertex Vertex;
			Vertex.Position = Points[Wedge.VertexIndex];
			Vertex.UVs[0] = Wedge.UVs[0];
			Vertex.TangentZ = Face.TangentZ[Corner];

			const FPackedSkinWeights& SkinWeights = PointSkinWeights[Wedge.VertexIndex];
			for (int32 i = 0; i < VertexLerpBoneCount; ++i)
			{
				Vertex.InfluenceBones[i] = SkinWeights.InfluenceBones[i];
				Vertex.InfluenceWeights[i] = SkinWeights.InfluenceWeights[i];
			}

			int32 NewIndex = static_cast<int32>(Section.SoftVertices.size());
			Section.SoftVertices.emplace_back(Vertex);
			Section.Indices.emplace_back(static_cast<uint32>(NewIndex));
		}
	}

	Section.TriangleCount = static_cast<int32>(Section.Indices.size() / 3);

	OutLODModel.Sections.emplace_back(Section);

	return true;
}
