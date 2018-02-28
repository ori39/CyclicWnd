#pragma once

#if (_DEBUG || _DEBUGMSG)
#include <windows.h>
#include <tchar.h>
#include <stdarg.h>

void DebugMsg(LPTSTR pFmt, ...);

#define DBGMSG(x) DebugMsg x

#else

#define DBGMSG(x)

#endif