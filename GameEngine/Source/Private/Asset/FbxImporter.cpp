#include "../../Public/Asset/FbxImporter.h"
#include "../../Public/Animation/AnimSequence.h"

#include <algorithm>
#include <cmath>

#include <fbxsdk.h>
#include <SDL_log.h>

using namespace fbxsdk;

UFbxImporter* UFbxImporter::Instance = nullptr;

UFbxImporter* UFbxImporter::Get()
{
	if (Instance == nullptr)
	{
		Instance = new UFbxImporter();
	}
	return Instance;
}

UFbxImporter::UFbxImporter()
	: SdkManager(nullptr)
	, Importer(nullptr)
	, Scene(nullptr)
{
	SdkManager = FbxManager::Create();

	FbxIOSettings* IOSettings = FbxIOSettings::Create(SdkManager, IOSROOT);
	SdkManager->SetIOSettings(IOSettings);

	Importer = FbxImporter::Create(SdkManager, "");
}

UFbxImporter::~UFbxImporter()
{
	ReleaseScene();

	if (Importer)
	{
		Importer->Destroy();
		Importer = nullptr;
	}
	if (SdkManager)
	{
		SdkManager->Destroy();
		SdkManager = nullptr;
	}
}

bool UFbxImporter::ImportFromFile(const FString& Filename)
{
	ReleaseScene();

	bool bImportStatus = Importer->Initialize(Filename.c_str(), -1, SdkManager->GetIOSettings());
	if (!bImportStatus)
	{
		SDL_Log("FFbxImporter: Initialize failed for %s (%s)", Filename.c_str(),
			Importer->GetStatus().GetErrorString());
		return false;
	}

	Scene = FbxScene::Create(SdkManager, "ImportScene");
	bImportStatus = Importer->Import(Scene);
	if (!bImportStatus)
	{
		SDL_Log("FFbxImporter: Import failed for %s (%s)", Filename.c_str(),
			Importer->GetStatus().GetErrorString());
		Scene->Destroy();
		Scene = nullptr;
		return false;
	}

	return true;
}

void UFbxImporter::ReleaseScene()
{
	if (Scene)
	{
		Scene->Destroy();
		Scene = nullptr;
	}
}

// 노드/자손을 순회하며 스킨 디포머를 가진 첫 메시 노드를 찾는다 (테스트용 단순 탐색)
FbxNode* UFbxImporter::FindFirstSkinnedMeshNode(FbxNode* Root) const
{
	if (Root == nullptr)
	{
		return nullptr;
	}

	FbxNodeAttribute* Attr = Root->GetNodeAttribute();
	if (Attr && Attr->GetAttributeType() == FbxNodeAttribute::eMesh)
	{
		FbxMesh* Mesh = static_cast<FbxMesh*>(Attr);
		if (Mesh->GetDeformerCount(FbxDeformer::eSkin) > 0)
		{
			return Root;
		}
	}

	for (int32 ChildIndex = 0; ChildIndex < Root->GetChildCount(); ++ChildIndex)
	{
		FbxNode* Found = FindFirstSkinnedMeshNode(Root->GetChild(ChildIndex));
		if (Found != nullptr)
		{
			return Found;
		}
	}
	return nullptr;
}

// UE FbxSkeletalMeshImport.cpp::IsUnrealBone과 동일한 판단 기준
bool UFbxImporter::IsUnrealBone(FbxNode* Link) const
{
	FbxNodeAttribute* Attr = Link->GetNodeAttribute();
	if (Attr)
	{
		FbxNodeAttribute::EType AttrType = Attr->GetAttributeType();
		if (AttrType == FbxNodeAttribute::eSkeleton ||
			AttrType == FbxNodeAttribute::eMesh ||
			AttrType == FbxNodeAttribute::eNull)
		{
			return true;
		}
	}
	return false;
}

