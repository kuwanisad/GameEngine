#include "../../Public/Core/TestManager.h"

#if WITH_DEV_AUTOMATION_TESTS

#include <SDL_log.h>
#include <cmath>

#include "../../Public/Actor/Actor.h"
#include "../../Public/Core/DebugManager.h"
#include "../../Public/Test/FPSActor.h"
#include "../../Public/Test/ThirdPersonCharacter.h"
#include "../../Public/Core/Engine.h"
#include "../../Public/Test/PlaneActor.h"
#include "../../Public/Rendering/Renderer.h"
#include "../../Public/Component/SpriteComponent.h"
#include "../../Public/Component/StaticMeshComponent.h"
#include "../../Public/Asset/SkeletalMesh.h"
#include "../../Public/Asset/Skeleton.h"
#include "../../Public/Rendering/RenderingStatics.h"
#include "../../Public/Asset/FbxImporter.h"
#include "../../Public/Component/SkeletalMeshComponent.h"
#include "../../Public/Rendering/RenderingManager.h"
#include "../../Public/Utilities/AnimSequenceCache.h"
#include "../../Public/Utilities/BlendSpaceManifest.h"
#include "../../Public/Utilities/DelaunayTriangulation2D.h"
#include "../../Public/Animation/AnimSequence.h"
#include "../../Public/Animation/AnimGraph.h"
#include "../../Public/Animation/AnimNode_SequencePlayer.h"
#include "../../Public/Animation/AnimNode_BlendSpace2D.h"
#include "../../Public/Animation/AnimNode_StateMachine.h"
#include "../../Public/Test/FPSCharacterAnimInstance.h"
#include "../../Public/Test/PhysicsTestBox.h"
#include "../../Public/Asset/AssetManager.h"

FString UTestManager::DebugSkeletalMeshPath;
FString UTestManager::DebugAnimationPath;
bool UTestManager::bUseBlendSpace = false;
bool UTestManager::bUseStateMachine = false;

// =============================================================================
// [1) 게임 실행형] LoadTestLevel
// =============================================================================

