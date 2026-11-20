#pragma once
#include "AnimNode_Base.h"
#include <functional>

// [전방 선언] FAnimTransition이 OwnerStateMachine으로 이 타입의 포인터를 담아야
// 하는데, 정의 순서상 FAnimNode_StateMachine이 파일 뒤쪽에 있어서 필요함.
template<typename TSnapshot>
class FAnimNode_StateMachine;

// UE의 FAnimState/FAnimationActiveTransitionEntry에 대응. FromStateIndex ==
// EntryStateIndex(스테이트 머신의 "Entry" 노드가 처음 가리키는 상태)로 시작함 -
// 별도의 Entry 노드 타입을 만들지 않고, 스테이트 머신 자신이 "시작 상태 인덱스"를
// 하나 들고 있는 것으로 대신함(UE의 실제 그래프는 Entry가 노드로 그려지지만,
// 런타임 의미상 "그냥 시작 상태를 가리키는 것"과 동일함).
template<typename TSnapshot>
struct FAnimState
{
	FString Name;
	UAnimNode_Base<TSnapshot>* StateNode = nullptr;   // 소유권 없음 - 그래프 구성 시 다른 곳과 공유될 수 있음
};

// [State Alias - 진짜 재사용 가능한 개체] UE Details 패널의 "States" 체크박스
// 목록에 대응 - "어떤 상태들이 이 별칭으로 들어올 수 있는지"만 담음. 이름을 갖고
// 있어서, 여러 전이(FAnimTransition)가 이 하나의 별칭을 가리키며 재사용할 수 있음
// (매 전이마다 출발 상태 목록을 중복 하드코딩하지 않도록).
template<typename TSnapshot>
struct FAnimStateAlias
{
	FString Name;
	// [TSet] "이 상태가 멤버인가"만 확인하면 되는 용도라, 순서가 의미 없는
	// 집합(TSet)이 배열보다 개념적으로 더 정확함.
	TSet<int32> MemberStateIndices;
};

template<typename TSnapshot>
struct FAnimTransition
{
	// [조립 시점 입력 - 이름으로만 적으면 됨] "이게 상태 이름인지 별칭 이름인지"를
	// 조립하는 사람이 미리 알 필요가 없음 - Init()이 States/Aliases를 순서 상관없이
	// 뒤져서 알아서 찾아줌. States/Aliases/Transitions를 어떤 순서로 채워도
	// 안전한 이유가 이것 때문 - 이름만 나중에 존재하면 됨.
	FString FromName;
	FString ToName;
	float CrossfadeDuration = 0.2f;

	// [Init()이 채움 - 직접 손대지 말 것] FromName/ToName을 실제로 찾아본 결과.
	// CheckForTransitions/MatchesFrom은 이 값들만 읽음(문자열 비교를 매 프레임
	// 반복하지 않도록 - 이름 찾기는 Init() 한 번뿐).
	bool bFromIsAlias = false;
	int32 FromIndex = INDEX_NONE;
	int32 ToStateIndex = INDEX_NONE;

	// [UE 실제 기능 이름 - "Automatic Rule Based on Sequence Player in State"]
	// 켜져 있으면, Condition을 아예 안 보고, OwnerStateMachine을 통해 FromName에
	// 해당하는 실제 재생 노드를 찾아 그 재생 비율(GetPlayRatio())이 다 찼는지로만
	// 판정함 - 수동으로 조건을 안 써도 됨. 예: Land->Locomotion의 "재생이 끝나면" 전이.
	bool bAutomaticRuleBasedOnSequencePlayerInState = false;

	// [Init()이 채움] 이 전이가 속한 스테이트 머신 자신 - 자동 규칙 판정 시
	// StateNameToState로 실제 재생 노드를 찾는 데 씀.
	FAnimNode_StateMachine<TSnapshot>* OwnerStateMachine = nullptr;

