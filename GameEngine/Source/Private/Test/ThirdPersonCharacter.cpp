#include "../../Public/Test/ThirdPersonCharacter.h"
#include "../../Public/Component/MovementComponent.h"
#include "../../Public/Rendering/Renderer.h"
#include "../../Public/Core/Engine.h"
#include "../../Public/Camera/FPSCamera.h"
#include "../../Public/Component/SkeletalMeshComponent.h"
#include "../../Public/Component/BoxComponent.h"
#include "../../Public/Collision/Collision.h"
#include "../../Public/Input/EnhancedInputComponent.h"
#include "../../Public/Input/InputMappingContext.h"
#include "../../Public/Test/GameplayInputActions.h"
#include "../../Public/Input/InputModifiers.h"
#include "../../Public/Input/InputTriggers.h"
#include "../../Public/Rendering/RenderingManager.h"
#include "../../Public/Asset/SkeletalMesh.h"
#include "../../Public/Asset/Skeleton.h"
#include "../../Public/Asset/AssetManager.h"
#include "../../Public/Test/FPSCharacterAnimInstance.h"
#include "../../Public/Core/Cast.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "../../Public/Core/TestManager.h"
#endif

AThirdPersonCharacter::AThirdPersonCharacter(const FString& SkeletalMeshPath) : AActor()
{
	// [수정] new XComponent(this) 직접 생성은 Init()이 안 불려서 Owner->Components
	// 등록/틱 등록/(BoxComp의 경우) 콜리전 생성이 전부 빠짐 - UEngine::CreateComponent<T>()
	// 로 교체해서 생성과 Init() 호출을 하나로 묶음. AFPSCamera는 이름이 "A" 접두사라
	// 액터처럼 보이지만 실제로는 UCameraComponent를 상속하는 컴포넌트라 동일하게 적용.
	MoveComp = UEngine::GetEngine()->CreateComponent<UMovementComponent>(this);
	MoveComp->SetUseCCD(true);

	// [FPSActor와 동일한 이유] 이 액터의 Tick()이 MoveComp가 이번 프레임에 이미
	// 적용한 요(회전)를 읽어야 함 - MoveComp가 먼저 끝나야 함을 명시적으로 걸어둠.
	PrimaryActorTick.AddPrerequisite(&MoveComp->PrimaryComponentTick);

	CameraComp = UEngine::GetEngine()->CreateComponent<AFPSCamera>(this);

	// [FPSActor와 동일] 일반적인 사람 크기로 임의로 잡음.
	BoxComp = UEngine::GetEngine()->CreateComponent<UBoxComponent>(this);
	BoxComp->SetObjectBox(FAABB(FVector3(-25.0f, -25.0f, -44.0f), FVector3(25.0f, 25.0f, 44.0f)));

	// [부착 계층] 콜리전 박스를 루트로 삼음(FPSActor와 동일). 스켈레탈 메시는
	// FPSActor의 FPSModel(별도 액터)과 달리, 이 박스에 바로 부착함 - 3인칭이라
	// 카메라가 모델 내부에 있는 문제 자체가 없어서 별도 액터로 분리할 이유가 없음.
	SetRootComponent(BoxComp);
	CameraComp->SetupAttachment(BoxComp);
	// [3인칭 카메라 임시 오프셋] FPSCamera가 원래 1인칭(눈높이)용이라, BoxComp에
	// 그대로 붙이면 카메라가 캐릭터 몸 안에 파묻힘 - 뒤(-X)+위(+Z)로 떨어뜨림.
	CameraComp->SetRelativeLocation(FVector3(-200, 0, 300));

	// [스켈레탈 메시 + 애니메이션 그래프] 캐릭터 모델 로드, 정적 에셋 등록,
	// Idle<->Locomotion 스테이트 머신 그래프 등록, AnimInstance 연결까지 한 번에.
	SkelMeshComp = nullptr;
#if WITH_DEV_AUTOMATION_TESTS
	FAssetHandle MeshHandle;
	FAssetHandle SkelHandle;

	if (UAssetManager::Get()->LoadSkeletalMesh(SkeletalMeshPath, MeshHandle, SkelHandle))
	{
		auto Skel = UAssetManager::Get()->GetSkeleton(SkelHandle);
		FAnimGraphHandle GraphHandle = UTestManager::RegisterCharacterAnimGraph(SkelHandle, Skel);

		SkelMeshComp = UEngine::GetEngine()->CreateComponent<USkeletalMeshComponent>(this);
		SkelMeshComp->SetupAttachment(BoxComp);
		// [수정됨 - 도->라디안 변환 누락] Quaternion(axis, angle)은 라디안을 받는데
		// 90.f(도 단위 그대로)를 넣었던 게 원인이었음 - 90라디안(약 14.32바퀴)만큼
		// 돌아가버려서 의도와 다른 각도로 보였던 것.
		SkelMeshComp->SetRelativeRotation(FQuaternion(FVector3::UnitZ, 90.0f * Math::Pi / 180.0f));

		UFPSCharacterAnimInstance* AnimInst = new UFPSCharacterAnimInstance();
		AnimInst->SkeletonHandle = SkelHandle;
		AnimInst->SkeletalMeshHandle = MeshHandle;
		AnimInst->AnimGraphHandle = GraphHandle;
		SkelMeshComp->SetAnimInstance(AnimInst);
	}
#endif

	// [입력 시스템] FPSActor와 완전히 동일한 구성 - Move/Turn/LookUp/Jump.
	InputComp = UEngine::GetEngine()->CreateComponent<UEnhancedInputComponent>(this);
	MappingContext = new UInputMappingContext();
	MappingContext->AddKeyMapping(EKeys::W, &GameplayInputActions::Move(),
		{ new UInputModifierSwizzleAxis(EInputAxisSwizzle::YXZ) });
	MappingContext->AddKeyMapping(EKeys::S, &GameplayInputActions::Move(),
		{ new UInputModifierSwizzleAxis(EInputAxisSwizzle::YXZ), new UInputModifierNegate() });
	MappingContext->AddKeyMapping(EKeys::A, &GameplayInputActions::Move(),
		{ new UInputModifierNegate() });
	MappingContext->AddKeyMapping(EKeys::D, &GameplayInputActions::Move());
	MappingContext->AddKeyMapping(EKeys::MouseX, &GameplayInputActions::Turn());
	MappingContext->AddKeyMapping(EKeys::MouseY, &GameplayInputActions::LookUp());
	MappingContext->AddKeyMapping(EKeys::SpaceBar, &GameplayInputActions::Jump());
	InputComp->SetMappingContext(MappingContext);

	const float MaxSpeed = 400.0f;
	InputComp->BindAction(&GameplayInputActions::Move(), [this, MaxSpeed](const FInputActionValue& Value)
		{
			FVector2 Move = Value.Get<FVector2>();
			if (Move.LengthSq() > 1.0f)
			{
				Move.Normalize();
			}

			MoveComp->SetForwardSpeed(Move.y * MaxSpeed);
			MoveComp->SetStrafeSpeed(Move.x * MaxSpeed);

			// [애님 인스턴스에 직접 푸시 - 방향 부호 확실히 보존] MoveComp의
			// GetForwardSpeed/GetStrafeSpeed를 매 프레임 다시 읽어오는(pull) 대신,
			// 입력이 실제로 발생하는 이 시점에 원본 부호 있는 Move 벡터를 그대로
			// 애님 인스턴스에 밀어넣음 - "방향 구분이 없는 것 같다"는 의심을
			// 근본적으로 우회함(GetForwardSpeed/GetStrafeSpeed의 실제 구현을
			// 안 봐도 됨).
			if (SkelMeshComp != nullptr)
			{
				UFPSCharacterAnimInstance* AnimInst = Cast<UFPSCharacterAnimInstance>(SkelMeshComp->GetAnimInstance());
				if (AnimInst != nullptr)
				{
					// [수정됨 - 부호 반전 필요] SkelMeshComp가 BoxComp 기준 Z축 90도
					// 회전돼 있어서(그 위 SetupAttachment 참고), 실제 이동 좌표계
					// (BoxComp/MoveComp 기준)와 애니메이션이 참조하는 좌표계 사이에
					// 좌우 부호가 뒤집혀 있는 것으로 추정됨(정확한 근본 원인은 FbxImporter의
					// 좌표계 변환까지 파봐야 확정 가능 - 지금은 실측으로 확인된 값 사용).
					AnimInst->VelocityX = -Move.x * MaxSpeed;
					AnimInst->VelocityY = Move.y * MaxSpeed;
				}
			}
		});

	const float MaxAngularSpeed = Math::Pi * 0.75f;
	InputComp->BindAction(&GameplayInputActions::Turn(), [this, MaxAngularSpeed](const FInputActionValue& Value)
		{
			MoveComp->SetAngularSpeed(Value.Get<float>() * MaxAngularSpeed);
		});

	const float MaxPitchSpeed = Math::Pi * 0.75f;
	InputComp->BindAction(&GameplayInputActions::LookUp(), [this, MaxPitchSpeed](const FInputActionValue& Value)
		{
			CameraComp->SetPitchSpeed(Value.Get<float>() * MaxPitchSpeed);
		});

	const float JumpVelocity = 500.0f;
	InputComp->BindAction(&GameplayInputActions::Jump(), ETriggerEvent::Started, new UInputTriggerPressed(),
		[this, JumpVelocity](const FInputActionValue& Value)
		{
			if (MoveComp->IsOnGround())
			{
				MoveComp->AddImpulse(FVector3(0.0f, 0.0f, JumpVelocity));
			}
		});
}