void UTestManager::LoadTestLevel()
{
	UEngine* engine = UEngine::GetEngine();
	// Create actors
	AActor* a = engine->SpawnActor<AActor>();
	UStaticMeshComponent* mc = new UStaticMeshComponent(a);
	mc->SetMesh(UAssetManager::Get()->LoadMesh("Assets/Cube.gpmesh"));
	// [부착 계층] 콜리전 없이 메시 하나뿐이라, 그 메시 자체를 루트로 삼음.
	// [순서 중요] 아래 SetActorLocation/Scale/Rotation보다 반드시 먼저 와야 함 - 늦게
	// 부르면 그 시점의(곧 버려질) 기본 루트한테 값들이 적용되고, 새 루트로 안 넘어감.
	a->SetRootComponent(mc);
	a->SetActorLocation(FVector3(200.0f, 75.0f, 0.0f));
	a->SetActorScale(FVector3::One * 100.0f);
	FQuaternion q(FVector3::UnitY, -Math::PiOver2);
	q = FQuaternion::Concatenate(q, FQuaternion(FVector3::UnitZ, Math::Pi + Math::Pi / 4.0f));
	a->SetActorRotation(q);

	a = engine->SpawnActor<AActor>();
	mc = new UStaticMeshComponent(a);
	mc->SetMesh(UAssetManager::Get()->LoadMesh("Assets/Sphere.gpmesh"));
	a->SetRootComponent(mc);
	a->SetActorLocation(FVector3(200.0f, -75.0f, 0.0f));
	a->SetActorScale(FVector3::One * 3.0f);

	// Setup floor
	const float start = -1250.0f;
	const float size = 250.0f;

	// [렌더링용 - 콜리전 없음] 미관은 예전 그대로(작은 타일들이 겹쳐 보이는 텍스처
	// 타일링) - 물리에는 전혀 관여 안 하므로 겹쳐도 상관없음.
	for (int i = 0; i < 10; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			a = engine->SpawnActor<APlaneActor>(10.0f, /*bWithMesh*/ true, /*bWithCollision*/ false);
			a->SetActorLocation(FVector3(start + i * size, start + j * size, -100.0f));
		}
	}

	// Left/right walls (렌더링용)
	q = FQuaternion(FVector3::UnitX, Math::PiOver2);
	for (int i = 0; i < 10; i++)
	{
		a = engine->SpawnActor<APlaneActor>(10.0f, true, false);
		a->SetActorLocation(FVector3(start + i * size, start - size, 0.0f));
		a->SetActorRotation(q);

		a = engine->SpawnActor<APlaneActor>(10.0f, true, false);
		a->SetActorLocation(FVector3(start + i * size, -start + size, 0.0f));
		a->SetActorRotation(q);
	}

	q = FQuaternion::Concatenate(q, FQuaternion(FVector3::UnitZ, Math::PiOver2));
	// Forward/back walls (렌더링용)
	for (int i = 0; i < 10; i++)
	{
		a = engine->SpawnActor<APlaneActor>(10.0f, true, false);
		a->SetActorLocation(FVector3(start - size, start + i * size, 0.0f));
		a->SetActorRotation(q);

		a = engine->SpawnActor<APlaneActor>(10.0f, true, false);
		a->SetActorLocation(FVector3(-start + size, start + i * size, 0.0f));
		a->SetActorRotation(q);
	}

	// [콜리전용 - 메시 없음] 실제 물리 반응은 이 5개(바닥 1 + 벽 4)만 담당함 - 타일로
	// 안 쪼개져 있어서, 물체가 타일 경계를 넘나들며 접촉 정체성(영속 매니폴드)이 끊기는
	// 문제 자체가 없음. 원래 10x10/4x10 타일 그리드가 덮던 전체 범위(가장자리 타일의
	// 바깥쪽 끝까지)를 정확히 덮도록 스케일을 계산함.
	{
		const float gridCenter = start + 9.0f * size / 2.0f; // = -125.0f
		const float oldTileSize = 1000.0f; // 렌더링 타일 한 개 크기(스케일10 * 메시100)
		const float totalSpan = (start + 9.0f * size + oldTileSize * 0.5f) - (start - oldTileSize * 0.5f); // = 3250
		const float bigPlaneScale = (totalSpan / 100.0f) + 2.0f; // 여유 포함 약 34.5

		// 바닥
		a = engine->SpawnActor<APlaneActor>(bigPlaneScale, false, true);
		a->SetActorLocation(FVector3(gridCenter, gridCenter, -100.0f));

		// 좌/우 벽
		FQuaternion wallQ = FQuaternion(FVector3::UnitX, Math::PiOver2);
		a = engine->SpawnActor<APlaneActor>(bigPlaneScale, false, true);
		a->SetActorLocation(FVector3(gridCenter, start - size, 0.0f));
		a->SetActorRotation(wallQ);

		a = engine->SpawnActor<APlaneActor>(bigPlaneScale, false, true);
		a->SetActorLocation(FVector3(gridCenter, -start + size, 0.0f));
		a->SetActorRotation(wallQ);

		// 앞/뒤 벽
		wallQ = FQuaternion::Concatenate(wallQ, FQuaternion(FVector3::UnitZ, Math::PiOver2));
		a = engine->SpawnActor<APlaneActor>(bigPlaneScale, false, true);
		a->SetActorLocation(FVector3(start - size, gridCenter, 0.0f));
		a->SetActorRotation(wallQ);

		a = engine->SpawnActor<APlaneActor>(bigPlaneScale, false, true);
		a->SetActorLocation(FVector3(-start + size, gridCenter, 0.0f));
		a->SetActorRotation(wallQ);
	}

	// ==========================================================
	// [테스트용] 물리 시뮬레이션 확인용 낙하 박스 두 개.
	// restBox가 바닥 바로 위에 살짝 떠서 "받침대" 역할을 하고, fallBox가 그 위에서
	// 떨어져서 (1) 중력 낙하, (2) restBox와의 동적-동적 충돌(운동량 보존),
	// (3) 그 여파로 restBox가 바닥(정적)과 부딪혀 튕기는 것까지 한 번에 확인할 수 있게
	// 배치함. 확인 끝나면 이 블록은 지워도 됨.
	// ==========================================================
	{
		APhysicsTestBox* restBox = engine->SpawnActor<APhysicsTestBox>(
			FVector3(500.0f, 0.0f, -65.0f), // 바닥(z=-100) 위 5유닛 틈, 플레이어 스폰(0,0,0)에서 +X로 500유닛 떨어진 시야 안
			60.0f, 3.0f, 0.5f);

		APhysicsTestBox* fallBox = engine->SpawnActor<APhysicsTestBox>(
			FVector3(500.0f, 0.0f, 3000.0f), // restBox와 같은 X,Y, 한참 위에서 떨어뜨림
			60.0f, 5.0f, 0.6f);

		// [디버그] 이 둘만 지켜보고 싶을 때 - 코드 곳곳의 DebugManager::Log/LogPair 호출은
		// 이 감시 목록에 있는 액터가 관련된 경우에만 실제로 출력됨. 필요 없어지면 이 두 줄만
		// 지우거나 주석 처리하면 프로젝트 전체 로그가 조용해짐(호출부 코드는 안 건드려도 됨).
		DebugManager::Watch(restBox);
		DebugManager::Watch(fallBox);
	}

	// Setup lights
	auto AmbientLight = FVector3(0.2f, 0.2f, 0.2f);
	auto renderer = engine->GetRenderingManager()->GetRenderer();

	renderer->SetAmbientLight(AmbientLight);
	FDirectionalLight& dir = renderer->GetDirectionalLight();
	dir.Direction = FVector3(0.0f, -0.707f, -0.707f);
	dir.DiffuseColor = FVector3(0.78f, 0.88f, 1.0f);
	dir.SpecColor = FVector3(0.8f, 0.8f, 0.8f);

	// UI elements
	a = engine->SpawnActor<AActor>();
	a->SetActorLocation(FVector3(-350.0f, -350.0f, 0.0f));
	USpriteComponent* sc = new USpriteComponent(a);
	sc->SetTexture(UAssetManager::Get()->LoadTexture("Assets/HealthBar.png"));

	a = engine->SpawnActor<AActor>();
	a->SetActorLocation(FVector3(375.0f, -275.0f, 0.0f));
	a->SetActorScale(FVector3::One * 0.75f);
	sc = new USpriteComponent(a);
	sc->SetTexture(UAssetManager::Get()->LoadTexture("Assets/Radar.png"));

	SDL_SetRelativeMouseMode(SDL_TRUE);
	SDL_GetRelativeMouseState(nullptr, nullptr);

	// Camera actor
	//Player = new CameraActor(game);

	// --loadskm으로 경로가 설정됐으면 스켈레탈 메시 액터를 스폰.
	// [bUseStateMachine이면 이 더미 테스트 액터는 건너뜀] ThirdPersonCharacter가
	// 자기 스켈레탈 메시를 직접 로드하므로, 여기서 또 별도 더미 캐릭터를 만들면
	// 화면에 캐릭터가 둘 생기는 중복이 됨.
	if (!DebugSkeletalMeshPath.empty() && !bUseStateMachine)
	{
		USkeletalMesh* SkelMesh = new USkeletalMesh();
		FAssetHandle SkelMeshHandle;
		FAssetHandle SkeletonHandle;

		if (UAssetManager::Get()->LoadSkeletalMesh(DebugSkeletalMeshPath, SkelMeshHandle, SkeletonHandle))
		{
			const USkeleton* Skel = SkelMesh->GetSkeleton();

			FAnimGraphHandle GraphHandle;

			// [--loadblendspace 최우선] 8방향 블렌드 스페이스 실전 테스트.
			if (bUseBlendSpace)
			{
				GraphHandle = RegisterBlendSpaceTestAnimGraph(SkeletonHandle, Skel);
			}
			else if (!DebugAnimationPath.empty())
			{
				FString CachePath = GetAnimCachePathForFbx(DebugAnimationPath);
				UAnimSequence* RealSeq = LoadAnimSequenceFromBinary(CachePath, Skel->GetReferenceSkeleton());

				if (RealSeq == nullptr && UFbxImporter::Get()->ImportFromFile(DebugAnimationPath))
				{
					// [캐시 없음 - FBX 파싱 후 캐시 저장] 반드시 SkelMesh->Load()가
					// 캐릭터 씬으로부터 필요한 걸 다 뽑아낸 뒤(스켈레톤/스킨/지오메트리
					// 임포트가 이미 끝난 뒤)에만 ImportFromFile을 호출해야 함 - 안
					// 그러면 캐릭터 씬이 사라짐.
					RealSeq = new UAnimSequence();
					RealSeq->SequenceName = "DebugAnim";

					if (UFbxImporter::Get()->ImportAnimation(Skel->GetReferenceSkeleton(), *RealSeq))
					{
						SaveAnimSequenceToBinary(*RealSeq, Skel->GetReferenceSkeleton(), CachePath);
					}
					else
					{
						delete RealSeq;
						RealSeq = nullptr;
					}
				}

				if (RealSeq != nullptr)
				{
					FAssetHandle SequenceHandle = UAssetManager::Get()->RegisterAnimSequence(RealSeq);
					GraphHandle = RegisterRealTestAnimGraph(SkeletonHandle, SequenceHandle);
				}
				else
				{
					GraphHandle = RegisterEmptyTestAnimGraph(SkeletonHandle);
				}
			}
			else
			{
				// [테스트 전용 그래프] SequencePlayer 하나가 고의로 빈 시퀀스를 재생 -
				// 본 트랙이 없으니 매 본이 스켈레톤 바인드포즈로 폴백돼야 정상.
				GraphHandle = RegisterEmptyTestAnimGraph(SkeletonHandle);
			}

			AActor* SkelActor = engine->SpawnActor<AActor>();
			USkeletalMeshComponent* smc = new USkeletalMeshComponent(SkelActor);

			UAnimInstance* AnimInst = nullptr;
			if (bUseBlendSpace)
			{
				AnimInst = new UFPSCharacterAnimInstance();
			}
			else
			{
				AnimInst = new UTestAnimInstance();
			}
			AnimInst->SkeletonHandle = SkeletonHandle;
			AnimInst->SkeletalMeshHandle = SkelMeshHandle;
			AnimInst->AnimGraphHandle = GraphHandle;
			smc->SetAnimInstance(AnimInst);

			SkelActor->SetRootComponent(smc);
			SkelActor->SetActorLocation(FVector3(300.0f, 0.0f, 0.0f));
			SkelActor->SetActorScale(FVector3::One * 1.0f);
		}
		else
		{
			delete SkelMesh;
		}
	}

	// [플레이어 스폰 - 누구를 플레이어로 삼을지는 여기서만 결정] bUseStateMachine이면
	// ThirdPersonCharacter(진짜 WASD 입력으로 Idle<->Locomotion 전이 확인용),
	// 아니면 예전처럼 FPSActor. (이 파일 전체가 이미 WITH_DEV_AUTOMATION_TESTS로
	// 감싸여 있어서 여기서 또 감쌀 필요 없음)
	if (bUseStateMachine)
	{
		AThirdPersonCharacter* Player = engine->SpawnActor<AThirdPersonCharacter>(DebugSkeletalMeshPath);
		DebugManager::Watch(Player);
	}
	else
	{
		AFPSActor* Player = engine->SpawnActor<AFPSActor>();
		DebugManager::Watch(Player);
	}
}

