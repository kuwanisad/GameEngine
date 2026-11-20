#include "../../Public/Test/FPSCharacterAnimInstance.h"
#include "../../Public/Core/Math.h"
#include "../../Public/Actor/Actor.h"
#include "../../Public/Test/ThirdPersonCharacter.h"
#include "../../Public/Component/MovementComponent.h"
#include "../../Public/Core/Cast.h"

#include <cmath>

UFPSCharacterAnimInstance::UFPSCharacterAnimInstance_Snapshot* UFPSCharacterAnimInstance::GetSnapshot() const
{
	auto* Snap = new UFPSCharacterAnimInstance_Snapshot();
	Snap->VelocityX = VelocityX;
	Snap->VelocityY = VelocityY;
	Snap->VelocityZ = VelocityZ;
	Snap->bIsOnGround = bIsOnGround;
	return Snap;
}

void UFPSCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	// [수정 - pull에서 push로] 예전엔 여기서 매 프레임 MoveComp->GetStrafeSpeed()/
	// GetForwardSpeed()를 다시 읽어왔는데(pull), 그 함수들이 부호(방향)를 실제로
	// 보존하는지 확인이 안 된 상태였음 - 대신 ThirdPersonCharacter의 Move 입력
	// 콜백이 입력 발생 시점에 원본 부호 있는 벡터를 직접 밀어넣도록(push) 바꿈.
	// 그러니 여기서 다시 덮어쓰면(pull) 방금 들어온 push 값을 지워버리는 충돌이
	// 생기므로, 진짜 ThirdPersonCharacter면 아무것도 안 하고 그냥 넘어감 - 값은
	// 이미 입력 콜백이 최신 상태로 유지하고 있음.
	AThirdPersonCharacter* Character = Cast<AThirdPersonCharacter>(OwningActor);
	if (Character != nullptr)
	{
		return;
	}

	// [폴백 - 테스트 전용 스윕] 진짜 플레이어(ThirdPersonCharacter)가 아니면
	// (예: --loadblendspace의 더미 테스트 액터), 예전처럼 시간 기반 스윕으로
	// 블렌딩 자체만 확인함.
	DebugSweepTime += DeltaSeconds;

	float Angle = (DebugSweepTime / DebugSweepPeriodSeconds) * 2.0f * Math::Pi;
	VelocityX = std::cos(Angle) * DebugSweepSpeed;
	VelocityY = std::sin(Angle) * DebugSweepSpeed;
}