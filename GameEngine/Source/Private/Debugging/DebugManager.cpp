#include "../../Public/Debugging/DebugManager.h"

#include <SDL.h>
#include <cstdarg>
#include <cstdio>


void DebugManager::Log(const char* InfoName, const char* Fmt, ...)
{
	char buf[512];
	va_list args;
	va_start(args, Fmt);
	vsnprintf(buf, sizeof(buf), Fmt, args);
	va_end(args);

	SDL_Log("%s: %s", InfoName, buf);
}