// =============================================================================
// [1) 내부 헬퍼 - 그래프 조립]
// =============================================================================

UAnimSequence* UTestManager::CreateEmptyTestAnimSequence()
{
	UAnimSequence* Seq = new UAnimSequence();
	Seq->SequenceName = "TestEmpty_DeliberatelyBlank";
	Seq->PlayLength = 0.0f;
	Seq->BoneTracks.clear();   // 고의로 비움 - "아직 안 채워서 우연히"가 아니라 "의도"임을 명시
	return Seq;
}

namespace
{
	// [순수 함수 포인터 제약 우회] FAnimGraphFactoryFn이 캡처 불가능한 순수 함수
	// 포인터라서, 등록 직전에 여기 미리 세팅해두고 빌더 함수가 읽어가는 방식으로
	// 우회함. 이 핸들 전역들은 파일 범위(익명 네임스페이스)에 그대로 둬도, 같은
	// 번역 단위 안의 UTestManager:: 정적 멤버 함수들에서 평범하게 접근 가능함
	// (private 멤버 접근 제약과는 무관한, 그냥 일반적인 C++ 스코프 규칙).
	FAssetHandle GTestSkeletonHandle = 0;
	FAssetHandle GTestEmptySequenceHandle = 0;

	FAssetHandle GRealSkeletonHandle = 0;
	FAssetHandle GRealSequenceHandle = 0;

	FAssetHandle GBlendSpaceSkeletonHandle = 0;
	const USkeleton* GBlendSpaceSkeletonRef = nullptr;

	FAssetHandle GCharacterSkeletonHandle = 0;
	const USkeleton* GCharacterSkeletonRef = nullptr;
}

// [private static 멤버로 정의 - 예전엔 익명 네임스페이스의 평범한 함수였음]
// static 멤버 함수의 주소는 this 포인터가 없어서 일반 함수 포인터와 정확히 같은
// 타입이라, FAnimGraphFactoryFn(순수 함수 포인터)으로 그대로 등록 가능하면서도
// private을 유지할 수 있음 - 이 덕분에 ApplyBlendSpaceEntrySettings 같은 다른
// private 멤버를 자유롭게 호출할 수 있게 됨.
IAnimGraphBase* UTestManager::BuildEmptyTestAnimGraph()
{
	auto* Graph = new TAnimGraph<FAnimSnapshotBase>();

	auto* Player = new FAnimNode_SequencePlayer<FAnimSnapshotBase>();
	Player->SequenceHandle = GTestEmptySequenceHandle;
	Player->SkeletonHandle = GTestSkeletonHandle;
	Player->bLoop = false;

	Graph->RegisterNode(Player);
	Graph->SetOutPose(Player);

	return Graph;
}

IAnimGraphBase* UTestManager::BuildRealTestAnimGraph()
{
	auto* Graph = new TAnimGraph<FAnimSnapshotBase>();

	auto* Player = new FAnimNode_SequencePlayer<FAnimSnapshotBase>();
	Player->SequenceHandle = GRealSequenceHandle;
	Player->SkeletonHandle = GRealSkeletonHandle;
	Player->bLoop = true;

	Graph->RegisterNode(Player);
	Graph->SetOutPose(Player);

	return Graph;
}