	// Condition은 private이라 이 함수로만 설정 가능 - 구조체 자체가 더 이상
	// 순수 애그리게이트가 아니게 됨(private 멤버가 있으면 중괄호 초기화로 값을
	// 직접 못 넣음) - 그래서 기본 생성 후 필드별로 설정 + 이 세터로 조립함.
	void SetCondition(std::function<bool(const TSnapshot&)> InCondition) { Condition = InCondition; }

	// [선언만 - 정의는 파일 뒤쪽, FAnimNode_StateMachine이 완전히 정의된 뒤에]
	// OwnerStateMachine의 멤버(StateNameToState)에 접근해야 해서, 그 타입이
	// 완전히 정의된 뒤로 미룸(전방 선언만으로는 멤버 접근이 안 됨).
	bool CheckCondition(const TSnapshot& Snapshot) const;

private:
	std::function<bool(const TSnapshot&)> Condition;   // Snapshot만 받음 - 살아있는 객체 캡처 금지
};

// [State Alias 대응 - 확인된 실제 UE 동작] "여러 상태가 같은 목적지로 전이할 때
// 화살표 하나로 묶는" 기능(예: Locomotion+Land 둘 다 -> Jump/Fall Loop로 갈 때
// "To Falling" 별칭 하나만 그으면 됨). FAnimStateAlias가 그 별칭을 이름 있는 진짜
// 개체로 표현함.
//
// UE의 FAnimNode_StateMachine에 대응(축소판). 자기 자신도 FAnimNode_Base<TSnapshot>라
// 다른 노드(FAnimNode_SaveCachedPose 등) 안에 그대로 연결될 수 있음 - 한 애니메이션
// 블루프린트 안에 스테이트 머신이 여러 개 있어도, 서로 SaveCachedPose/UseCachedPose로
// 연결하면 이 타입 하나로 전부 표현 가능함.
template<typename TSnapshot>
class FAnimNode_StateMachine : public UAnimNode_Base<TSnapshot>
{
public:
	void Update(float DeltaSeconds, const TSnapshot& Snapshot) override
	{
		if (States.empty())
		{
			return;
		}

		if (States[CurrentStateIndex].StateNode != nullptr)
		{
			States[CurrentStateIndex].StateNode->Update(DeltaSeconds, Snapshot);
		}

		if (bIsTransitioning)
		{
			if (TransitionTargetIndex != INDEX_NONE && States[TransitionTargetIndex].StateNode != nullptr)
			{
				States[TransitionTargetIndex].StateNode->Update(DeltaSeconds, Snapshot);
			}

			TransitionElapsedTime += DeltaSeconds;
			TransitionUpdateCallCount++;   // [진단 전용]

			if (TransitionElapsedTime >= TransitionDuration)
			{
				CurrentStateIndex = TransitionTargetIndex;
				bIsTransitioning = false;
				TransitionTargetIndex = INDEX_NONE;
				TransitionElapsedTime = 0.0f;
				TransitionUpdateCallCount = 0;
			}
		}
		else
		{
			int32 PrevStateIndex = CurrentStateIndex;
			CheckForTransitions(Snapshot);

			// [진단 강화] 새 전이가 시작된 순간을 이름으로 남김.
			if (bIsTransitioning)
			{
				FString PrevName = (PrevStateIndex >= 0 && PrevStateIndex < static_cast<int32>(States.size()))
					? States[PrevStateIndex].Name : FString("?");
				FString TargetName = (TransitionTargetIndex >= 0 && TransitionTargetIndex < static_cast<int32>(States.size()))
					? States[TransitionTargetIndex].Name : FString("?");
			}
		}
	}