AThirdPersonCharacter::~AThirdPersonCharacter()
{
	delete MappingContext;
}

void AThirdPersonCharacter::Tick(float DeltaTime)
{
	AActor::Tick(DeltaTime);

	// [즉시 반영 - FPSActor와 동일] 요(MoveComp)와 같은 타이밍에 피치도 여기서 확정.
	CameraComp->IntegratePitch(DeltaTime);

	// [FPSActor와 달리 별도 모델 액터 위치 계산이 필요 없음] SkelMeshComp가 이미
	// BoxComp(=이 액터의 루트)에 부착돼있어서, 이 액터의 트랜스폼을 그대로 따라감.

#if WITH_DEV_AUTOMATION_TESTS
	// [VelocityZ/bIsOnGround - 물리 상태라 여기서 pull 방식으로 채움] VelocityX/Y와
	// 달리 입력 이벤트가 아니라 중력/충돌 결과라, 입력 콜백이 아니라 매 프레임 이
	// Tick()에서 직접 계산해서 애님 인스턴스에 채워넣음.
	//
	// [수정됨 - 레이캐스트 방식 전체 제거] UMovementComponent가 이미 자기만의 착지
	// 판정(bIsOnGround)을 갖고 있었고, IsOnGround()로 공개하고 있었음(점프 입력
	// 콜백에서 이미 MoveComp->IsOnGround()를 쓰고 있었던 걸 뒤늦게 발견함) - 별도로
	// GameplayStatics::Raycast를 새로 만들어 썼던 건 불필요한 중복이었고, 그 경로
	// 자체에 버그(BVH::IntersectSegmentWithObject에 AABB/OBB 두께 0 케이스가
	// 걸리는 문제 등)도 있었음. MoveComp가 이미 검증된 로직으로 판정해주므로 그걸
	// 그대로 재사용함.
	if (SkelMeshComp != nullptr)
	{
		UFPSCharacterAnimInstance* AnimInst = Cast<UFPSCharacterAnimInstance>(SkelMeshComp->GetAnimInstance());
		if (AnimInst != nullptr)
		{
			AnimInst->VelocityZ = MoveComp->GetVelocity().z;
			AnimInst->bIsOnGround = MoveComp->IsOnGround();
		}
	}
#endif
}