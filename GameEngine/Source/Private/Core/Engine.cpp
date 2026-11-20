#include "../../Public/Core/Engine.h"
#include "../../Public/Physics/PhysScene.h"
#include "../../Public/Tick/TickManager.h"
#include "../../Public/Input/InputSubsystem.h"
#include "../../Public/Rendering/RenderingManager.h"
#include "../../Public/Actor/Actor.h"
#include "../../Public/Core/TestManager.h"
#include "../../Public/Component/PrimitiveComponent.h"

#include <algorithm>
#include <GL/glew.h>
#include <SDL.h>

UEngine* UEngine::Instance = nullptr;

UEngine::UEngine()
	: IsRunning(true),
	IsUpdatingActors(false),
	TickCount(0),
	PhysScene(nullptr),
	InputSubsystem(nullptr),
	RenderingManager(nullptr),
	TickManager(nullptr)
{
}

UEngine::~UEngine()
{
}

bool UEngine::Initialize()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
	{
		SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
		return false;
	}

	RenderingManager = new URenderingManager();

	// [Renderer 흡수] Game이 더 이상 Renderer를 직접 생성/소유하지 않음 -
	// URenderingManager::Startup() 하나가 그 내부에서 대신 해줌.
	if (RenderingManager->Init(1024.0f, 768.0f) == false)
	{
		SDL_Log("Failed to initialize renderer");
		return false;
	}

	// AActor/UActorComponent들의 생성자가 GetPhysicsScene()를 바로 쓰기 때문에
	// (예: UBoxComponent가 생성 시점에 AddBox를 호출함), LoadData()보다 먼저 만들어야 함.
	PhysScene = new FPhysScene();

	LoadData();

	TickCount = SDL_GetTicks();

	return true;
}

void UEngine::ProcessInput()
{
	SDL_Event event;

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_QUIT:
			IsRunning = false;
			break;
		}
	}

	const Uint8* KeyState = SDL_GetKeyboardState(NULL);
	if (KeyState[SDL_SCANCODE_ESCAPE])
	{
		IsRunning = false;
	}

	// [SDL_SCANCODE_*가 등장하는 유일한 지점] 이 아래로는 FRawInputState만 전파됨 -
	// UEnhancedInputComponent 등 전부 SDL을 모름.
	FRawInputState RawInput;
	RawInput.SetPressed(EKeys::W, KeyState[SDL_SCANCODE_W]);
	RawInput.SetPressed(EKeys::A, KeyState[SDL_SCANCODE_A]);
	RawInput.SetPressed(EKeys::S, KeyState[SDL_SCANCODE_S]);
	RawInput.SetPressed(EKeys::D, KeyState[SDL_SCANCODE_D]);
	RawInput.SetPressed(EKeys::SpaceBar, KeyState[SDL_SCANCODE_SPACE]);

	int MouseDeltaX = 0;
	int MouseDeltaY = 0;
	SDL_GetRelativeMouseState(&MouseDeltaX, &MouseDeltaY);
	RawInput.SetAxis(EKeys::MouseX, Math::Clamp(float(MouseDeltaX), -1.0f, 1.0f));
	RawInput.SetAxis(EKeys::MouseY, Math::Clamp(float(MouseDeltaY), -1.0f, 1.0f));

	// [UInputSubsystem으로 전달] 예전엔 여기서 모든 Actor를 순회하며
	// actor->ProcessInput(RawInput)을 불렀는데, 이제 입력에 관심 있는 컴포넌트
	// (UEnhancedInputComponent)만 스스로 등록해뒀으므로 그쪽에만 전달하면 됨.
	InputSubsystem->ProcessInput(RawInput);
}