void UFbxImporter::RecursiveBuildSkeleton(FbxNode* Link, TArray<FbxNode*>& OutSortedLinks) const
{
	if (IsUnrealBone(Link))
	{
		OutSortedLinks.emplace_back(Link);
		for (int32 ChildIndex = 0; ChildIndex < Link->GetChildCount(); ++ChildIndex)
		{
			RecursiveBuildSkeleton(Link->GetChild(ChildIndex), OutSortedLinks);
		}
	}
}

void UFbxImporter::BuildSkeletonSystem(TArray<FbxCluster*>& ClusterArray, TArray<FbxNode*>& OutSortedLinks) const
{
	TSet<FbxNode*> RootLinks;
	for (FbxCluster* Cluster : ClusterArray)
	{
		FbxNode* Link = Cluster->GetLink();
		if (Link == nullptr)
		{
			continue;
		}

		// 링크의 최상위 조상(스켈레톤 루트)까지 거슬러 올라감
		FbxNode* Root = Link;
		while (Root->GetParent() != nullptr && IsUnrealBone(Root->GetParent()))
		{
			Root = Root->GetParent();
		}

		// TSet이라 이미 있으면 insert가 그냥 무시됨 (중복 체크 루프 불필요)
		RootLinks.insert(Root);
	}

	for (FbxNode* Root : RootLinks)
	{
		RecursiveBuildSkeleton(Root, OutSortedLinks);
	}
}

// FbxAMatrix -> FTransform (Translation/Rotation/Scale) 변환
static FTransform FbxMatrixToFTransform(const FbxAMatrix& Matrix)
{
	FbxVector4 T = Matrix.GetT();
	FbxQuaternion Q = Matrix.GetQ();
	FbxVector4 S = Matrix.GetS();

	FTransform Out;
	Out.Translation = FVector3(static_cast<float>(T[0]), static_cast<float>(T[1]), static_cast<float>(T[2]));
	Out.Rotation = FQuaternion(static_cast<float>(Q[0]), static_cast<float>(Q[1]), static_cast<float>(Q[2]), static_cast<float>(Q[3]));
	Out.Scale3D = FVector3(static_cast<float>(S[0]), static_cast<float>(S[1]), static_cast<float>(S[2]));
	return Out;
}

bool UFbxImporter::ImportSkeleton(UReferenceSkeleton& OutSkeleton)
{
	OutSkeleton.Empty();

	if (Scene == nullptr)
	{
		SDL_Log("FFbxImporter::ImportSkeleton - no scene loaded");
		return false;
	}

	FbxNode* MeshNode = FindFirstSkinnedMeshNode(Scene->GetRootNode());
	if (MeshNode == nullptr)
	{
		SDL_Log("FFbxImporter::ImportSkeleton - no skinned mesh found in scene");
		return false;
	}

	FbxMesh* Mesh = static_cast<FbxMesh*>(MeshNode->GetNodeAttribute());
	FbxSkin* Skin = static_cast<FbxSkin*>(Mesh->GetDeformer(0, FbxDeformer::eSkin));

	TArray<FbxCluster*> ClusterArray;
	for (int32 ClusterIndex = 0; ClusterIndex < Skin->GetClusterCount(); ++ClusterIndex)
	{
		ClusterArray.emplace_back(Skin->GetCluster(ClusterIndex));
	}

	TArray<FbxNode*> SortedLinks;
	BuildSkeletonSystem(ClusterArray, SortedLinks);

	if (SortedLinks.empty())
	{
		SDL_Log("FFbxImporter::ImportSkeleton - no bones found");
		return false;
	}

	// 매번 배열을 순회하며 찾는 대신, 미리 한 번씩만 인덱싱해서 O(1) 조회로 바꾼다.
	TMap<FbxNode*, int32> LinkToSortedIndex;
	for (int32 i = 0; i < static_cast<int32>(SortedLinks.size()); ++i)
	{
		LinkToSortedIndex[SortedLinks[i]] = i;
	}

	TMap<FbxNode*, FbxCluster*> LinkToCluster;
	for (FbxCluster* Cluster : ClusterArray)
	{
		if (FbxNode* Link = Cluster->GetLink())
		{
			LinkToCluster[Link] = Cluster;
		}
	}

	TArray<FbxAMatrix> GlobalsPerLink(SortedLinks.size());

	for (int32 LinkIndex = 0; LinkIndex < static_cast<int32>(SortedLinks.size()); ++LinkIndex)
	{
		FbxNode* Link = SortedLinks[LinkIndex];

		// 부모 인덱스 찾기: TMap 조회 한 번 (기존엔 앞쪽 원소를 매번 선형 탐색했음)
		int32 ParentIndex = INDEX_NONE;
		auto ParentIt = LinkToSortedIndex.find(Link->GetParent());
		if (ParentIt != LinkToSortedIndex.end())
		{
			ParentIndex = ParentIt->second;
		}

		// 글로벌(바인드포즈) 트랜스폼: 이 링크를 참조하는 클러스터의 TransformLinkMatrix 우선,
		// 없으면 씬에 평가된 현재 트랜스폼으로 폴백
		auto ClusterIt = LinkToCluster.find(Link);
		if (ClusterIt != LinkToCluster.end())
		{
			ClusterIt->second->GetTransformLinkMatrix(GlobalsPerLink[LinkIndex]);
		}
		else
		{
			GlobalsPerLink[LinkIndex] = Link->EvaluateGlobalTransform();
		}

		FbxAMatrix LocalMatrix;
		if (ParentIndex != INDEX_NONE)
		{
			LocalMatrix = GlobalsPerLink[ParentIndex].Inverse() * GlobalsPerLink[LinkIndex];
		}
		else
		{
			LocalMatrix = GlobalsPerLink[LinkIndex];
		}

		FMeshBoneInfo BoneInfo(Link->GetName(), ParentIndex);
		OutSkeleton.Add(BoneInfo, FbxMatrixToFTransform(LocalMatrix));
	}

	SDL_Log("FFbxImporter::ImportSkeleton - imported %d bones from '%s'",
		OutSkeleton.GetBoneCount(), MeshNode->GetName());

	return true;
}

