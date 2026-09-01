#pragma once
#include "../Core/CoreMinimal.h.h"

// 디버그 로그를 매번 SDL_Log로 손수 넣었다 뺐다 하는 대신, "누구를 지켜볼지"만 여기
// 등록해두고, 로그 찍는 쪽(Log/LogPair)은 그 대상이 관련된 경우에만 실제로 출력하게 함 -
// 지금까지 반복했던 "로그 심었다가, 확인 끝나면 지웠다가, 다음 버그에서 또 심는" 패턴을
// 없애는 게 목적. 실제 프로젝트를 껐다 켜도 유지될 필요는 없어서(디버그 세션 한정) 그냥
// 정적 멤버로 둠.
class DebugManager
{
public:
	//정적 클래스
	DebugManager() = delete;
	~DebugManager() = delete;

	static void Log(const char* InfoName, const char* Fmt, ...);
};