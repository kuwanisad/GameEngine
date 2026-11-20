#pragma once
#include "AnimSnapshotBase.h"

// [예전 설계 논의에서 확정된 매크로, 이번에 실제 파일로 정식화]
// UAnimInstance 파생 클래스가 이 매크로 하나로: (1) 게임플레이 전용 필드 선언,
// (2) 그 필드와 똑같은 구성의 Snapshot 구조체, 두 가지를 한 번에 만든다.
//
// 사용 예:
//   class UFPSCharacterAnimInstance : public UAnimInstance
//   {
//   public:
//       GENERATED_AnimInstance(UFPSCharacterAnimInstance,
//           float VelocityX;
//           float VelocityY;
//       )
//       // GetSnapshot()의 실제 본문(필드별 복사)은 매크로 밖, 개발자가 직접 작성
//   };
//
// [필드 목록을 두 곳에 재사용하는 원리] Fields(세미콜론으로 구분된 일반 C++
// 선언문 나열)를 "멤버 선언 자리"와 "Snapshot 구조체 안"에 그대로 두 번
// 붙여넣음 - 매크로 인자 안에 세미콜론이 몇 개든 매크로 자신은 신경 안 쓰므로
// (콤마만 인자 구분자로 취급) 문제없이 동작함이 검증됨.
//
// [자동화 안 한 것] GetSnapshot()의 본문(필드별 복사 코드)은 자동 생성하지
// 않음 - 그러려면 FIELD(Type,Name) 형태의 X-매크로가 필요한데, 그 정도까지는
// 지금 범위 밖으로 확정했었음. 대신 GetSnapshot()의 "선언"만 여기서 만들어두고
// (반환 타입이 이 파생 클래스 전용 Snapshot 포인터, 공변 반환 타입으로 성립),
// 본문은 개발자가 매크로 밖에서 직접 작성함.
#define GENERATED_AnimInstance(ClassName, Fields) \
	Fields \
	struct ClassName##_Snapshot : public FAnimSnapshotBase \
	{ \
		Fields \
	}; \
	ClassName##_Snapshot* GetSnapshot() const override;
