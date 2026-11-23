#include "../../Public/Component/MovementComponent.h"
#include "../../Public/Actor/Actor.h"
#include "../../Public/Core/Engine.h"
#include "../../Public/Physics/PhysScene.h"

#include <vector>

UMovementComponent::UMovementComponent(class AActor* owner)
	:UActorComponent(owner)
	, AngularSpeed(0.0f)
	, ForwardSpeed(0.0f)
	, StrafeSpeed(0.0f)
{
	// PrimaryComponentTick.CanEverTick 기본값은 false(실제 UE와 동일) - 이동 로직이
	// Tick() 안에 있으니 이 컴포넌트는 직접 켜줘야 함.
	PrimaryComponentTick.CanEverTick = true;
}

void UMovementComponent::Tick(float deltaTime)
{
	if (!Math::NearZero(PendingLaunchVelocity.LengthSq()))
	{
		Velocity = PendingLaunchVelocity;
		PendingLaunchVelocity = FVector3::Zero;
	}

	Velocity += AccumulatedForce * deltaTime;
	AccumulatedForce = FVector3::Zero;

	if (bEnableGravity && !bIsOnGround)
	{
		Velocity += FPhysScene::Gravity * deltaTime;
	}

	if (!Math::NearZero(AngularSpeed))
	{
		FQuaternion rot = Owner->GetActorRotation();
		float angle = AngularSpeed * deltaTime;
		FQuaternion inc(FVector3::UnitZ, angle);
		rot = FQuaternion::Concatenate(rot, inc);
		Owner->SetActorRotation(rot);
	}

	if (!Math::NearZero(ForwardSpeed) || !Math::NearZero(StrafeSpeed)
		|| !Math::NearZero(Velocity.LengthSq()) || (bUseCCD && !Math::NearZero(AngularSpeed)))
	{
		FVector3 desiredPos = Owner->GetActorLocation();
		desiredPos += Owner->GetActorForwardVector() * ForwardSpeed * deltaTime;
		desiredPos += Owner->GetActorRightVector() * StrafeSpeed * deltaTime;
		desiredPos += Velocity * deltaTime;

		// [버그 수정 - 착지 판정 오염 방지] 아래 CCD 슬라이딩 블록이 desiredPos를
		// (수평 충돌 처리 과정에서) 다시 계산하면서 .z 성분까지 같이 바뀔 수 있음 -
		// 수평 이동이 있을 때만 이 블록이 실행되는데, 그럴 때만 착지 판정이 깨졌던
		// 원인이 정확히 이것이었음(확인됨: 순수 낙하는 이 블록 자체가 스킵되니
		// 문제없었고, 수평 이동을 곁들이면 깨졌음). CCD가 desiredPos를 건드리기
		// "전"의, 순수하게 중력/속도만으로 원했던 Z를 따로 저장해뒀다가, 착지
		// 판정은 반드시 이 값을 기준으로 함 - 수평 충돌 처리 결과와 수직 착지
		// 판정을 완전히 분리시킴.
		float PureVerticalDesiredZ = desiredPos.z;

		if (bUseCCD && (!Math::NearZero(ForwardSpeed) || !Math::NearZero(StrafeSpeed)))
		{
			// 선형 이동으로 인한 터널링은 정확한 스윕으로 잡음 (UE의 "sweep-based CCD"와
			// 같은 역할 - 회전 없는 순수 이동에 한해 정확함). 막히면 그 자리에서 뚝 멈추는
			// 대신, 벽 표면에 평행한 성분(파고드는 성분만 제거하는 속도 클리핑)으로 남은
			// 이동을 이어감 - Quake의 ClipVelocity와 같은 방식. 코너처럼 한 번의 클리핑으로
			// 부족한 경우를 위해 FPhysScene::MaxSlideIterations회까지 반복함.
			FVector3 fromPos = Owner->GetActorLocation();
			FVector3 remaining = desiredPos - fromPos;

			for (int i = 0; i < FPhysScene::MaxSlideIterations && !Math::NearZero(remaining.LengthSq()); i++)
			{
				float t;
				FHitResult hit;

				if (!UEngine::GetEngine()->GetPhysScene()->SweepQuery(Owner, fromPos, fromPos + remaining, t, hit))
				{
					fromPos = fromPos + remaining;
					remaining = FVector3::Zero;
					break;
				}

				fromPos = fromPos + remaining * t;

				FVector3 leftover = remaining * (1.0f - t);
				leftover -= hit.Normal * FVector3::Dot(hit.Normal, leftover);
				remaining = leftover;
			}

			desiredPos = fromPos;
		}

		float margin = 0.0f;
		if (bUseCCD && !Math::NearZero(AngularSpeed))
		{
			std::vector<FCollision*> ownerCollisions = Owner->GetCollisions();
			if (!ownerCollisions.empty())
			{
				float boundingRadius = ownerCollisions[0]->GetBoundingBox().GetExtent().Length();
				margin = Math::Abs(AngularSpeed) * boundingRadius * deltaTime;
			}
		}

		Owner->SetActorLocation(desiredPos, /*bSweep=*/ true, margin);

		// [착지 판정 - 버그 수정됨] desiredPos.z(CCD 슬라이딩으로 오염됐을 수 있는
		// 값) 대신, 위에서 미리 저장해둔 PureVerticalDesiredZ(순수 낙하 목표치)를
		// 기준으로 비교함.
		if (Velocity.z < 0.0f)
		{
			float ActualZ = Owner->GetActorLocation().z;
			const float LandingEpsilon = 0.5f;
			if (ActualZ > PureVerticalDesiredZ + LandingEpsilon)
			{
				bIsOnGround = true;
				Velocity.z = 0.0f;
			}
			else
			{
				bIsOnGround = false;
			}
		}
		else if (Velocity.z > 0.0f)
		{
			bIsOnGround = false;
		}
	}
}