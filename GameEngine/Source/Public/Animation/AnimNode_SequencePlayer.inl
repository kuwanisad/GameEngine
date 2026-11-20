#pragma once
// 템플릿 구현부. 템플릿은 인스턴스화 지점에서 정의가 보여야 하므로 .cpp가 아니라
// .inl로 분리해서 헤더 끝에서 include함(우리 프로젝트의 다른 .cpp 관례와는 다르지만,
// 템플릿 클래스라 불가피함).
#include "AnimSequence.h"
#include "../Asset/Skeleton.h"
#include "../Rendering/RenderingManager.h"
#include "../Asset/AssetManager.h"

#include <cmath>

template<typename TSnapshot>
void FAnimNode_SequencePlayer<TSnapshot>::Update(float DeltaSeconds, const TSnapshot& Snapshot)
{
	// [핸들 조회] 매 호출마다 실제 포인터를 다시 얻어옴 - 정적 에셋이라 매번 같은
	// 결과가 나오지만, 캐싱해서 얻는 이득보다 "핸들 하나만 들고 있으면 된다"는
	// 단순함이 지금 규모엔 더 낫다고 판단함.
	UAnimSequence* Sequence = UAssetManager::Get()->GetAnimSequence(SequenceHandle);

	if (Sequence == nullptr)
	{
		return;
	}

	PlayTime += DeltaSeconds;

	if (bLoop && Sequence->PlayLength > 0.0f)
	{
		// [수정] while로 한 번씩 빼던 걸 std::fmod로 - 수학적으로 동일한 결과를
		// 한 번에 계산함(반복 없음). AccumulatedDeltaSeconds(여러 프레임 봐주기가
		// 겹쳤을 때의 누적 시간) 때문에 PlayTime이 한 번에 크게 뛸 수 있는데, 그
		// 경우 while이면 여러 번 반복해야 했을 걸 fmod는 항상 한 번에 끝냄.
		if (PlayTime >= Sequence->PlayLength)
		{
			PlayTime = std::fmod(PlayTime, Sequence->PlayLength);
		}
	}
	else if (!bLoop && Sequence->PlayLength > 0.0f)
	{
		PlayTime = PlayTime > Sequence->PlayLength ? Sequence->PlayLength : PlayTime;
	}
}

template<typename TSnapshot>
bool FAnimNode_SequencePlayer<TSnapshot>::IsFinished() const
{
	if (bLoop)
	{
		return false;   // 루프면 "끝"이라는 개념 자체가 없음
	}

	UAnimSequence* Sequence = UAssetManager::Get()->GetAnimSequence(SequenceHandle);
	if (Sequence == nullptr || Sequence->PlayLength <= 0.0f)
	{
		return false;   // 재생할 게 없으면 "끝났다"고 볼 근거도 없음
	}

	return PlayTime >= Sequence->PlayLength;
}

template<typename TSnapshot>
float FAnimNode_SequencePlayer<TSnapshot>::GetPlayRatio() const
{
	UAnimSequence* Sequence = UAssetManager::Get()->GetAnimSequence(SequenceHandle);
	if (Sequence == nullptr || Sequence->PlayLength <= 0.0f)
	{
		return 0.0f;
	}

	return Math::Clamp(PlayTime / Sequence->PlayLength, 0.0f, 1.0f);
}

template<typename TSnapshot>
void FAnimNode_SequencePlayer<TSnapshot>::ResetForStateEntry()
{
	// [핵심 수정] PlayTime을 0으로 되돌림 - 이게 없으면 bLoop=false인 노드가
	// (Jump/Land 등) 한 번 끝까지 재생된 뒤, 나중에 그 상태로 다시 들어와도
	// "이미 끝난 마지막 프레임"부터 시작해서 재생 없이 고정된 자세로 보였음.
	PlayTime = 0.0f;
}

template<typename TSnapshot>
void FAnimNode_SequencePlayer<TSnapshot>::Evaluate(TArray<FBoneTransform>& OutPose, const TSnapshot& Snapshot)
{
	
	UAnimSequence* Sequence = UAssetManager::Get()->GetAnimSequence(SequenceHandle);
	const USkeleton* SkeletonRef = UAssetManager::Get()->GetSkeleton(SkeletonHandle);

	if (SkeletonRef == nullptr)
	{
		return;
	}

	const UReferenceSkeleton& RefSkel = SkeletonRef->GetReferenceSkeleton();
	int32 BoneCount = RefSkel.GetBoneCount();
	if (static_cast<int32>(OutPose.size()) != BoneCount)
	{
		OutPose.resize(BoneCount);
	}

	for (int32 i = 0; i < BoneCount; i++)
	{
		// [바인드포즈 폴백] 항등 트랜스폼(회전 없음, 이동 없음)으로 채우면 모든 본이
		// 원점 근처로 뭉쳐서 캐릭터가 찌그러진 모습으로 나옴 - 실제 T포즈를 이루는
		// 값은 RawRefBonePose에 있음. Sequence 자체가 없거나(nullptr), 있어도 이
		// 본의 트랙 데이터가 비어있으면(Times가 비어있으면) 이쪽으로 옴.
		bool bHasTrackData = (Sequence != nullptr)
			&& (i < static_cast<int32>(Sequence->BoneTracks.size()))
			&& !Sequence->BoneTracks[i].Times.empty();

		if (bHasTrackData)
		{
			OutPose[i] = Sequence->GetBoneTransform(i, PlayTime);
		}
		else
		{
			const FTransform& BindPose = RefSkel.GetRefBonePose(i);
			OutPose[i] = FBoneTransform(BindPose.Rotation, BindPose.Translation);
		}

		// [루트 모션 억제 - 여기서 시퀀스별로 판단] 이 노드는 자기가 재생 중인
		// Sequence가 뭔지 정확히 알고 있는 유일한 지점이라, bForceRootLock 판단은
		// 반드시 여기서 해야 함(URenderingManager의 최종 팔레트 계산 지점에서는
		// BlendSpace2D 등이 이미 여러 시퀀스를 섞어버린 뒤라 "어느 시퀀스에서
		// 왔는지" 정보가 사라짐). 회전은 그대로 두고 이동만 지움 - 캐릭터가
		// 걸으며 방향을 트는 것 자체는 정상적으로 보여야 하므로.
		if (Sequence != nullptr && Sequence->bForceRootLock && RefSkel.GetParentIndex(i) == INDEX_NONE)
		{
			OutPose[i].Translation = FVector3::Zero;
		}
	}
}