bool UFbxImporter::ImportSkinWeights(const UReferenceSkeleton& Skeleton, TArray<FBoneInfluence>& OutInfluences, int32& OutVertexCount) const
{
	OutInfluences.clear();
	OutVertexCount = 0;

	if (Scene == nullptr)
	{
		SDL_Log("FFbxImporter::ImportSkinWeights - no scene loaded");
		return false;
	}

	FbxNode* MeshNode = FindFirstSkinnedMeshNode(Scene->GetRootNode());
	if (MeshNode == nullptr)
	{
		SDL_Log("FFbxImporter::ImportSkinWeights - no skinned mesh found in scene");
		return false;
	}

	FbxMesh* Mesh = static_cast<FbxMesh*>(MeshNode->GetNodeAttribute());
	FbxSkin* Skin = static_cast<FbxSkin*>(Mesh->GetDeformer(0, FbxDeformer::eSkin));

	OutVertexCount = Mesh->GetControlPointsCount();

	int32 SkippedClusterCount = 0;

	for (int32 ClusterIndex = 0; ClusterIndex < Skin->GetClusterCount(); ++ClusterIndex)
	{
		FbxCluster* Cluster = Skin->GetCluster(ClusterIndex);
		FbxNode* Link = Cluster->GetLink();
		if (Link == nullptr)
		{
			continue;
		}

		// ImportSkeleton()이 이미 만들어둔 이름->인덱스 매핑을 그대로 재사용.
		// (여기서 다시 씬 트리를 훑지 않고, 본 이름으로 바로 조회)
		int32 BoneIndex = Skeleton.FindBoneIndex(Link->GetName());
		if (BoneIndex == INDEX_NONE)
		{
			// 스킨에는 있는데 ImportSkeleton()이 뽑은 본 목록엔 없는 경우
			// (이론상 발생하면 안 되지만, 방어적으로 스킵하고 로그만 남김)
			SDL_Log("FFbxImporter::ImportSkinWeights - bone '%s' not found in skeleton, skipping cluster", Link->GetName());
			++SkippedClusterCount;
			continue;
		}

		int32 InfluenceCount = Cluster->GetControlPointIndicesCount();
		int32* Indices = Cluster->GetControlPointIndices();
		double* Weights = Cluster->GetControlPointWeights();

		for (int32 i = 0; i < InfluenceCount; ++i)
		{
			OutInfluences.emplace_back(static_cast<float>(Weights[i]), Indices[i], BoneIndex);
		}
	}

	SDL_Log("FFbxImporter::ImportSkinWeights - %d raw influences for %d vertices (skipped %d clusters)",
		static_cast<int32>(OutInfluences.size()), OutVertexCount, SkippedClusterCount);

	return true;
}



