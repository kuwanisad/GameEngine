#pragma once
#include "../Core/CoreMinimal.h"
#include "../../Public/Rendering/RenderingStatics.h"
#include "Skeleton.h"

//Claude를 사용해 구현함

class UAnimSequence;

// FBX SDK 전방 선언 (fbxsdk.h는 .cpp에서만 include)
namespace fbxsdk
{
	class FbxManager;
	class FbxImporter;
	class FbxScene;
	class FbxNode;
	class FbxCluster;
}

struct FSkeletalMeshImportData
{
	TArray<FVector3> Points;
	TArray<FVertex> Wedges;
	TArray<FTriangle> Faces;
	TArray<FBoneInfluence> Influences;

	// TODO(더미, 미사용): UE의 LODPointToRawMap 대응. 우리는 정점 중복 제거/재배치를
	// 안 하므로 지금은 항상 비어있거나 항등 매핑(자기 자신)일 것으로 취급한다.
	TArray<int32> PointToOriginalMap;

	int32 MaxMaterialIndex = 0;
};

// UE의 UnFbx::FFbxImporter 대응.
// - 내부에 FBX SDK의 FbxManager/FbxImporter(소문자 fbximporter.h 쪽 클래스)를 소유한다.
// - 지금 단계 목표: FBX 씬을 열고, 스켈레톤(본 계층)을 FReferenceSkeleton으로 뽑아내는 것까지 확인.
class UFbxImporter
{
public:
	static UFbxImporter* Get();

	// UE의 ImportFromFile과 동일한 이름/역할
	bool ImportFromFile(const FString& Filename);
	void ReleaseScene();

	// 씬 안에서 스킨(Skin Deformer)을 가진 첫 번째 메시를 찾아
	// 본 계층을 OutSkeleton에 채운다. 성공 시 true.
	// (UE의 ImportBones를 단순화한 버전)
	bool ImportSkeleton(UReferenceSkeleton& OutSkeleton);

	// 씬에서 스킨 있는 첫 메시를 찾아, 정점별 (본, 가중치) 원시 리스트를 뽑는다.
	// Skeleton은 미리 ImportSkeleton()으로 채워둔 것을 넘겨줘야 한다 (본 이름 -> 인덱스 매핑에 사용).
	// OutVertexCount에는 이 메시의 정점(Control Point) 총 개수가 채워진다.
	bool ImportSkinWeights(const UReferenceSkeleton& Skeleton, TArray<FBoneInfluence>& OutInfluences, int32& OutVertexCount) const;

	// (스킨 웨이트 정규화는 FBX와 무관한 로직이라 MeshUtilities::NormalizeAndPackInfluences로 이전됨)

	// 씬에서 스킨 있는 첫 메시를 찾아, 지오메트리(위치/UV/노멀/삼각형)를 뽑는다.
	// 먼저 전체 폴리곤을 삼각형으로 쪼갠 뒤(UE의 FbxGeometryConverter::Triangulate와 동일),
	// Points(위치) / Wedges(폴리곤 코너별 UV) / Faces(삼각형, Wedge 인덱스 3개)를 채운다.
	bool ImportGeometry(FSkeletalMeshImportData& OutImportData) const;

	// [애니메이션 전용 파일 대응] 캐릭터 모델과 별개의 FBX 파일(예: UE 마네킹의 개별
	// 클립, MM_Idle 등)에서 애니메이션 커브를 뽑아 UAnimSequence를 채움. 이런 파일엔
	// 스킨 메시가 없는 게 일반적이라(순수 본 계층 + 커브), ImportSkeleton()처럼
	// 스킨 클러스터를 거치지 않고 FbxNodeAttribute::eSkeleton 타입 노드를 씬 전체에서
	// 직접 찾음. 본 매칭은 이름으로 함(파일이 다르니 노드 트리/인덱스가 대응 안 됨) -
	// TargetSkeleton은 이미 캐릭터 FBX로 ImportSkeleton()해둔 것을 넘겨야 함.
	// 매칭 안 되는 본(이 애니메이션 파일에 없는 본)은 트랙이 빈 채로 남고, 재생 시
	// FAnimNode_SequencePlayer가 그 본만 자동으로 바인드포즈로 폴백함.
	bool ImportAnimation(const UReferenceSkeleton& TargetSkeleton, UAnimSequence& OutSequence, float SampleRate = 30.0f) const;

	fbxsdk::FbxScene* GetScene() const { return Scene; }

private:
	UFbxImporter();
	~UFbxImporter();

	// ---- 본 계층 빌드 (UE FbxSkeletalMeshImport.cpp 대응) ----
	bool IsUnrealBone(fbxsdk::FbxNode* Link) const;
	void RecursiveBuildSkeleton(fbxsdk::FbxNode* Link, TArray<fbxsdk::FbxNode*>& OutSortedLinks) const;
	void BuildSkeletonSystem(TArray<fbxsdk::FbxCluster*>& ClusterArray, TArray<fbxsdk::FbxNode*>& OutSortedLinks) const;
	fbxsdk::FbxNode* FindFirstSkinnedMeshNode(fbxsdk::FbxNode* Root) const;

	// [ImportAnimation 전용] 스킨 디포머 없이, 씬 전체에서 FbxNodeAttribute::eSkeleton
	// 타입 노드만 직접 재귀 탐색함 - 애니메이션 전용 파일엔 스킨이 아예 없을 수 있어서.
	void RecursiveFindSkeletonNodes(fbxsdk::FbxNode* Node, TArray<fbxsdk::FbxNode*>& OutNodes) const;

	fbxsdk::FbxManager* SdkManager;
	fbxsdk::FbxImporter* Importer;
	fbxsdk::FbxScene* Scene;

	static UFbxImporter* Instance;
};