IAnimGraphBase* UTestManager::BuildBlendSpaceTestAnimGraph()
{
	using FSnapshot = UFPSCharacterAnimInstance::UFPSCharacterAnimInstance_Snapshot;

	auto* Graph = new TAnimGraph<FSnapshot>();

	// [8방향 + Idle 임포트] 캐시 우선.
	TArray<FBlendSpaceManifestEntry> Entries = ImportAndRegisterManifest(
		"Assets/Animation/RunBlendSpace.manifest", GBlendSpaceSkeletonRef->GetReferenceSkeleton());

	// [에셋 로드 후, 첫 틱 전 설정] 그래프가 실제로 재생을 시작하기(첫 틱) 전인
	// 이 지점에서, UI로 했을 설정(bForceRootLock 등)을 코드로 몰아서 적용. 이제
	// 이 함수 자신이 UTestManager의 멤버라서, ApplyBlendSpaceEntrySettings(같은
	// 클래스의 private 멤버)를 정규화 없이 바로 부를 수 있음.
	ApplyBlendSpaceEntrySettings(Entries);

	auto* BlendNode = new FAnimNode_BlendSpace2D<FSnapshot>();
	BlendNode->Samples = BuildBlendSpaceSamplesFromManifest<FSnapshot>(Entries, GBlendSpaceSkeletonHandle);

	BlendNode->GetBlendInput = [](const FSnapshot& Snapshot)
		{
			// ============================================================
			// [진단 전용 - 임시, 확인 끝나면 지우고 원래 한 줄로 되돌릴 것]
			// ============================================================
			// 실제 입력(Snapshot.VelocityX/Y)을 무시하고, 시간에 따라 부드럽게
			// 각도가 바뀌는 값을 강제로 반환함 - WASD 디지털 입력은 항상 정확히
			// 샘플 위치(꼭짓점) 위에만 떨어져서, 두 샘플 사이 "애매한 각도"의
			// 진짜 블렌딩(if(bFirst)의 else 분기)이 한 번도 발동 안 했던 문제를
			// 확인하기 위함. 원래 코드로 되돌리려면 이 블록 전체를
			// "return Vector2(Snapshot.VelocityX, Snapshot.VelocityY);" 한 줄로
			// 바꾸면 됨.
			static float DebugSweepTime = 0.0f;
			// [비고] 이 람다는 DeltaSeconds를 못 받아서(GetBlendInput 시그니처가
			// Snapshot만 받음) 실제 경과 시간을 정확히 알 수 없음 - 60fps
			// 기준으로 대략적인 값을 씀(순수 진단용이라 정밀도는 중요하지 않음).
			DebugSweepTime += 1.0f / 60.0f;

			const float AngularSpeed = 0.3f;   // 초당 약 0.3라디안 - 천천히 회전
			const float Radius = 200.0f;       // 매니페스트 WalkSpeed와 동일 - 샘플 원 위를 그대로 훑음
			float Angle = DebugSweepTime * AngularSpeed;
			return FVector2(std::cos(Angle) * Radius, std::sin(Angle) * Radius);
		};

	for (auto& Sample : BlendNode->Samples)
	{
		Graph->RegisterNode(Sample.ChildNode);
	}
	Graph->RegisterNode(BlendNode);
	Graph->SetOutPose(BlendNode);

	BlendNode->BuildTriangulation();

	return Graph;
}

FAnimGraphHandle UTestManager::RegisterEmptyTestAnimGraph(FAssetHandle SkeletonHandle)
{
	GTestSkeletonHandle = SkeletonHandle;
	GTestEmptySequenceHandle = UAssetManager::Get()->RegisterAnimSequence(CreateEmptyTestAnimSequence());
	return UAnimGraphRegistry::Get().RegisterFactory(&UTestManager::BuildEmptyTestAnimGraph);
}

FAnimGraphHandle UTestManager::RegisterRealTestAnimGraph(FAssetHandle SkeletonHandle, FAssetHandle SequenceHandle)
{
	GRealSkeletonHandle = SkeletonHandle;
	GRealSequenceHandle = SequenceHandle;
	return UAnimGraphRegistry::Get().RegisterFactory(&UTestManager::BuildRealTestAnimGraph);
}

FAnimGraphHandle UTestManager::RegisterBlendSpaceTestAnimGraph(FAssetHandle SkeletonHandle, const USkeleton* SkeletonRef)
{
	GBlendSpaceSkeletonHandle = SkeletonHandle;
	GBlendSpaceSkeletonRef = SkeletonRef;
	return UAnimGraphRegistry::Get().RegisterFactory(&UTestManager::BuildBlendSpaceTestAnimGraph);
}

namespace
{
	// [Jump/Fall Loop/Land 개별 임포트 헬퍼] 매니페스트 없이 하나씩 캐시 우선 임포트.
	// 실패해도 빈 시퀀스(바인드포즈 폴백)로 대체해서 그래프 조립 자체는 안 깨지게 함.
	UAnimSequence* ImportSingleSequence(const FString& FbxPath, const UReferenceSkeleton& Skel)
	{
		FString CachePath = GetAnimCachePathForFbx(FbxPath);
		UAnimSequence* Seq = LoadAnimSequenceFromBinary(CachePath, Skel);

		if (Seq == nullptr && UFbxImporter::Get()->ImportFromFile(FbxPath))
		{
			Seq = new UAnimSequence();
			Seq->SequenceName = FbxPath;

			if (UFbxImporter::Get()->ImportAnimation(Skel, *Seq))
			{
				SaveAnimSequenceToBinary(*Seq, Skel, CachePath);
			}
			else
			{
				delete Seq;
				Seq = nullptr;
			}
		}

		if (Seq == nullptr)
		{
			SDL_Log("BuildCharacterAnimGraph - failed to import '%s', falling back to empty sequence", FbxPath.c_str());
			Seq = new UAnimSequence();   // 빈 시퀀스 - 바인드포즈 폴백
		}

		return Seq;
	}
}