bool UFbxImporter::ImportGeometry(FSkeletalMeshImportData& OutImportData) const
{
	OutImportData = FSkeletalMeshImportData();

	if (Scene == nullptr)
	{
		SDL_Log("FFbxImporter::ImportGeometry - no scene loaded");
		return false;
	}

	// 폴리곤이 삼각형이 아닐 수 있으므로(quad 등), 읽기 전에 씬 전체를 먼저 삼각형으로 쪼갠다.
	// (UE FbxSkeletalMeshImport.cpp가 GeometryConverter->Triangulate()를 미리 호출하는 것과 동일)
	FbxGeometryConverter GeometryConverter(SdkManager);
	GeometryConverter.Triangulate(Scene, /*pReplace*/ true);

	FbxNode* MeshNode = FindFirstSkinnedMeshNode(Scene->GetRootNode());
	if (MeshNode == nullptr)
	{
		SDL_Log("FFbxImporter::ImportGeometry - no skinned mesh found in scene");
		return false;
	}

	FbxMesh* Mesh = static_cast<FbxMesh*>(MeshNode->GetNodeAttribute());

	// ---- Points: Control Point(정점 위치)를 그대로 옮겨 담는다 ----
	int32 ControlPointCount = Mesh->GetControlPointsCount();
	FbxVector4* ControlPoints = Mesh->GetControlPoints();

	OutImportData.Points.resize(ControlPointCount);
	for (int32 PointIndex = 0; PointIndex < ControlPointCount; ++PointIndex)
	{
		const FbxVector4& P = ControlPoints[PointIndex];
		OutImportData.Points[PointIndex] = Vector3(
			static_cast<float>(P[0]), static_cast<float>(P[1]), static_cast<float>(P[2]));
	}

	// UV 셋 이름 확인 (없으면 UV 없이 진행)
	FbxStringList UVSetNameList;
	Mesh->GetUVSetNames(UVSetNameList);
	const char* UVSetName = (UVSetNameList.GetCount() > 0) ? UVSetNameList.GetStringAt(0) : nullptr;

	// ---- Faces + Wedges: 삼각형마다 코너 3개씩 순회하며 Wedge를 새로 만든다 ----
	int32 TriangleCount = Mesh->GetPolygonCount();
	OutImportData.Faces.resize(TriangleCount);

	for (int32 TriangleIndex = 0; TriangleIndex < TriangleCount; ++TriangleIndex)
	{
		FTriangle& Triangle = OutImportData.Faces[TriangleIndex];

		for (int32 Corner = 0; Corner < 3; ++Corner)
		{
			int32 ControlPointIndex = Mesh->GetPolygonVertex(TriangleIndex, Corner);

			FVertex Wedge;
			Wedge.VertexIndex = ControlPointIndex;

			if (UVSetName != nullptr)
			{
				FbxVector2 UV;
				bool bUnmapped = false;
				Mesh->GetPolygonVertexUV(TriangleIndex, Corner, UVSetName, UV, bUnmapped);
				Wedge.UVs[0] = Vector2(static_cast<float>(UV[0]), static_cast<float>(UV[1]));
			}

			int32 WedgeIndex = static_cast<int32>(OutImportData.Wedges.size());
			OutImportData.Wedges.emplace_back(Wedge);

			// 노멀은 코너(폴리곤 인스턴스)별로 다를 수 있어 Wedge가 아니라 Triangle 쪽에 저장
			// (UE의 Triangle.TangentZ[Corner]와 동일한 위치)
			// TODO(노멀매핑 도입 시): Triangle.TangentX/TangentY(탄젠트/바이탄젠트)는 여기서
			// 계산 안 하고 있음 - FVertex/FTriangle 필드 자리만 있고 값은 항상 기본값(0벡터).
			// 지금은 노멀매핑을 안 써서 문제 없지만, 나중에 필요해지면 삼각형의 두 변
			// (Position 차)과 UV 차이를 연립해서 구해야 함 (FbxMesh가 저장하고 있으면
			// GetPolygonVertexTangent/Binormal로 바로 가져올 수도 있음, 없으면 직접 계산).
			FbxVector4 Normal;
			Mesh->GetPolygonVertexNormal(TriangleIndex, Corner, Normal);
			Triangle.TangentZ[Corner] = FVector3(
				static_cast<float>(Normal[0]), static_cast<float>(Normal[1]), static_cast<float>(Normal[2]));

			Triangle.WedgeIndex[Corner] = WedgeIndex;
		}
	}

	SDL_Log("FFbxImporter::ImportGeometry - %d points, %d wedges, %d faces from '%s'",
		static_cast<int32>(OutImportData.Points.size()),
		static_cast<int32>(OutImportData.Wedges.size()),
		static_cast<int32>(OutImportData.Faces.size()),
		MeshNode->GetName());

	return true;
}

