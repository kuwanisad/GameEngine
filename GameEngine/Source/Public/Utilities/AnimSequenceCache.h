#pragma once
#include "../Core/CoreMinimal.h"

class UAnimSequence;
class UReferenceSkeleton;

// [경로 유도 - 하드코딩 방지] 원본 FBX 경로 하나만 알면, 캐시 파일 경로는 여기서
// 자동으로 유도함(같은 폴더/이름, 확장자만 .animcache로 교체) - 호출부가 캐시
// 경로를 별도 문자열로 하드코딩해서 FBX 경로와 따로 관리하지 않아도 되게.
FString GetAnimCachePathForFbx(const FString& FbxFilePath);

// [3번 과제 - 바이너리 캐싱] FBX를 매번 파싱하는 대신, 한 번 임포트한 UAnimSequence를
// 우리 자체 바이너리 포맷으로 구워두고 재사용함 - 스테이트 머신/블렌드 스페이스
// 개발 중 반복 테스트가 잦을 텐데, 그때마다 FBX 파싱(스킨 임포트까지 있으면 특히
// 무거움)을 다시 하지 않도록. FBX SDK 자체를 아예 안 거치는 순수 바이너리 I/O라,
// 이 두 함수만 쓸 땐 FbxImporter.h조차 include할 필요 없음.
//
// [본 매칭 - 이름 기준] ImportAnimation() 자체가 이름 매칭 방식이었던 것과 일관되게,
// 인덱스가 아니라 본 이름을 저장/복원함 - 나중에 스켈레톤 임포트 순서가 조금
// 달라져도 안전하게 다시 맞아떨어지도록.
bool SaveAnimSequenceToBinary(const UAnimSequence& Sequence, const UReferenceSkeleton& SourceSkeleton, const FString& FilePath);

// 캐시 파일이 없거나 손상됐으면 nullptr 반환(에러 아님) - 호출부가 "없으면 FBX로
// 다시 임포트한 뒤 저장"으로 폴백하면 됨.
UAnimSequence* LoadAnimSequenceFromBinary(const FString& FilePath, const UReferenceSkeleton& TargetSkeleton);
