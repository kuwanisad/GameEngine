#pragma once
#include "../Core/CoreMinimal.h"
#include "../Animation/AnimInstance.h"
#include "../Animation/AnimGraphRegistry.h"
#include "../Asset/AssetHandle.h"

#if WITH_DEV_AUTOMATION_TESTS

class Game;
class AFPSActor;
class UReferenceSkeleton;
class USkeleton;
class UAnimSequence;

// [통합됨] 예전엔 FTestLevelLoader / FFbxImportAutomationTest / FBlendSpaceUnitTest /
// UBlendSpaceTestGraph / UTestAnimInstance / CreateEmptyTestAnimSequence로 나뉘어
// 있던 걸, "지금 분할된 모든 테스트를 몰아서 진행할 클래스"로 하나로 합침. 크게
// 세 갈래 책임이 있음:
//   1) 게임을 실제로 실행하며 콘텐츠를 스폰(LoadTestLevel) - 예전 FTestLevelLoader
//   2) 게임을 안 켜고 로그만 남기고 끝나는 임포트/기하 검증(RunFbxTest,
//      RunBlendSpaceUnitTest) - 예전 FFbxImportAutomationTest/FBlendSpaceUnitTest
//   3) 애니메이션 그래프 조립 - 예전 UBlendSpaceTestGraph/UTestAnimInstance
//
// [정리된 것] 교재 원본 유산 코드(BallActor/BallMove/TargetActor - 실제로 스폰되는
// 곳이 없어서 확인 후 삭제)와, 미구현 스텁이었던 RunAnimTest(실제 ImportAnimation
// 검증은 지금까지의 --loadblendspace 화면 테스트로 충분히 확인됐다고 판단해 제거).
class UTestManager
{
public:
	// --loadskm 인자로 받은 FBX 경로. 설정돼 있으면 LoadTestLevel()이 그 자리에
	// 스켈레탈 메시 액터를 하나 스폰한다.
	static void SetDebugSkeletalMeshPath(const FString& Path) { DebugSkeletalMeshPath = Path; }

	// --loadanim 인자로 받은 애니메이션 전용 FBX 경로(예: MM_Idle.fbx, 스킨 메시
	// 없음). 설정돼 있으면 스켈레탈 메시 액터가 이 애니메이션을 재생하고, 없으면
	// 고의로 빈 테스트 그래프(바인드포즈)로 폴백한다.
	static void SetDebugAnimationPath(const FString& Path) { DebugAnimationPath = Path; }

	// --loadblendspace 플래그. 켜져 있으면 --loadanim 대신, 8방향 블렌드 스페이스
	// (RunBlendSpace.manifest)를 실제로 재생한다.
	static void SetUseBlendSpace(bool bInUse) { bUseBlendSpace = bInUse; }

	// [신규] --loadstatemachine 플래그. 켜져 있으면 ThirdPersonCharacter를 실제
	// 플레이어로 스폰해서, 진짜 입력(WASD)으로 Idle<->Locomotion 전이가 실제로
	// 일어나는지 확인함 - --loadblendspace(더미 액터+시간 기반 스윕)와 달리 진짜
	// 사용자 입력이 소스임.
	static void SetUseStateMachine(bool bInUse) { bUseStateMachine = bInUse; }

	// [public - 외부(ThirdPersonCharacter)에서 직접 호출] 8방향 + Idle(원점) 9개
	// 샘플을 담은 순수 블렌드 스페이스 하나만 등록함. [재설계됨] 예전엔 이걸
	// FAnimNode_StateMachine으로 별도 Idle 상태와 묶었는데, 실제 UE 구조를 확인해보니
	// 로코모션은 "순수 블렌드 스페이스 하나"로 Idle까지 전부 처리하고, 다른 노드와
	// 전이(트랜지션) 자체가 없었음 - 매니페스트의 원점 샘플이 이미 Idle 역할을 하므로
	// 중복이었음(FAnimNode_StateMachine 자체가 무의미해진 건 아니고, 나중에 진짜
	// 이산적인 상태(점프/공격/사망 등)에 쓰일 인프라로 남겨둠).
	// [--loadblendspace(더미 스윕)와 --loadstatemachine(진짜 입력) 둘 다 이 그래프를
	// 그대로 공유함] 차이는 오직 "누가 VelocityX/VelocityY를 채우는가"뿐, 그래프
	// 구조 자체는 완전히 동일함.
	static FAnimGraphHandle RegisterBlendSpaceTestAnimGraph(FAssetHandle SkeletonHandle, const USkeleton* SkeletonRef);

	// [public - 외부(ThirdPersonCharacter)에서 직접 호출] 위와 달리 진짜 게임플레이용.
	// Locomotion(블렌드 스페이스) + Jump/Fall Loop/Land를 FAnimNode_StateMachine으로
	// 묶은 전체 그래프. --loadblendspace(더미 스윕 테스트)는 bIsOnGround/VelocityZ가
	// 항상 기본값(false/0)이라 이 그래프를 쓰면 계속 "낙하 중" 상태에 갇히므로,
	// 반드시 RegisterBlendSpaceTestAnimGraph(순수 로코모션)와 분리해서 둠.
	static FAnimGraphHandle RegisterCharacterAnimGraph(FAssetHandle SkeletonHandle, const USkeleton* SkeletonRef);