IAnimGraphBase* UTestManager::BuildCharacterAnimGraph()
{
	using FSnapshot = UFPSCharacterAnimInstance::UFPSCharacterAnimInstance_Snapshot;

	auto* Graph = new TAnimGraph<FSnapshot>();
	const UReferenceSkeleton& RefSkel = GCharacterSkeletonRef->GetReferenceSkeleton();

	// ── Locomotion (기존 로직 그대로) ──────────────────────────
	TArray<FBlendSpaceManifestEntry> Entries = ImportAndRegisterManifest(
		"Assets/Animation/RunBlendSpace.manifest", RefSkel);
	ApplyBlendSpaceEntrySettings(Entries);

	auto* LocomotionNode = new FAnimNode_BlendSpace2D<FSnapshot>();
	LocomotionNode->Samples = BuildBlendSpaceSamplesFromManifest<FSnapshot>(Entries, GCharacterSkeletonHandle);
	LocomotionNode->GetBlendInput = [](const FSnapshot& Snapshot)
		{
			return FVector2(Snapshot.VelocityX, Snapshot.VelocityY);
		};
	for (auto& Sample : LocomotionNode->Samples)
	{
		Graph->RegisterNode(Sample.ChildNode);
	}
	Graph->RegisterNode(LocomotionNode);
	LocomotionNode->BuildTriangulation();

	// ── Jump / Fall Loop / Land - 개별 임포트 ──────────────────
	UAnimSequence* JumpSeq = ImportSingleSequence("Assets/Animation/MM_Jump.FBX", RefSkel);
	UAnimSequence* FallLoopSeq = ImportSingleSequence("Assets/Animation/MM_Fall_Loop.FBX", RefSkel);
	UAnimSequence* LandSeq = ImportSingleSequence("Assets/Animation/MM_Land.FBX", RefSkel);

	FAssetHandle JumpHandle = UAssetManager::Get()->RegisterAnimSequence(JumpSeq);
	FAssetHandle FallLoopHandle = UAssetManager::Get()->RegisterAnimSequence(FallLoopSeq);
	FAssetHandle LandHandle = UAssetManager::Get()->RegisterAnimSequence(LandSeq);

	auto* JumpNode = new FAnimNode_SequencePlayer<FSnapshot>();
	JumpNode->SequenceHandle = JumpHandle;
	JumpNode->SkeletonHandle = GCharacterSkeletonHandle;
	JumpNode->bLoop = false;   // 한 번 재생하고 끝 - IsFinished()/GetPlayRatio()가 의미 있으려면 필수
	Graph->RegisterNode(JumpNode);

	auto* FallLoopNode = new FAnimNode_SequencePlayer<FSnapshot>();
	FallLoopNode->SequenceHandle = FallLoopHandle;
	FallLoopNode->SkeletonHandle = GCharacterSkeletonHandle;
	FallLoopNode->bLoop = true;   // 이름 그대로 - 착지할 때까지 반복
	Graph->RegisterNode(FallLoopNode);

	auto* LandNode = new FAnimNode_SequencePlayer<FSnapshot>();
	LandNode->SequenceHandle = LandHandle;
	LandNode->SkeletonHandle = GCharacterSkeletonHandle;
	LandNode->bLoop = false;   // "자동 규칙"(재생 비율 >= 1.0) 판정이 성립하려면 필수
	Graph->RegisterNode(LandNode);

	// ── 스테이트 머신 조립 ──────────────────────────────────────
	auto* StateMachine = new FAnimNode_StateMachine<FSnapshot>();

	StateMachine->States = {
		{ "Locomotion", LocomotionNode },   // 0
		{ "Land",       LandNode },          // 1
		{ "Jump",       JumpNode },          // 2
		{ "Fall Loop",  FallLoopNode },      // 3
	};

	// [별칭 - Init() 전에 인덱스로 직접 채워야 함, 이름 기반 자동 해석은 Transitions만 지원]
	FAnimStateAlias<FSnapshot> ToFallingAlias;
	ToFallingAlias.Name = "To Falling";
	ToFallingAlias.MemberStateIndices = { 0, 1 };   // Locomotion, Land

	FAnimStateAlias<FSnapshot> ToLandAlias;
	ToLandAlias.Name = "To Land";
	ToLandAlias.MemberStateIndices = { 2, 3 };   // Jump, Fall Loop

	StateMachine->Aliases = { ToFallingAlias, ToLandAlias };

	// [전이] 이름으로만 적음 - Init()이 실제 인덱스로 해석
	FAnimTransition<FSnapshot> ToJump;
	ToJump.FromName = "To Falling";
	ToJump.ToName = "Jump";
	ToJump.SetCondition([](const FSnapshot& S) { return !S.bIsOnGround && S.VelocityZ > 0.0f; });

	FAnimTransition<FSnapshot> ToFallLoop;
	ToFallLoop.FromName = "To Falling";
	ToFallLoop.ToName = "Fall Loop";
	ToFallLoop.SetCondition([](const FSnapshot& S) { return !S.bIsOnGround && S.VelocityZ <= 0.0f; });

	FAnimTransition<FSnapshot> ToLand;
	ToLand.FromName = "To Land";
	ToLand.ToName = "Land";
	ToLand.SetCondition([](const FSnapshot& S) { return S.bIsOnGround; });

	// [Land -> Locomotion, 2개] 언리얼 확인 결과 그대로: 하나는 재생이 자연스럽게
	// 끝나면(자동 규칙), 다른 하나는 재생 도중이라도 이동을 시작하면 끼어듦.
	FAnimTransition<FSnapshot> LandFinished;
	LandFinished.FromName = "Land";
	LandFinished.ToName = "Locomotion";
	LandFinished.bAutomaticRuleBasedOnSequencePlayerInState = true;

	FAnimTransition<FSnapshot> LandInterrupted;
	LandInterrupted.FromName = "Land";
	LandInterrupted.ToName = "Locomotion";
	LandInterrupted.SetCondition([](const FSnapshot& S) { return S.VelocityX != 0.0f || S.VelocityY != 0.0f; });

	// [발견된 누락 - 수정] Jump 자신에서 Fall Loop로 가는 전이가 아예 없었음 -
	// 그래서 Jump가 다 재생된 뒤(아직 착지 전이라도) 착지할 때까지 그 자리에
	// 얼어붙어 있다가, 착지하는 순간 Fall Loop를 완전히 건너뛰고 바로 Land로
	// 가버렸음(확인된 증상: "굳어있는 자세"가 사실 Fall Loop가 아니라 Jump의
	// 마지막 프레임이었음). Jump의 재생이 자연스럽게 끝나면(자동 규칙) Fall
	// Loop로 넘어가게 함 - "To Land" 별칭(Jump/Fall Loop 둘 다 -> Land)은 그대로
	// 둬서, 아주 짧은 점프처럼 Jump가 다 끝나기 전에 착지하는 경우는 여전히
	// Jump에서 바로 Land로 갈 수 있음(두 경로가 공존).
	FAnimTransition<FSnapshot> JumpFinishedToFallLoop;
	JumpFinishedToFallLoop.FromName = "Jump";
	JumpFinishedToFallLoop.ToName = "Fall Loop";
	JumpFinishedToFallLoop.bAutomaticRuleBasedOnSequencePlayerInState = true;

	StateMachine->Transitions = { ToJump, ToFallLoop, ToLand, LandFinished, LandInterrupted, JumpFinishedToFallLoop };

	StateMachine->Init();

	Graph->RegisterNode(StateMachine);
	Graph->SetOutPose(StateMachine);

	return Graph;
}

