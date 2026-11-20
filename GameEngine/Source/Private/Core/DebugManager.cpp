#include "../../Public/Core/DebugManager.h"
#include <SDL.h>
#include <cstdarg>
#include <cstdio>

TSet<const void*> DebugManager::WatchedTargets;

void DebugManager::Watch(const void* target)
{
	WatchedTargets.insert(target);
}

void DebugManager::Unwatch(const void* target)
{
	WatchedTargets.erase(target);
}

void DebugManager::ClearWatch()
{
	WatchedTargets.clear();
}

bool DebugManager::IsWatched(const void* target)
{
	return WatchedTargets.count(target) > 0;
}

void DebugManager::Log(const void* target, const char* fmt, ...)
{
	if (!IsWatched(target))
	{
		return;
	}

	char buf[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	SDL_Log("%s", buf);
}

void DebugManager::LogPair(const void* targetA, const void* targetB, const char* fmt, ...)
{
	if (!IsWatched(targetA) && !IsWatched(targetB))
	{
		return;
	}

	char buf[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	SDL_Log("%s", buf);
}