// 스킨 디포머를 거치지 않고, 씬 전체에서 FbxNodeAttribute::eSkeleton 타입 노드를
// 직접 재귀 탐색함 - 애니메이션 전용 파일(예: MM_Idle)엔 스킨 메시가 없을 수 있어서
// FindFirstSkinnedMeshNode 경로(ImportSkeleton/ImportSkinWeights가 쓰는)를 못 씀.
void UFbxImporter::RecursiveFindSkeletonNodes(FbxNode* Node, TArray<FbxNode*>& OutNodes) const
{
	if (Node == nullptr)
	{
		return;
	}

	FbxNodeAttribute* Attr = Node->GetNodeAttribute();
	if (Attr != nullptr && Attr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		OutNodes.emplace_back(Node);
	}

	for (int32 ChildIndex = 0; ChildIndex < Node->GetChildCount(); ++ChildIndex)
	{
		RecursiveFindSkeletonNodes(Node->GetChild(ChildIndex), OutNodes);
	}
}

bool UFbxImporter::ImportAnimation(const UReferenceSkeleton& TargetSkeleton, UAnimSequence& OutSequence, float SampleRate) const
{
	if (Scene == nullptr)
	{
		SDL_Log("FFbxImporter::ImportAnimation - no scene loaded");
		return false;
	}

	int32 AnimStackCount = Scene->GetSrcObjectCount<FbxAnimStack>();
	if (AnimStackCount == 0)
	{
		SDL_Log("FFbxImporter::ImportAnimation - no animation stack found in '%s'", "scene");
		return false;
	}

	// [범위 축소] 첫 번째 애니메이션 스택만 씀 - 한 FBX 파일에 클립 하나(예: MM_Idle)만
	// 있는 게 일반적인 배포 방식이라, 여러 스택(파일 하나에 클립 여럿)을 다루는 건
	// 지금 범위 밖 - 필요해지면 이름으로 골라 쓰는 오버로드를 추가하면 됨.
	FbxAnimStack* AnimStack = Scene->GetSrcObject<FbxAnimStack>(0);
	Scene->SetCurrentAnimationStack(AnimStack);

	FbxTimeSpan TimeSpan = AnimStack->GetLocalTimeSpan();
	double StartSeconds = TimeSpan.GetStart().GetSecondDouble();
	double StopSeconds = TimeSpan.GetStop().GetSecondDouble();
	double DurationSeconds = StopSeconds - StartSeconds;

	if (DurationSeconds <= 0.0)
	{
		SDL_Log("FFbxImporter::ImportAnimation - animation stack '%s' has zero or negative duration",
			AnimStack->GetName());
		return false;
	}

	// [스킨 없이 본 노드 직접 탐색]
	TArray<FbxNode*> SkeletonNodes;
	RecursiveFindSkeletonNodes(Scene->GetRootNode(), SkeletonNodes);

	if (SkeletonNodes.empty())
	{
		SDL_Log("FFbxImporter::ImportAnimation - no skeleton nodes found in scene");
		return false;
	}

	// [본 매칭 - 이름 기준, 먼저 한 번만] 이 애니메이션 파일의 본 노드 트리는
	// TargetSkeleton을 만든 캐릭터 FBX와 완전히 다른 씬이라, 노드 포인터/인덱스로는
	// 대응이 안 됨 - 이름으로만 매칭 가능. 매칭 안 되는 본(TargetSkeleton엔 없거나,
	// 이 파일에만 있는 보조 노드 등)의 트랙은 빈 채로 남고, 재생 시
	// FAnimNode_SequencePlayer가 그 본만 자동으로 바인드포즈로 폴백함(이미 확인된
	// 동작). 프레임 루프 안에서 매번 이름을 다시 찾지 않도록 미리 걸러서 목록으로
	// 만들어둠.
	struct FMatchedBone
	{
		FbxNode* Node;
		int32 BoneIndex;
	};
	TArray<FMatchedBone> MatchedBones;
	for (FbxNode* Node : SkeletonNodes)
	{
		int32 BoneIndex = TargetSkeleton.FindBoneIndex(Node->GetName());
		if (BoneIndex != INDEX_NONE)
		{
			MatchedBones.push_back({ Node, BoneIndex });
		}
	}

	OutSequence.BoneTracks.clear();
	OutSequence.BoneTracks.resize(TargetSkeleton.GetBoneCount());

	int32 FrameCount = static_cast<int32>(DurationSeconds * SampleRate) + 1;

	for (const FMatchedBone& MB : MatchedBones)
	{
		OutSequence.BoneTracks[MB.BoneIndex].Times.reserve(FrameCount);
		OutSequence.BoneTracks[MB.BoneIndex].Values.reserve(FrameCount);
	}

	// [수정 - 프레임이 바깥, 본이 안쪽] FbxAnimEvaluator는 시간 기준으로 내부
	// 캐시를 관리함(공식 클래스 레퍼런스 확인: 노드별/시간별 평가 상태를 캐싱).
	// "같은 시간에 대해 여러 본을 연달아 물어보는" 이 순서가, 예전처럼 "한 본에
	// 대해 여러 시간을 오가며 반복 조회"하던 것보다 훨씬 빠름 - Autodesk 공식
	// 샘플 코드(애니메이션 커브 추출 예제)가 쓰는 표준 순서와 일치시킴.
	for (int32 Frame = 0; Frame < FrameCount; ++Frame)
	{
		double SampleSeconds = StartSeconds + (static_cast<double>(Frame) / SampleRate);
		if (SampleSeconds > StopSeconds)
		{
			SampleSeconds = StopSeconds;
		}

		FbxTime SampleTime;
		SampleTime.SetSecondDouble(SampleSeconds);

		for (const FMatchedBone& MB : MatchedBones)
		{
			FbxAMatrix LocalMatrix = MB.Node->EvaluateLocalTransform(SampleTime);
			FTransform LocalTransform = FbxMatrixToFTransform(LocalMatrix);

			FAnimTrack& Track = OutSequence.BoneTracks[MB.BoneIndex];
			Track.Times.emplace_back(static_cast<float>(SampleSeconds - StartSeconds));
			Track.Values.emplace_back(LocalTransform.Rotation, LocalTransform.Translation);
		}
	}

	OutSequence.PlayLength = static_cast<float>(DurationSeconds);
	OutSequence.bLoop = true;   // 기본값 - 호출부가 필요하면 나중에 덮어씀

	SDL_Log("FFbxImporter::ImportAnimation - %d frames, matched %d/%d bones by name, duration %.2fs (stack '%s')",
		FrameCount, static_cast<int32>(MatchedBones.size()), static_cast<int32>(SkeletonNodes.size()), DurationSeconds, AnimStack->GetName());

	return !MatchedBones.empty();
}