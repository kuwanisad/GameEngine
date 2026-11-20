#pragma once
#include "../Animation/AnimInstance.h"
#include "../Animation/GENERATED_AnimInstance.h"

// 실제 게임플레이용 AnimInstance 파생 클래스 - 예전 UTestAnimInstance(테스트
// 전용, 빈 스냅샷)와 달리 진짜 이동 속도를 담음. FAnimNode_BlendSpace2D의
// GetBlendInput이 이 값들(정확히는 이걸로 만든 Snapshot)을 읽어서 블렌드 좌표로 씀.
//
// [단위 - 확인 필요] VelocityX/VelocityY의 단위가 매니페스트(RunBlendSpace.manifest)
// 에서 정한 좌표 단위(WalkSpeed=200 기준)와 실제로 맞는지는, 진짜 이동 로직
// (UMovementComponent 등)과 연결해봐야 확실해짐 - 지금은 임시로 시간 기반
// 스윕(원을 도는 값)으로 테스트만 함.
class UFPSCharacterAnimInstance : public UAnimInstance
{
public:
	GENERATED_AnimInstance(UFPSCharacterAnimInstance,
		float VelocityX = 0.0f;
	float VelocityY = 0.0f;
	float VelocityZ = 0.0f;      // 신규 - Jump vs Fall Loop 판정용(위/아래 속도)
	bool bIsOnGround = false;    // 신규 - 착지 판정용. ThirdPersonCharacter::Tick()이
	// GameplayStatics::Raycast로 매 프레임 직접 채움
		)

		// [테스트 전용 - 나중에 실제 이동 속도로 교체] 지금은 진짜 UMovementComponent
		// 연결이 없어서, 시간에 따라 블렌드 스페이스의 원을 한 바퀴씩 도는 값을
		// 인위적으로 만들어서 블렌딩이 실제로 눈에 보이는지 확인하는 용도.
		float DebugSweepTime = 0.0f;
	float DebugSweepSpeed = 200.0f;       // 매니페스트의 WalkSpeed와 맞춤
	float DebugSweepPeriodSeconds = 60.0f; // 한 바퀴 도는 데 걸리는 시간 (방향당 약 7.5초 - 전환을 눈으로 볼 수 있게 충분히 느리게)

	void NativeUpdateAnimation(float DeltaSeconds) override;
};