	// [1) 게임 실행형] 데모/테스트 액터들을 전부 생성하고, 플레이어로 쓸 FPSActor를
	// 반환한다. (Game::TestFPSActor는 private이라 Game::LoadData()가 반환값을
	// 직접 대입해야 함)
	// [1) 게임 실행형] 데모/테스트 액터들을 전부 생성한다. 예전엔 "플레이어로 쓸
	// FPSActor"를 반환해서 Game::LoadData()가 Game::TestFPSActor에 대입했는데,
	// bUseStateMachine이면 플레이어가 FPSActor가 아니라 ThirdPersonCharacter여야
	// 해서 반환 타입 하나로 고정할 수 없게 됨 - 그래서 반환 자체를 없애고
	// (void), "누구를 플레이어로 삼을지"부터 "감시 등록(DebugManager::Watch)"까지
	// 전부 이 함수 내부에서 알아서 끝냄. Game은 이제 구체적으로 무슨 타입이
	// 스폰됐는지 전혀 몰라도 됨.
	static void LoadTestLevel();

	// [2) 로그 전용 - FBX 임포트 검증] --fbxtest skeleton|mesh|all 서브커맨드가 호출.
	// 게임을 켜지 않고 임포트 결과만 로그로 남기고 종료한다.
	static bool RunFbxSkeletonTest(const FString& FbxFilePath);
	static bool RunFbxMeshTest(const FString& FbxFilePath);
	static bool RunFbxAllTests(const FString& FbxFilePath);

	// [2) 로그 전용 - 블렌드 스페이스 기하 검증] --blendspacetest가 호출. 삼각분할 →
	// 무게중심 좌표 → FAnimNode_BlendSpace2D 세 층위를 독립적으로 검증한다.
	static bool RunBlendSpaceUnitTest();

private:
	static FString DebugSkeletalMeshPath;
	static FString DebugAnimationPath;
	static bool bUseBlendSpace;
	static bool bUseStateMachine;

	// ── 1) LoadTestLevel 내부 헬퍼 ──────────────────────────────
	static UAnimSequence* CreateEmptyTestAnimSequence();
	static FAnimGraphHandle RegisterEmptyTestAnimGraph(FAssetHandle SkeletonHandle);
	static FAnimGraphHandle RegisterRealTestAnimGraph(FAssetHandle SkeletonHandle, FAssetHandle SequenceHandle);

	// [신규 - "에셋 로드 후, 첫 틱 전" 설정 단계] 원래는 UE 에디터의 애셋 인스펙터
	// UI로 클립마다 켜고 껐을 설정(예: bForceRootLock)을, UI가 없는 우리 프로젝트
	// 에서는 여기 한 곳에 몰아서 코드로 적용함 - "임포트가 다 끝나고, 그 결과가
	// 실제로 재생되기(첫 틱) 전"이라는 시점이 보장되는 지점(그래프 빌더 함수
	// 안에서, 그래프가 아직 반환되기 전에 호출됨).
	static void ApplyBlendSpaceEntrySettings(const TArray<struct FBlendSpaceManifestEntry>& Entries);

	// [FAnimGraphFactoryFn(순수 함수 포인터)로 그대로 쓰기 위해 private static으로
	// 둠] static 멤버 함수의 주소는 일반 함수 포인터와 정확히 같은 타입이라(this
	// 포인터가 없으므로), FAnimGraphRegistry::RegisterFactory(&FTestManager::
	// BuildXxx)로 그대로 등록 가능함 - 이러면서도 private을 유지할 수 있어서,
	// ApplyBlendSpaceEntrySettings 같은 다른 private 멤버를 이 함수들이 자유롭게
	// 부를 수 있음(예전엔 익명 네임스페이스의 평범한 함수였는데, 그러면 클래스의
	// private 멤버에 접근할 권한이 없어서 여기로 옮김).
	static IAnimGraphBase* BuildEmptyTestAnimGraph();
	static IAnimGraphBase* BuildRealTestAnimGraph();
	static IAnimGraphBase* BuildBlendSpaceTestAnimGraph();
	static IAnimGraphBase* BuildCharacterAnimGraph();

	// ── 2) FBX 임포트 검증 내부 헬퍼 ────────────────────────────
	static void LogBoneHierarchy(const UReferenceSkeleton& Skeleton, int32 BoneIndex, int32 Depth);

	// ── 2) 블렌드 스페이스 단위 테스트 내부 헬퍼 ────────────────
	static bool TestTriangulation();
	static bool TestBarycentricWeights();
	static bool TestBlendSpaceNode();

	UTestManager() = delete;
};

// [테스트 전용] 검증할 게임플레이 값이 없어서, GetSnapshot()도 그냥 빈
// FAnimSnapshotBase를 반환하는 최소 스텁. "빈 애니메이션 -> 바인드포즈 폴백"이
// 실제로 동작하는지만 확인하려는 용도(--loadskm만 주고 --loadanim/--loadblendspace가
// 없을 때 씀). FTestManager 자신은 static 전용이라 인스턴스를 못 만들지만, 이건
// UAnimInstance 파생이라 인스턴스화가 필요해서 별도 클래스로 둠.
class UTestAnimInstance : public UAnimInstance
{
public:
	FAnimSnapshotBase* GetSnapshot() const override;
};

#endif // WITH_DEV_AUTOMATION_TESTS