FAnimGraphHandle UTestManager::RegisterCharacterAnimGraph(FAssetHandle SkeletonHandle, const USkeleton* SkeletonRef)
{
	GCharacterSkeletonHandle = SkeletonHandle;
	GCharacterSkeletonRef = SkeletonRef;
	return UAnimGraphRegistry::Get().RegisterFactory(&UTestManager::BuildCharacterAnimGraph);
}


void UTestManager::ApplyBlendSpaceEntrySettings(const TArray<FBlendSpaceManifestEntry>& Entries)
{
	// [루트 모션 억제 - "Idle" 제외 전부] 걷기 계열 8개 클립은 캡처 당시 실제로
	// 이동한 거리가 담겨있어서(확인된 증상: 캐릭터가 화면 위를 미끄러져 이동함),
	// 재생 시 루트 이동을 지워야 함. Idle은 애초에 제자리 애니메이션이라 켤 필요 없음.
	for (const FBlendSpaceManifestEntry& Entry : Entries)
	{
		if (!Entry.bImportSucceeded || Entry.Name == "Idle")
		{
			continue;
		}

		UAnimSequence* Seq = UAssetManager::Get()->GetAnimSequence(Entry.SequenceHandle);
		if (Seq != nullptr)
		{
			Seq->bForceRootLock = true;
		}
	}
}

// =============================================================================
// [2) 로그 전용] FBX 임포트 검증
// =============================================================================

void UTestManager::LogBoneHierarchy(const UReferenceSkeleton& Skeleton, int32 BoneIndex, int32 Depth)
{
	FString Indent(Depth * 2, ' ');
	SDL_Log("%s- %s", Indent.c_str(), Skeleton.GetRefBoneInfo(BoneIndex).Name.c_str());

	for (int32 ChildIndex = 0; ChildIndex < Skeleton.GetBoneCount(); ++ChildIndex)
	{
		if (Skeleton.GetParentIndex(ChildIndex) == BoneIndex)
		{
			LogBoneHierarchy((UReferenceSkeleton&)Skeleton, ChildIndex, Depth + 1);
		}
	}
}

bool UTestManager::RunFbxSkeletonTest(const FString& FbxFilePath)
{
	UFbxImporter* Importer = UFbxImporter::Get();

	if (!Importer->ImportFromFile(FbxFilePath))
	{
		SDL_Log("[UTestManager::RunFbxSkeletonTest] failed to open '%s'", FbxFilePath.c_str());
		return false;
	}

	UReferenceSkeleton Skeleton;
	if (!Importer->ImportSkeleton(Skeleton))
	{
		SDL_Log("[UTestManager::RunFbxSkeletonTest] failed to import skeleton from '%s'", FbxFilePath.c_str());
		return false;
	}

	SDL_Log("[UTestManager::RunFbxSkeletonTest] bone hierarchy (%d bones):", Skeleton.GetBoneCount());
	for (int32 BoneIndex = 0; BoneIndex < Skeleton.GetBoneCount(); ++BoneIndex)
	{
		if (Skeleton.GetParentIndex(BoneIndex) == INDEX_NONE)
		{
			LogBoneHierarchy(Skeleton, BoneIndex, 0);
		}
	}

	return true;
}