	void Evaluate(TArray<FBoneTransform>& OutPose, const TSnapshot& Snapshot) override
	{
		if (States.empty() || States[CurrentStateIndex].StateNode == nullptr)
		{
			return;
		}

		if (!bIsTransitioning)
		{
			States[CurrentStateIndex].StateNode->Evaluate(OutPose, Snapshot);
			return;
		}

		TArray<FBoneTransform> FromPose;
		States[CurrentStateIndex].StateNode->Evaluate(FromPose, Snapshot);

		TArray<FBoneTransform> ToPose;
		if (TransitionTargetIndex != INDEX_NONE && States[TransitionTargetIndex].StateNode != nullptr)
		{
			States[TransitionTargetIndex].StateNode->Evaluate(ToPose, Snapshot);
		}

		float Alpha = TransitionDuration > 0.0f
			? Math::Clamp(TransitionElapsedTime / TransitionDuration, 0.0f, 1.0f)
			: 1.0f;

		size_t BoneCount = FromPose.size();
		OutPose.resize(BoneCount);
		for (size_t i = 0; i < BoneCount; i++)
		{
			OutPose[i] = (i < ToPose.size())
				? FBoneTransform::Interpolate(FromPose[i], ToPose[i], Alpha)
				: FromPose[i];
		}
	}

	int32 GetCurrentStateIndex() const { return CurrentStateIndex; }

	// UE의 "Entry" 노드가 가리키는 시작 상태를 지정 - 기본은 0번(States에 넣은 첫 상태).
	void SetEntryState(int32 StateIndex) { CurrentStateIndex = StateIndex; }

	// [이름으로 찾기 - 매직 넘버 하드코딩 방지] States/Aliases를 이름으로 채운 뒤,
	// 인덱스 대신 이름으로 전이를 조립할 때 씀. 못 찾으면 INDEX_NONE.
	int32 FindStateIndex(const FString& Name) const
	{
		for (size_t i = 0; i < States.size(); i++)
		{
			if (States[i].Name == Name)
			{
				return static_cast<int32>(i);
			}
		}
		return INDEX_NONE;
	}

	int32 FindAliasIndex(const FString& Name) const
	{
		for (size_t i = 0; i < Aliases.size(); i++)
		{
			if (Aliases[i].Name == Name)
			{
				return static_cast<int32>(i);
			}
		}
		return INDEX_NONE;
	}

	// [초기화 - States/Aliases/Transitions를 전부 채운 뒤, 첫 Update() 전에 반드시
	// 한 번 호출] Transitions의 FromName/ToName(이름 문자열)을 실제로 찾아서
	// bFromIsAlias/FromIndex/ToStateIndex를 채우고, OwnerStateMachine도 자기
	// 자신으로 채움(자동 규칙 판정용). StateNameToState도 같이 채워서, 자동 규칙이
	// 이름 하나로 바로 해당 상태를 찾을 수 있게 함.
	void Init()
	{
		StateNameToState.clear();
		for (const FAnimState<TSnapshot>& State : States)
		{
			StateNameToState[State.Name] = State;
		}

		for (FAnimTransition<TSnapshot>& Transition : Transitions)
		{
			Transition.OwnerStateMachine = this;

			int32 StateIdx = FindStateIndex(Transition.FromName);
			if (StateIdx != INDEX_NONE)
			{
				Transition.bFromIsAlias = false;
				Transition.FromIndex = StateIdx;
			}
			else
			{
				// 상태에서 못 찾았으면 별칭에서 찾음 - 여기서도 못 찾으면
				// FromIndex가 INDEX_NONE으로 남고, MatchesFrom이 그 경우 안전하게
				// false를 반환하도록 이미 방어돼 있음(범위 체크).
				Transition.bFromIsAlias = true;
				Transition.FromIndex = FindAliasIndex(Transition.FromName);
			}

			Transition.ToStateIndex = FindStateIndex(Transition.ToName);
		}
	}

	TArray<FAnimState<TSnapshot>> States;
	TArray<FAnimStateAlias<TSnapshot>> Aliases;
	TArray<FAnimTransition<TSnapshot>> Transitions;

