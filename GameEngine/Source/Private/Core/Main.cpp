#include "../../Public/Core/CoreMinimal.h"
#include "../../Public/Core/Engine.h"
#include <SDL_log.h>

#if WITH_DEV_AUTOMATION_TESTS
#include "../../Public/Core/TestManager.h"
#endif

int main(int argc, char** argv)
{
#if WITH_DEV_AUTOMATION_TESTS
	// 테스트 훅: 실행 인자로 FBX 파일 경로를 주면 게임을 켜지 않고 임포트 결과만 확인하고 종료한다.
	// 사용법:
	//   Practice.exe --fbxtest skeleton "Assets/CharacterA.fbx"
	//   Practice.exe --fbxtest mesh     "Assets/CharacterA.fbx"
	//   Practice.exe --fbxtest all      "Assets/CharacterA.fbx"
	// [정리됨] anim 서브커맨드는 제거 - ImportAnimation()이 --loadblendspace 화면
	// 테스트로 충분히 검증됐다고 판단됨.
	if (argc >= 4 && FString(argv[1]) == "--fbxtest")
	{
		FString SubCommand = argv[2];
		FString FbxFilePath = argv[3];

		bool bSuccess = false;
		if (SubCommand == "skeleton")
		{
			bSuccess = UTestManager::RunFbxSkeletonTest(FbxFilePath);
		}
		else if (SubCommand == "mesh")
		{
			bSuccess = UTestManager::RunFbxMeshTest(FbxFilePath);
		}
		else if (SubCommand == "all")
		{
			bSuccess = UTestManager::RunFbxAllTests(FbxFilePath);
		}
		else
		{
			SDL_Log("Unknown --fbxtest subcommand: %s (use skeleton|mesh|all)", SubCommand.c_str());
		}

		return bSuccess ? 0 : 1;
	}
#endif

#if WITH_DEV_AUTOMATION_TESTS
	// 디버그 전용: --loadskm 경로.fbx 로 실행하면 실제 게임을 켜면서 그 자리에
	// 스켈레탈 메시 액터를 하나 스폰한다 (Test/FTestManager.cpp 참고). 다른 --fbxtest
	// 서브커맨드와 달리 이건 게임을 끝까지 정상 실행한다 - 화면으로 직접 확인하는 용도.
	// 사용법: Practice.exe --loadskm "Assets/CharacterA.fbx"
	if (argc >= 3 && FString(argv[1]) == "--loadskm")
	{
		UTestManager::SetDebugSkeletalMeshPath(argv[2]);
	}

	// --loadanim 경로.fbx - 애니메이션 전용 FBX(예: MM_Idle.fbx, 스킨 없음).
	// --loadskm과 같이 줘야 의미가 있음(스켈레탈 메시 액터가 있어야 그 위에 재생할
	// 그래프가 생기므로) - 있으면 실제 애니메이션으로, 없으면 예전처럼 빈 테스트
	// 그래프(바인드포즈)로 감.
	// [고정 위치가 아니라 전체 순회] --loadskm이 항상 argv[1]에 있다는 전제를
	// --loadanim에까지 강요하지 않기 위해, 두 플래그가 어느 순서로 오든 찾음.
	// 사용법: Practice.exe --loadskm "Assets/CharacterA.fbx" --loadanim "Assets/Animation/MM_Idle.fbx"
	for (int i = 1; i < argc - 1; ++i)
	{
		if (FString(argv[i]) == "--loadanim")
		{
			UTestManager::SetDebugAnimationPath(argv[i + 1]);
			break;
		}
	}

	// --loadblendspace - 값 없는 플래그. --loadskm과 같이 줘야 의미가 있음.
	// 켜져 있으면 --loadanim보다 우선해서, 8방향 블렌드 스페이스(Assets/Animation/
	// RunBlendSpace.manifest)를 재생함.
	// 사용법: Practice.exe --loadskm "Assets/CharacterA.fbx" --loadblendspace
	for (int i = 1; i < argc; ++i)
	{
		if (FString(argv[i]) == "--loadblendspace")
		{
			UTestManager::SetUseBlendSpace(true);
			break;
		}
	}

	// --loadstatemachine - 값 없는 플래그. --loadskm과 같이 줘야 의미가 있음.
	// 켜져 있으면 --loadblendspace보다 우선해서, ThirdPersonCharacter를 진짜
	// 플레이어로 스폰함 - 실제 WASD 입력으로 Idle<->Locomotion 전이가 일어나는지
	// 확인하는 용도(--loadblendspace는 더미 액터+시간 기반 스윕이라 진짜 입력이 아님).
	// 사용법: Practice.exe --loadskm "Assets/CharacterA.fbx" --loadstatemachine
	for (int i = 1; i < argc; ++i)
	{
		if (FString(argv[i]) == "--loadstatemachine")
		{
			UTestManager::SetUseStateMachine(true);
			break;
		}
	}
#endif

#if WITH_DEV_AUTOMATION_TESTS
	// 테스트 훅: 블렌드 스페이스(삼각분할/무게중심 좌표/노드 블렌딩) 단위 테스트.
	// FBX/URenderingManager 파이프라인과 무관하게 그래프 노드만 떼어내서 검증함.
	// 사용법: Practice.exe --blendspacetest
	if (argc >= 2 && FString(argv[1]) == "--blendspacetest")
	{
		bool bSuccess = UTestManager::RunBlendSpaceUnitTest();
		return bSuccess ? 0 : 1;
	}
#endif

	UEngine game;
	bool success = game.Initialize();
	if (success)
	{
		game.RunLoop();
	}
	game.Shutdown();
	return 0;
}