bool UTestManager::RunFbxMeshTest(const FString& FbxFilePath)
{
	UFbxImporter* Importer = UFbxImporter::Get();

	if (!Importer->ImportFromFile(FbxFilePath))
	{
		SDL_Log("[UTestManager::RunFbxMeshTest] failed to open '%s'", FbxFilePath.c_str());
		return false;
	}

	// 본 인덱스 매핑에 필요하므로 스켈레톤을 먼저 임포트
	UReferenceSkeleton Skeleton;
	if (!Importer->ImportSkeleton(Skeleton))
	{
		SDL_Log("[UTestManager::RunFbxMeshTest] failed to import skeleton");
		return false;
	}
	
	TArray<FBoneInfluence> RawInfluences;
	int32 VertexCount = 0;
	if (!Importer->ImportSkinWeights(Skeleton, RawInfluences, VertexCount))
	{
		SDL_Log("[UTestManager::RunFbxMeshTest] failed to import skin weights");
		return false;
	}

	if (VertexCount <= 0)
	{
		SDL_Log("[UTestManager::RunFbxMeshTest] vertex count is 0");
		return false;
	}

	TArray<FPackedSkinWeights> Packed;
	URenderingStatics::NormalizeAndPackInfluences(RawInfluences, VertexCount, Packed);

	FSkeletalMeshImportData ImportData;
	if (!Importer->ImportGeometry(ImportData))
	{
		SDL_Log("[UTestManager::RunFbxMeshTest] failed to import geometry");
		return false;
	}

	// 지오메트리 정합성 검증
	bool bGeometryValid = true;
	if (static_cast<int32>(ImportData.Points.size()) != VertexCount)
	{
		SDL_Log("[UTestManager::RunFbxMeshTest] Points count (%d) != skin weight vertex count (%d)",
			static_cast<int32>(ImportData.Points.size()), VertexCount);
		bGeometryValid = false;
	}

	int32 InvalidWedgeVertexCount = 0;
	for (const FVertex& Wedge : ImportData.Wedges)
	{
		if (Wedge.VertexIndex < 0 || Wedge.VertexIndex >= static_cast<int32>(ImportData.Points.size()))
		{
			++InvalidWedgeVertexCount;
		}
	}

	int32 InvalidFaceWedgeCount = 0;
	for (const FTriangle& Face : ImportData.Faces)
	{
		for (int32 Corner = 0; Corner < 3; ++Corner)
		{
			if (Face.WedgeIndex[Corner] < 0 || Face.WedgeIndex[Corner] >= static_cast<int32>(ImportData.Wedges.size()))
			{
				++InvalidFaceWedgeCount;
			}
		}
	}

	SDL_Log("[UTestManager::RunFbxMeshTest] geometry: %d points, %d wedges, %d faces, invalid wedge vertex=%d, invalid face wedge=%d",
		static_cast<int32>(ImportData.Points.size()), static_cast<int32>(ImportData.Wedges.size()), static_cast<int32>(ImportData.Faces.size()),
		InvalidWedgeVertexCount, InvalidFaceWedgeCount);

	if (InvalidWedgeVertexCount > 0 || InvalidFaceWedgeCount > 0)
	{
		bGeometryValid = false;
	}

	// ---- 스킨 웨이트 검증 ----
	bool bAllValid = true;
	int32 CheckedCount = 0;
	int32 InvalidBoneIndexCount = 0;
	int32 BadWeightSumCount = 0;

	for (int32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
	{
		const FPackedSkinWeights& Weights = Packed[VertexIndex];

		int32 WeightSum = 0;
		for (int32 i = 0; i < 4; ++i)
		{
			if (Weights.InfluenceBones[i] >= Skeleton.GetBoneCount() && Weights.InfluenceWeights[i] > 0)
			{
				++InvalidBoneIndexCount;
				bAllValid = false;
			}
			WeightSum += Weights.InfluenceWeights[i];
		}

		if (WeightSum != 255)
		{
			++BadWeightSumCount;
			bAllValid = false;
		}

		++CheckedCount;
	}

	SDL_Log("[UTestManager::RunFbxMeshTest] checked %d vertices: invalid bone index=%d, bad weight sum=%d",
		CheckedCount, InvalidBoneIndexCount, BadWeightSumCount);

	if (VertexCount > 0)
	{
		const FPackedSkinWeights& Sample = Packed[0];
		SDL_Log("[UTestManager::RunFbxMeshTest] sample vertex[0]: bones=(%d,%d,%d,%d) weights=(%d,%d,%d,%d)",
			Sample.InfluenceBones[0], Sample.InfluenceBones[1], Sample.InfluenceBones[2], Sample.InfluenceBones[3],
			Sample.InfluenceWeights[0], Sample.InfluenceWeights[1], Sample.InfluenceWeights[2], Sample.InfluenceWeights[3]);
	}

	// ---- 최종 조립 검증 ----
	FSkeletalMeshLODModel LODModel;
	if (!URenderingStatics::BuildSkeletalMesh(LODModel, RawInfluences, ImportData.Wedges, ImportData.Faces, ImportData.Points))
	{
		SDL_Log("[UTestManager::RunFbxMeshTest] BuildSkeletalMesh failed");
		return false;
	}

	bool bBuildValid = true;
	if (LODModel.Sections.empty())
	{
		SDL_Log("[UTestManager::RunFbxMeshTest] BuildSkeletalMesh produced no sections");
		bBuildValid = false;
	}
	else
	{
		const FSkelMeshSection& Section = LODModel.Sections[0];

		bool bIndicesMultipleOf3 = (Section.Indices.size() % 3 == 0);
		int32 OutOfRangeIndexCount = 0;
		for (uint32 Index : Section.Indices)
		{
			if (Index >= Section.SoftVertices.size())
			{
				++OutOfRangeIndexCount;
			}
		}

		SDL_Log("[UTestManager::RunFbxMeshTest] BuildSkeletalMesh: %d vertices, %d indices, %d triangles, indices%%3==0:%s, out-of-range indices=%d",
			static_cast<int32>(Section.SoftVertices.size()), static_cast<int32>(Section.Indices.size()), Section.TriangleCount,
			bIndicesMultipleOf3 ? "yes" : "no", OutOfRangeIndexCount);

		if (!bIndicesMultipleOf3 || OutOfRangeIndexCount > 0)
		{
			bBuildValid = false;
		}
	}

	return bAllValid && bGeometryValid && bBuildValid;
}

bool UTestManager::RunFbxAllTests(const FString& FbxFilePath)
{
	if (!RunFbxSkeletonTest(FbxFilePath))
	{
		SDL_Log("[UTestManager::RunFbxAllTests] stopped at skeleton test");
		return false;
	}
	if (!RunFbxMeshTest(FbxFilePath))
	{
		SDL_Log("[UTestManager::RunFbxAllTests] stopped at mesh test");
		return false;
	}
	return true;
}

// =============================================================================
// [2) 로그 전용] 블렌드 스페이스 단위 테스트
// =============================================================================

namespace
{
	// [테스트 전용 리프 노드] 항상 고정된 포즈(본 하나, 고정 트랜스폼)만 반환함 -
	// 블렌드 결과를 손으로 미리 계산해서 검증할 수 있게 하기 위함.
	class FTestConstantPoseNode : public UAnimNode_Base<FAnimSnapshotBase>
	{
	public:
		FBoneTransform ConstantTransform;

		void Update(float DeltaSeconds, const FAnimSnapshotBase& Snapshot) override {}

		void Evaluate(TArray<FBoneTransform>& OutPose, const FAnimSnapshotBase& Snapshot) override
		{
			OutPose.resize(1);
			OutPose[0] = ConstantTransform;
		}
	};

	bool NearlyEqual(float A, float B, float Epsilon = 0.001f)
	{
		return std::fabs(A - B) <= Epsilon;
	}
}

bool UTestManager::RunBlendSpaceUnitTest()
{
	bool bAllPassed = true;
	bAllPassed = TestTriangulation() && bAllPassed;
	bAllPassed = TestBarycentricWeights() && bAllPassed;
	bAllPassed = TestBlendSpaceNode() && bAllPassed;

	SDL_Log("UTestManager::RunBlendSpaceUnitTest - %s", bAllPassed ? "ALL PASSED" : "SOME FAILED");
	return bAllPassed;
}

bool UTestManager::TestTriangulation()
{
	TArray<FVector2> Points = {
		FVector2(0.0f, 0.0f),
		FVector2(1.0f, 0.0f),
		FVector2(1.0f, 1.0f),
		FVector2(0.0f, 1.0f),
		FVector2(0.5f, 0.5f),
	};

	TArray<FTriangleIndices> Triangles = ComputeDelaunayTriangulation2D(Points);

	bool bPassed = true;

	if (Triangles.size() != 4)
	{
		SDL_Log("UTestManager::TestTriangulation - FAIL: expected 4 triangles, got %d",
			static_cast<int32>(Triangles.size()));
		bPassed = false;
	}

	for (const FTriangleIndices& Tri : Triangles)
	{
		if (Tri.A < 0 || Tri.A >= 5 || Tri.B < 0 || Tri.B >= 5 || Tri.C < 0 || Tri.C >= 5)
		{
			SDL_Log("UTestManager::TestTriangulation - FAIL: triangle index out of range (%d,%d,%d)",
				Tri.A, Tri.B, Tri.C);
			bPassed = false;
		}
	}

	float TotalArea = 0.0f;
	for (const FTriangleIndices& Tri : Triangles)
	{
		const FVector2& A = Points[Tri.A];
		const FVector2& B = Points[Tri.B];
		const FVector2& C = Points[Tri.C];
		float Area = std::fabs((B.x - A.x) * (C.y - A.y) - (C.x - A.x) * (B.y - A.y)) * 0.5f;
		TotalArea += Area;
	}
	if (!NearlyEqual(TotalArea, 1.0f, 0.01f))
	{
		SDL_Log("UTestManager::TestTriangulation - FAIL: total area %.4f (expected 1.0)", TotalArea);
		bPassed = false;
	}

	TArray<FVector2> CollinearPoints = { FVector2(0.0f, 0.0f), FVector2(1.0f, 0.0f), FVector2(2.0f, 0.0f) };
	TArray<FTriangleIndices> CollinearResult = ComputeDelaunayTriangulation2D(CollinearPoints);
	if (!CollinearResult.empty())
	{
		SDL_Log("UTestManager::TestTriangulation - FAIL: collinear points produced %d triangles (expected 0)",
			static_cast<int32>(CollinearResult.size()));
		bPassed = false;
	}

	SDL_Log("UTestManager::TestTriangulation - %s", bPassed ? "PASS" : "FAIL");
	return bPassed;
}

bool UTestManager::TestBarycentricWeights()
{
	FVector2 A(0.0f, 0.0f), B(1.0f, 0.0f), C(0.0f, 1.0f);
	bool bPassed = true;
	bool bValid = false;

	FBarycentricWeights W_A = ComputeBarycentricWeights(A, A, B, C, bValid);
	if (!bValid || !NearlyEqual(W_A.WeightA, 1.0f) || !NearlyEqual(W_A.WeightB, 0.0f) || !NearlyEqual(W_A.WeightC, 0.0f))
	{
		SDL_Log("UTestManager::TestBarycentricWeights - FAIL: point A -> (%.3f, %.3f, %.3f), expected (1,0,0)",
			W_A.WeightA, W_A.WeightB, W_A.WeightC);
		bPassed = false;
	}

	FVector2 Centroid((A.x + B.x + C.x) / 3.0f, (A.y + B.y + C.y) / 3.0f);
	FBarycentricWeights W_Centroid = ComputeBarycentricWeights(Centroid, A, B, C, bValid);
	if (!bValid || !NearlyEqual(W_Centroid.WeightA, 1.0f / 3.0f, 0.01f)
		|| !NearlyEqual(W_Centroid.WeightB, 1.0f / 3.0f, 0.01f)
		|| !NearlyEqual(W_Centroid.WeightC, 1.0f / 3.0f, 0.01f))
	{
		SDL_Log("UTestManager::TestBarycentricWeights - FAIL: centroid -> (%.3f, %.3f, %.3f), expected (1/3,1/3,1/3)",
			W_Centroid.WeightA, W_Centroid.WeightB, W_Centroid.WeightC);
		bPassed = false;
	}
	if (!IsPointInTriangle(W_Centroid))
	{
		SDL_Log("UTestManager::TestBarycentricWeights - FAIL: centroid should be inside triangle");
		bPassed = false;
	}

	FVector2 Outside(5.0f, 5.0f);
	FBarycentricWeights W_Outside = ComputeBarycentricWeights(Outside, A, B, C, bValid);
	if (bValid && IsPointInTriangle(W_Outside))
	{
		SDL_Log("UTestManager::TestBarycentricWeights - FAIL: point (5,5) should be outside triangle");
		bPassed = false;
	}

	SDL_Log("UTestManager::TestBarycentricWeights - %s", bPassed ? "PASS" : "FAIL");
	return bPassed;
}

bool UTestManager::TestBlendSpaceNode()
{
	FTestConstantPoseNode NodeA, NodeB, NodeC;
	NodeA.ConstantTransform = FBoneTransform(FQuaternion::Identity, FVector3(0.0f, 0.0f, 0.0f));
	NodeB.ConstantTransform = FBoneTransform(FQuaternion::Identity, FVector3(10.0f, 0.0f, 0.0f));
	NodeC.ConstantTransform = FBoneTransform(FQuaternion::Identity, FVector3(20.0f, 0.0f, 0.0f));

	FAnimNode_BlendSpace2D<FAnimSnapshotBase> BlendNode;
	BlendNode.Samples = {
		{ FVector2(0.0f, 0.0f), &NodeA },
		{ FVector2(10.0f, 0.0f), &NodeB },
		{ FVector2(0.0f, 10.0f), &NodeC },
	};
	BlendNode.BuildTriangulation();

	FVector2 TestInput((0.0f + 10.0f + 0.0f) / 3.0f, (0.0f + 0.0f + 10.0f) / 3.0f);
	BlendNode.GetBlendInput = [TestInput](const FAnimSnapshotBase& Snapshot) { return TestInput; };

	FAnimSnapshotBase DummySnapshot;
	BlendNode.Update(0.0f, DummySnapshot);

	TArray<FBoneTransform> OutPose;
	BlendNode.Evaluate(OutPose, DummySnapshot);

	bool bPassed = true;

	if (OutPose.size() != 1)
	{
		SDL_Log("UTestManager::TestBlendSpaceNode - FAIL: expected 1 bone in OutPose, got %d",
			static_cast<int32>(OutPose.size()));
		bPassed = false;
	}
	else if (!NearlyEqual(OutPose[0].Translation.x, 10.0f, 0.05f))
	{
		SDL_Log("UTestManager::TestBlendSpaceNode - FAIL: blended Translation.x = %.4f (expected 10.0)",
			OutPose[0].Translation.x);
		bPassed = false;
	}

	FVector2 FarInput(1000.0f, 1000.0f);
	BlendNode.GetBlendInput = [FarInput](const FAnimSnapshotBase& Snapshot) { return FarInput; };
	BlendNode.Update(0.0f, DummySnapshot);
	TArray<FBoneTransform> FarOutPose;
	BlendNode.Evaluate(FarOutPose, DummySnapshot);
	if (FarOutPose.empty())
	{
		SDL_Log("UTestManager::TestBlendSpaceNode - FAIL: fallback path produced empty pose");
		bPassed = false;
	}

	SDL_Log("UTestManager::TestBlendSpaceNode - %s", bPassed ? "PASS" : "FAIL");
	return bPassed;
}

// =============================================================================
// [UTestAnimInstance]
// =============================================================================

FAnimSnapshotBase* UTestAnimInstance::GetSnapshot() const
{
	return new FAnimSnapshotBase();
}

#endif // WITH_DEV_AUTOMATION_TESTS