	// [자동 규칙 판정 보조] 이름 -> 상태로 바로 찾는 조회 - FAnimTransition::
	// CheckCondition이 OwnerStateMachine을 통해 이걸 씀. Init()이 States로부터 채움.
	TMap<FString, FAnimState<TSnapshot>> StateNameToState;

private:
	// 지금 상태(CurrentStateIndex)가 이 전이의 출발점에 해당하는지 판정 -
	// 평범한 상태면 그냥 인덱스 비교, 별칭이면 TSet::find()로 빠르게 멤버십 확인.
	bool MatchesFrom(const FAnimTransition<TSnapshot>& Transition) const
	{
		if (!Transition.bFromIsAlias)
		{
			return Transition.FromIndex == CurrentStateIndex;
		}

		if (Transition.FromIndex < 0 || Transition.FromIndex >= static_cast<int32>(Aliases.size()))
		{
			return false;
		}

		auto FoundMember = Aliases[Transition.FromIndex].MemberStateIndices.find(CurrentStateIndex);

		if (FoundMember != Aliases[Transition.FromIndex].MemberStateIndices.end())
		{
			return true;
		}

		return false;
	}

	void CheckForTransitions(const TSnapshot& Snapshot)
	{
		for (const FAnimTransition<TSnapshot>& Transition : Transitions)
		{
			if (!MatchesFrom(Transition))
			{
				continue;
			}

			if (Transition.CheckCondition(Snapshot))
			{
				bIsTransitioning = true;
				TransitionTargetIndex = Transition.ToStateIndex;
				TransitionElapsedTime = 0.0f;
				TransitionDuration = Transition.CrossfadeDuration;

				// [핵심 수정] 목적지 상태로 "새로 들어온다"고 확정된 이 순간, 그 노드에게
				// 알려줌 - bLoop=false 노드(Jump/Land)가 재진입 시 PlayTime을 0으로
				// 되돌리지 않으면, 지난번 끝났던 마지막 프레임부터 시작해서 재생 없이
				// 고정된 자세로 보이는 버그가 있었음.
				if (TransitionTargetIndex >= 0 && TransitionTargetIndex < static_cast<int32>(States.size())
					&& States[TransitionTargetIndex].StateNode != nullptr)
				{
					States[TransitionTargetIndex].StateNode->ResetForStateEntry();
				}

				break;   // 한 프레임엔 하나의 전이만 시작 (지금 범위)
			}
		}
	}

	int32 CurrentStateIndex = 0;   // = Entry가 가리키는 시작 상태 (기본 0)

	bool bIsTransitioning = false;
	int32 TransitionTargetIndex = INDEX_NONE;
	float TransitionElapsedTime = 0.0f;
	float TransitionDuration = 0.0f;
	int32 TransitionUpdateCallCount = 0;   // [진단 전용]
};

// [FAnimTransition::CheckCondition - 밖에서 정의] FAnimNode_StateMachine이 여기서
// 완전히 정의됐으므로, 이제서야 OwnerStateMachine->StateNameToState에 안전하게
// 접근 가능함.
template<typename TSnapshot>
bool FAnimTransition<TSnapshot>::CheckCondition(const TSnapshot& Snapshot) const
{
	if (bAutomaticRuleBasedOnSequencePlayerInState)
	{
		if (OwnerStateMachine == nullptr)
		{
			return false;
		}

		auto Found = OwnerStateMachine->StateNameToState.find(FromName);
		if (Found == OwnerStateMachine->StateNameToState.end() || Found->second.StateNode == nullptr)
		{
			return false;
		}

		// [재생 비율로 판정] UE의 "Relevant Anim Time Remaining"과 같은 개념 -
		// 1.0(=100%, 끝까지 재생됨)에 도달했는지로 전이 여부를 정함.
		return Found->second.StateNode->GetPlayRatio() >= 1.0f;
	}

	return Condition && Condition(Snapshot);
}