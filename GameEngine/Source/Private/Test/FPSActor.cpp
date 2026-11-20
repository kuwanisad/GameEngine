#include "../../Public/Test/FPSActor.h"
#include "../../Public/Component/MovementComponent.h"
#include "../../Public/Rendering/Renderer.h"
#include "../../Public/Core/Engine.h"
#include "../../Public/Camera/FPSCamera.h"
#include "../../Public/Component/StaticMeshComponent.h"
#include "../../Public/Component/BoxComponent.h"
#include "../../Public/Collision/Collision.h"
#include "../../Public/Input/EnhancedInputComponent.h"
#include "../../Public/Input/InputMappingContext.h"
#include "../../Public/Test/GameplayInputActions.h"
#include "../../Public/Input/InputModifiers.h"
#include "../../Public/Input/InputTriggers.h"
#include "../../Public/Rendering/RenderingManager.h"
#include "../../Public/Asset/AssetManager.h"

#include <iostream>

AFPSActor::AFPSActor() : AActor()
{
	MoveComp = new UMovementComponent(this);
	// [버그 수정] bUseCCD 기본값이 false인데 여기서 켜준 적이 없어서, UMovementComponent::
	// Update()의 스윕(연속 충돌 감지) 블록 전체가 항상 스킵되고 있었음 - 그러면 desiredPos가
	// 경로 중간 검사 없이 그냥 한 프레임 치 이동량만큼 계산되고, 만약 그 이동이 장애물을
	// 완전히 건너뛰기 충분하면(또는 얇은 벽이면) MoveActor의 최종 위치 기준 겹침 검사도
	// 이미 장애물을 지나친 지점에서 이뤄져서 아무것도 못 잡음 - 실제로 플레이어가 물리
	// 테스트 박스를 그냥 통과하는 버그로 확인됨.
	MoveComp->SetUseCCD(true);

	// [Prerequisite] 액터 기본 그룹(TG_PrePhysics)이 컴포넌트 기본 그룹
	// (TG_DuringPhysics_ComponentTick)보다 먼저 도는데, 이 액터의 Tick()은 MoveComp가
	// 이번 프레임에 이미 적용한 요(GetActorRotation())를 읽어야 함(총 모델 회전 계산에
	// 씀) - 그래서 MoveComp가 먼저 끝나야 한다고 명시적으로 걸어둠(UE의
	// AddTickPrerequisiteComponent와 동일한 목적/이름).
	PrimaryActorTick.AddPrerequisite(&MoveComp->PrimaryComponentTick);

	LastFootstep = 0.f;
	CameraComp = new AFPSCamera(this);

	// [버그 수정] BoxComp가 지금까지 생성자에서 한 번도 할당된 적이 없어서(선언만 되고
	// nullptr조차 아닌 미초기화 포인터였음), 플레이어가 FPhysScene에 콜리전으로 전혀
	// 등록이 안 돼 있었음(AActor::GetCollisions()가 항상 빈 목록을 반환) - 사실상 플레이어가
	// 충돌 검사 대상에서 통째로 빠져 있던 상태. 참조할 몸체 메시가 따로 없어서(FPSModel은
	// 총 모델일 뿐), 크기는 일반적인 사람 크기로 임의로 잡아둠 - 실제 게임 스케일에 맞게
	// 조정 필요함.
	BoxComp = new UBoxComponent(this);
	BoxComp->SetObjectBox(FAABB(FVector3(-25.0f, -25.0f, -44.0f), FVector3(25.0f, 25.0f, 44.0f)));

	FPSModel = UEngine::GetEngine()->SpawnActor<AActor>();
	FPSModel->SetActorScale(FVector3::One * 0.75f);

	MeshComp = new UStaticMeshComponent(FPSModel);
	MeshComp->SetMesh(UAssetManager::Get()->LoadMesh("Assets/Rifle.gpmesh"));
	FPSModel->SetRootComponent(MeshComp);

	// [부착 계층] 콜리전 박스를 루트로 삼음 - 이동/충돌은 다 이 박스의 위치를 기준으로
	// 이뤄지니까. 무기 메시(MeshComp)는 여기 부착함(그냥 시각적으로 따라다니기만 하면
	// 되니까 - 실제 위치는 어차피 FPSActor::UpdateActor에서 별도 오프셋으로 매 프레임
	// FPSModel한테 다시 계산돼서 넘어가므로, 부착 자체보다는 일관성을 위한 것에 가까움).
	// [비고] CameraComp(FPSCamera)는 UCameraComponent가 USceneComponent를 상속 안 해서
	// (예전부터 알려진 한계) 이 부착 계층에 못 들어감 - 여전히 Owner->GetActorLocation()을
	// 직접 읽는 방식으로 남아있음.
	SetRootComponent(BoxComp);
	// [UCameraComponent가 이제 USceneComponent를 상속하면서 새로 가능해짐] 예전엔
	// UCameraComponent가 UActorComponent에서 바로 파생돼서 부착 계층에 아예 못 들어갔음 -
	// 지금은 카메라도 정식으로 부착 가능한 컴포넌트가 됨.
	CameraComp->SetupAttachment(BoxComp);

	// [입력 시스템] 액션-키 매핑을 여기서 한 번만 구성. 아래에서 각 액션에 대응하는
	// 콜백을 BindAction으로 등록함(더 이상 매 프레임 폴링하지 않음).
	// [수정] W/A/S/D 네 키를 하나의 Move(Axis2D) 액션으로 합침 - UE 실제 방식
	// (UInputModifierSwizzleAxis 확인) 그대로: W/S는 SwizzleAxis(YXZ)로 Y축에 배치,
	// A/D는 원래 X축 그대로 두고 부호만 조정.
	InputComp = new UEnhancedInputComponent(this);
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

	// [BindAction] 예전엔 ActorInput()에서 매 프레임 GetActionValue()로 직접 폴링했는데,
	// UE의 실제 방식(BindAction + FInputActionValue)대로 콜백을 미리 등록해두는 식으로
	// 바꿈 - 값이 계산되는 시점에 이 람다들이 곧바로 불림.
	const float MaxSpeed = 400.0f;
	InputComp->BindAction(&GameplayInputActions::Move(), [this, MaxSpeed](const FInputActionValue& Value)
		{
			FVector2 Move = Value.Get<FVector2>();

			// [대각선 이동 보정] W+D를 같이 누르면 Move가 (1,1)이 되는데, 이 벡터의
			// 크기는 sqrt(2)≈1.41 - 그대로 곱하면 대각선 이동이 정면/측면 단독
			// 이동보다 약 41% 더 빨라짐. 크기가 1을 넘을 때만 정규화해서 최대
			// 속력이 어느 방향이든 MaxSpeed를 넘지 않게 함(1을 안 넘는 경우, 예:
			// 살짝만 기울인 게임패드 스틱 입력 같은 건 그대로 둬야 하니 "넘을 때만"이
			// 중요함 - 여기선 디지털 키라 항상 0 아니면 1이지만, 나중에 아날로그 입력을
			// 붙일 걸 대비해서 이 조건을 유지함).
			if (Move.LengthSq() > 1.0f)
			{
				Move.Normalize();
			}

			MoveComp->SetForwardSpeed(Move.y * MaxSpeed);
			MoveComp->SetStrafeSpeed(Move.x * MaxSpeed);
		});

	// [주의] Turn/LookUp 값은 이미 Game::ProcessInput()에서 [-1,1]로 정규화됨(EKeys::MouseX/Y) -
	// 여기서 또 MaxMouseSpeed로 나누면 이중 정규화가 되니 하지 않음.
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

	// [점프] Boolean 값이라 Get<float>() 대신 콜백 파라미터 자체는 안 씀 - 여기까지
// 콜백이 불렸다는 사실 자체가 "이번 프레임에 눌렸다"는 뜻(Pressed 트리거 +
// Started 필터라 정확히 그 프레임에만 1회 호출됨). 점프는 순간적인 속도 변화라
// AddForce(지속 가속도)가 아니라 AddImpulse(즉시 반영)를 씀.
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

AFPSActor::~AFPSActor()
{
	// MappingContext는 UActorComponent가 아니라 순수 데이터 객체라 자동 정리가 안 됨
	// (InputComp 자신은 UActorComponent라 AActor 소멸자가 Components를 돌며 알아서 지움).
	delete MappingContext;
}

void AFPSActor::Tick(float DeltaTime)
{
	AActor::Tick(DeltaTime);
	LastFootstep -= DeltaTime;

	// [즉시 반영] 요(MoveComp)와 같은 타이밍(일반 그룹)에 피치도 여기서 확정.
	// 아래 총 모델 회전 계산이 CameraComp->GetPitch()를 읽기 전에 반드시 먼저 와야 함.
	CameraComp->IntegratePitch(DeltaTime);

	if (Math::NearZero(MoveComp->GetForwardSpeed()) == false &&
		LastFootstep <= 0.0f)
	{
		LastFootstep = 0.5f;
	}

	const FVector3 ModelOffset(FVector3(10.f, 10.f, -10.f));
	FVector3 ModelPos = GetActorLocation();
	ModelPos += GetActorForwardVector() * ModelOffset.x;
	ModelPos += GetActorRightVector() * ModelOffset.y;
	ModelPos.z += ModelOffset.z;
	FPSModel->SetActorLocation(ModelPos);
	FQuaternion q = GetActorRotation();
	q = FQuaternion::Concatenate(q, FQuaternion(GetActorRightVector(), CameraComp->GetPitch()));
	FPSModel->SetActorRotation(q);
}

void AFPSActor::SetFootstepSurface(float value)
{
}

void AFPSActor::SetVisible(bool _Visible)
{
	MeshComp->SetVisible(_Visible);
}