void UEngine::UpdateGame()
{
	while (!SDL_TICKS_PASSED(SDL_GetTicks(), TickCount + 16));

	float DeltaTime = (SDL_GetTicks() - TickCount) / 1000.f;
	if (DeltaTime > 0.05f)
	{
		DeltaTime = 0.05f;
	}
	TickCount = SDL_GetTicks();

	// [GetDeltaSeconds() 저장소] 이 프레임의 게임플레이 틱 그룹들이 실제로 쓰는
	// 값(클램프까지 반영된 값)과 정확히 같은 걸 애니메이션 쪽도 봐야, 게임 로직
	// 진행 속도와 애니메이션 재생 속도가 서로 어긋나지 않음.
	CurrentDeltaSeconds = DeltaTime;

	TickManager->BeginFrame();

	IsUpdatingActors = true;

	// [틱 그룹 순서, 실제 UE 소스로 확인] 액터 기본 그룹(TG_PrePhysics)에서
	// 입력 소비 + 사용자 정의 로직을 처리. SetActorLocation/Rotation은 즉시
	// 전파되므로 이 시점에 트랜스폼도 바로 확정됨.
	// [프레임당 1회로 수정] 서브스텝(SubDelta)이 아니라 전체 DeltaTime으로 딱 한 번만 -
	// 입력/게임플레이 로직은 물리처럼 잘게 쪼갤 이유가 없고(오히려 쪼개면 발소리 타이머 등
	// "프레임당 1회"를 전제한 로직이 여러 번 불려서 깨짐), 서브스테핑은 물리 시뮬레이션
	// (터널링 방지)에만 필요한 것이었음.
	TickManager->RunTickGroup(ETickingGroup::TG_PrePhysics, DeltaTime);

	// 컴포넌트 기본 그룹(TG_DuringPhysics_ComponentTick) - 일반 컴포넌트 틱. 이것도
	// 프레임당 1회. FPSActor처럼 자기 컴포넌트(MoveComp) 다음에 실행돼야 하는 액터는
	// PrimaryActorTick.AddPrerequisite(...)로 걸어뒀으므로, 그룹 순서상
	// 먼저 도는 이 시점에 그 프리퀴짓들이 자연스럽게 해소됨.
	TickManager->RunTickGroup(ETickingGroup::TG_DuringPhysics_ComponentTick, DeltaTime);

	// 서브스테핑: DeltaTime이 크면(프레임이 순간적으로 늘어지면) 여러 개의 작은 스텝으로
	// 쪼개서, 한 스텝당 이동 거리를 짧게 유지함. 정교한 전역 TOI 정렬 시스템을 새로 만드는
	// 대신, UE가 실제로 기대는 전략(터널링 확률 자체를 줄이는 것)을 그대로 따라감 - CCD는
	// 그 위에 얹는 "그래도 뚫으려는 특정 무버만 잡아내는" 보조 안전장치(UMovementComponent::bUseCCD).
	const float MaxSubstep = 1.0f / 60.0f;
	int NumSubsteps = static_cast<int>(Math::Ceil(DeltaTime / MaxSubstep));
	if (NumSubsteps < 1)
	{
		NumSubsteps = 1;
	}
	float SubDelta = DeltaTime / static_cast<float>(NumSubsteps);

	for (int step = 0; step < NumSubsteps; step++)
	{
		// [TG_DuringPhysics_Physics] 1) 트랜스폼 트리 업데이트 -> 2) 콜리전/물리 순.
		// 이번 서브스텝의 물리 쿼리(SegmentCast, TestPairwise, SweepQuery 등)들이 일관된
		// 스냅샷을 보도록, 콜리전 처리 "전"에 BVH를 최신 등록 목록으로 다시 빌드해둠.
		PhysScene->Update();
		// [역학 제거] 예전엔 여기서 ResolveDynamicCollisions(운동량+각운동량 보존, 마찰)를
		// 먼저 불렀는데, 회전/충격량 기반 역학을 통째로 뺀 지금은 필요 없음 -
		// StepPhysicsBodies 하나가 중력 적분 + CCD + 겹침 해소(MoveActor) + 단순 선형
		// 반발까지 다 담당함(순수 콜리전 + 최소한의 튕김만). MoveActor가 SetActorLocation을
		// 거치므로 즉시 전파로 트랜스폼도 같이 확정됨.
		PhysScene->Step(SubDelta);
	}

	// [TG_PostPhysics - 이번에 처음 실제로 채워짐] 물리 서브스텝이 전부 끝나서
	// "최종" 위치가 확정된 직후. Epic 공식 문서(중국어판 확인): "此步骤开始时物理
	// 模拟已完成...此组可用于武器或运动追踪...激光束必须从枪的最终位置发出，即便
	// 出现一帧延迟也会十分明显"(무기/조준선처럼 최종 위치가 꼭 필요한 로직에 씀,
	// 한 프레임이라도 늦으면 눈에 띔) - 정확히 USkeletalMeshComponent가
	// URenderingManager::UpdateTransform을 부르기에 맞는 시점. 이 그룹도 프레임당 1회.
	TickManager->RunTickGroup(ETickingGroup::TG_PostPhysics, DeltaTime);

	// [카메라 갱신 + TG_PostUpdateWork] 카메라는 이 그룹 소속으로 등록해뒀음
	// (UCameraComponent 생성자 참고) - Epic 공식 문서: "TG_PostUpdateWork happens
	// after cameras are updated"라는 순서를, 카메라를 이 그룹 안에 두는 것으로 근사함.
	// 이것도 서브스텝이 아니라 프레임당 1회.
	TickManager->RunTickGroup(ETickingGroup::TG_PostUpdateWork, DeltaTime);

	IsUpdatingActors = false;

	for (auto pending : PendingActors)
	{
		pending->ComputeWorldTransform();
		Actors.emplace_back(pending);
	}
	PendingActors.clear();

	// 이번 프레임의 모든 서브스텝이 끝난 뒤 딱 한 번만 - AddForce가 서브스텝 도중이 아니라
	// 프레임에 한 번만 불려도(예: 입력 처리) 모든 서브스텝에 걸쳐 그 힘이 온전히 반영된
	// 뒤에 리셋되도록 보장함.
	ResetAccumulatedForceThisTick();

	std::vector<AActor*> deadActors;

	for (auto actor : Actors)
	{
		if (actor->GetIsPendingKill())
		{
			deadActors.emplace_back(actor);
		}
	}

	for (auto actor : deadActors)
	{
		delete actor;
	}
}

