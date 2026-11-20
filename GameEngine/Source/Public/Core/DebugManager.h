#pragma once
#include "CoreMinimal.h"

// 디버그 로그를 매번 SDL_Log로 손수 넣었다 뺐다 하는 대신, "누구를 지켜볼지"만 여기
// 등록해두고, 로그 찍는 쪽(Log/LogPair)은 그 대상이 관련된 경우에만 실제로 출력하게 함 -
// 지금까지 반복했던 "로그 심었다가, 확인 끝나면 지웠다가, 다음 버그에서 또 심는" 패턴을
// 없애는 게 목적. 실제 프로젝트를 껐다 켜도 유지될 필요는 없어서(디버그 세션 한정) 그냥
// 정적 멤버로 둠.
class DebugManager
{
public:
	// 이 포인터(액터, 바디 등 - 타입 안 가리고 그냥 식별자로 씀)를 감시 목록에 추가.
	static void Watch(const void* target);
	static void Unwatch(const void* target);
	static void ClearWatch();
	static bool IsWatched(const void* target);

	// target이 감시 대상이면 SDL_Log로 실제 출력하고, 아니면 조용히 무시함 -
	// 호출부는 매번 "감시 중인지" 직접 안 물어보고 그냥 이 함수만 부르면 됨.
	static void Log(const void* target, const char* fmt, ...);

	// 관련된 대상이 둘일 때(예: 두 액터의 충돌) - 둘 중 하나라도 감시 대상이면 출력.
	static void LogPair(const void* targetA, const void* targetB, const char* fmt, ...);

private:
	static TSet<const void*> WatchedTargets;
};
