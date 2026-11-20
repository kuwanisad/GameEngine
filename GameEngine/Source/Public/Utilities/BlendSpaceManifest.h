#pragma once
#include "../Core/CoreMinimal.h"
#include "../Core/Math.h"
#include "../Asset/AssetHandle.h"

class UReferenceSkeleton;

// [묘수 - 28개를 한 번에] 27개 방향/속도별 Run + Idle 1개처럼, 애니메이션 클립이
// 많을 때 하나하나 손으로 등록 코드를 반복해서 쓰지 않도록, "텍스트 목록 파일 하나 +
// 함수 호출 한 번"으로 전부 임포트/캐싱/등록까지 끝내는 배치 파이프라인.
//
// 매니페스트 파일 형식(한 줄에 하나, 쉼표 구분, '#'로 시작하는 줄은 주석):
//   이름, X좌표, Y좌표, FBX경로
// 예:
//   Idle,    0,   0, Assets/Animation/MM_Idle.fbx
//   Run_F,   0, 300, Assets/Animation/MF_Fwd.fbx
//   Run_FR,  45, 300, Assets/Animation/MF_FwdRight.fbx
struct FBlendSpaceManifestEntry
{
	FString Name;
	FVector2 Position;
	FString FbxPath;

	// [ImportAndRegisterManifest가 채움] 핸들 0이 "정상적으로 발급된 진짜 핸들"일
	// 수 있어서(FAssetHandle은 0부터 발급됨), "실패"를 핸들 값 자체로 표시하면
	// 첫 번째로 성공한 항목과 헷갈릴 위험이 있음 - 그래서 별도 bool로 구분함.
	FAssetHandle SequenceHandle = 0;
	bool bImportSucceeded = false;
};

// 매니페스트 텍스트 파일만 읽어서 파싱함 - 임포트는 안 함(순수 파싱 단계 분리).
TArray<FBlendSpaceManifestEntry> ParseBlendSpaceManifest(const FString& ManifestPath);

// [실제 배치 작업] 매니페스트를 파싱한 뒤, 각 항목마다: 바이너리 캐시 우선 조회 ->
// 없으면 FBX 파싱 + 캐시 저장 -> URenderingManager에 정적 에셋 등록까지 처리하고,
// 결과(SequenceHandle/bImportSucceeded)를 채워서 돌려줌. TargetSkeleton은 모든
// 항목이 공유(같은 캐릭터의 서로 다른 방향/속도 클립들이므로).
TArray<FBlendSpaceManifestEntry> ImportAndRegisterManifest(const FString& ManifestPath, const UReferenceSkeleton& TargetSkeleton);