void UEngine::GenerateOutput()
{
	if (RenderingManager->ScheduleRendering(CurrentDeltaSeconds))
	{
		RenderingManager->FlushAndDispatch();
		RenderingManager->Draw();
	}
}

void UEngine::LoadData()
{
#if WITH_DEV_AUTOMATION_TESTS
	// 데모/테스트 콘텐츠는 전부 FTestManager로 격리돼 있음 (Test/FTestManager.h 참고).
	// [수정] LoadTestLevel이 이제 void - "누가 플레이어인지" 판단과 감시 등록
	// (DebugManager::Watch)까지 전부 FTestManager 내부에서 끝냄.
	UTestManager::LoadTestLevel();
#endif
}

void UEngine::UnloadData()
{
	while (!Actors.empty())
	{
		delete Actors.back();
	}
}

void UEngine::ResetAccumulatedForceThisTick()
{
	for (auto actor : Actors)
	{
		for (auto prim : actor->GetPrimitiveComponents())
		{
			prim->BodyInstance.AccumulatedForce = FVector3::Zero;
		}
	}
}

void UEngine::RunLoop()
{
	while (IsRunning)
	{
		ProcessInput();
		UpdateGame();
		GenerateOutput();
	}
}

void UEngine::Shutdown()
{
}

UEngine* UEngine::GetEngine()
{
	if (Instance == nullptr)
	{
		Instance = new UEngine();
	}

	return Instance;
}

FPhysScene* UEngine::GetPhysScene()
{
	if (PhysScene == nullptr)
	{
		PhysScene = new FPhysScene();
	}

	return PhysScene;
}

UTickManager* UEngine::GetTickManager()
{
	if (TickManager == nullptr)
	{
		TickManager = new UTickManager();
	}

	return TickManager;
}

UInputSubsystem* UEngine::GetInputSubsystem()
{
	if (InputSubsystem == nullptr)
	{
		InputSubsystem = new UInputSubsystem();
	}

	return InputSubsystem;
}

URenderingManager* UEngine::GetRenderingManager()
{
	if (RenderingManager == nullptr)
	{
		RenderingManager = new URenderingManager();
	}

	return RenderingManager;
}

void UEngine::AddActor(AActor* InActor)
{
	if (IsUpdatingActors)
	{
		PendingActors.emplace_back(InActor);
	}
	else
	{
		Actors.emplace_back(InActor);
	}
}

void UEngine::RemoveActor(AActor* InActor)
{
	auto iter = std::find(PendingActors.begin(), PendingActors.end(), InActor);
	if (iter != PendingActors.end())
	{
		std::iter_swap(iter, PendingActors.end() - 1);
		PendingActors.pop_back();
	}

	iter = std::find(Actors.begin(), Actors.end(), InActor);
	if (iter != Actors.end())
	{
		std::iter_swap(iter, Actors.end() - 1);
		Actors.pop_back();
	}
}