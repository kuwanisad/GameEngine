#pragma once
#include "AnimNode_Base.h"
#include "../Asset/AssetHandle.h"

// UE의 FAnimNode_SequencePlayer에 대응(축소판). 그래프의 리프 노드 - 자식이 없고,
// 그냥 UAnimSequence 하나를 재생만 함.
//
// [수정 - 원시 포인터 대신 핸들] 예전엔 Sequence/SkeletonRef가 원시 포인터였는데,
// URenderingManager가 이미 RegisterAnimSequence/RegisterSkeleton + GetAnimSequence/
// GetSkeleton으로 핸들 기반 정적 에셋 저장소를 갖고 있었으면서도 이 노드만 그걸
// 안 쓰던 불일치가 있었음 - 이제 이 노드도 핸들만 들고, 실제 포인터는
// Update/Evaluate가 불릴 때마다 URenderingManager::Get()으로 조회함.
//
// [새로 생기는 의존성] 이 조회 때문에 Animation 폴더가 Rendering/URenderingManager를
// 참조하게 됨 - 예전엔 Animation 폴더가 Rendering을 몰라도 됐지만(URenderingManager가
// Animation을 아는 단방향), 이제 양방향이 됨. URenderingManager가 이 노드 타입을
// 직접 아는 건 아니라서(여전히 IAnimGraphBase로만 타입 지워진 채 다룸) 진짜
// 순환 include는 아니지만, 결합도가 늘어난 지점이라 명시해둠.
//
// [스레드 안전성] 이 조회는 워커 스레드에서 일어남 - URenderingManager의 정적 에셋
// 맵(Skeletons/AnimSequences)이 게임 시작 전 초기화 시점에만 채워지고 이후로는
// 절대 다시 안 바뀐다는 전제 하에서만 안전함(여러 스레드가 "읽기만" 동시에 하는 건
// 안전, "쓰기"가 동시에 섞이면 위험). 나중에 런타임 중 에셋 재로딩/핫스왑 같은 걸
// 붙이게 되면 이 전제가 깨지므로 그때 다시 봐야 함.
template<typename TSnapshot>
class FAnimNode_SequencePlayer : public UAnimNode_Base<TSnapshot>
{
public:
	FAssetHandle SequenceHandle = 0;
	FAssetHandle SkeletonHandle = 0;
	float PlayTime = 0.0f;
	bool bLoop = true;

	void Update(float DeltaSeconds, const TSnapshot& Snapshot) override;
	void Evaluate(TArray<FBoneTransform>& OutPose, const TSnapshot& Snapshot) override;

	// [Land 같은 상태 판정용] bLoop==true면 "끝"이라는 개념 자체가 없어서 항상
	// false. bLoop==false일 때만, 재생 시간이 시퀀스 길이를 다 채웠는지로 판정.
	bool IsFinished() const override;
	float GetPlayRatio() const override;
	void ResetForStateEntry() override;
};

#include "AnimNode_SequencePlayer.inl"
