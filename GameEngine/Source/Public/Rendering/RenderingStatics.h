#pragma once
#include "../Core/CoreMinimal.h"
#include "../Core/Math.h"

class UReferenceSkeleton;

// 최대 UV 채널 수
constexpr int32 MAX_TEXCOORDS = 4;

// UE의 FSoftSkinVertex 최소화 버전으로 최종 GPU 렌더링용 정점 하나임
struct FSoftSkinVertex
{
	FVector3 Position;
	FVector3 TangentX; //이 프로젝트에서 안 씀
	FVector3 TangentY; //이 프로젝트에서 안 씀
	FVector3 TangentZ; //노멀
	FVector2 UVs[MAX_TEXCOORDS]; //인덱스 0만 사용
	uint8 InfluenceBones[4];
	uint8 InfluenceWeights[4];

	FSoftSkinVertex()
	{
		for (int32 i = 0; i < 4; ++i)
		{
			InfluenceBones[i] = 0;
			InfluenceWeights[i] = 0;
		}
	}
};

// UE의 FSkelMeshSection
struct FSkelMeshSection
{
	int32 TriangleCount = 0;
	TArray<FSoftSkinVertex> SoftVertices;
	TArray<uint32> Indices;
};

//특정 버텍스에 영향을 주는 본의 영향력
struct FBoneInfluence
{
	float Weight;
	int32 VertexIndex;
	int32 BoneIndex;

	FBoneInfluence()
		: Weight(0.0f)
		, VertexIndex(INDEX_NONE)
		, BoneIndex(INDEX_NONE)
	{
	}

	FBoneInfluence(float InWeight, int32 InVertexIndex, int32 InBoneIndex)
		: Weight(InWeight)
		, VertexIndex(InVertexIndex)
		, BoneIndex(InBoneIndex)
	{
	}
};

struct FVertex
{
	int32 VertexIndex; //Points 배열 인덱스
	FVector2 UVs[MAX_TEXCOORDS]; //0만 사용
	FVector3 Color;
	int32 MatIndex; //머티리얼 슬롯 인덱스. 지금은 0

	FVertex()
		: VertexIndex(INDEX_NONE)
		, Color(1.0f, 1.0f, 1.0f)
		, MatIndex(0)
	{
	}
};

//삼각형 폴리곤
struct FTriangle
{
	int32 WedgeIndex[3];
	int32 MatIndex;
	FVector3 TangentX[3];       // 코너별 탄젠트
	FVector3 TangentY[3];       // 코너별 바이탄젠트
	FVector3 TangentZ[3];       // 코너별 노멀 (실사용: 최종 노멀 계산에 씀)

	FTriangle()
		: MatIndex(0)
	{
		for (int32 i = 0; i < 3; ++i)
		{
			WedgeIndex[i] = INDEX_NONE;
		}
	}
};

// 정점 하나에 대한 정규화, 4개로 압축된 최종 스킨 웨이트
struct FPackedSkinWeights
{
	uint8 InfluenceBones[4];
	uint8 InfluenceWeights[4];

	FPackedSkinWeights()
	{
		for (int32 i = 0; i < 4; ++i)
		{
			InfluenceBones[i] = 0;
			InfluenceWeights[i] = 0;
		}
	}
};

// UE의 FSkeletalMeshLODModel를 필요한 것만 남김
struct FSkeletalMeshLODModel
{
	TArray<FSkelMeshSection> Sections;
};

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

	static FString Shader_uSpecPower;

	//VertexContainer의 PosNormSkinTex 오프셋과 일치해야 하는 상수

	static int32 VertexPositionOffset; //float 3
	static int32 VertexNormalOffset; //float 3
	static int32 BoneIndexOffset; //byte 4
	static int32 BoneWeightOffset; //byte 4
	static int32 TextureUVOffset; //float 2
	static int32 TotalVertexOffset; //총 40 바이트

public:
	// 정점당 무제한으로 쌓인 Influence 리스트를, 정점당 상위 4개로 잘라 정규화함
	static void NormalizeAndPackInfluences(const TArray<FBoneInfluence>& Influences, int32 VertexCount, TArray<FPackedSkinWeights>& OutPackedWeights);

	// UE의 FMeshUtilities::BuildSkeletalMesh로 Faces, Wedges, Points, Influences를 섹션 1개 최종 정점, 인덱스 버퍼로 조립함
	static bool BuildSkeletalMesh(FSkeletalMeshLODModel& OutLODModel, const UReferenceSkeleton* Skeleton, const TArray<FBoneInfluence>& Influences, const TArray<FVertex>& Wedges, const TArray<FTriangle>& Faces, const TArray<FVector3>& Points);
};