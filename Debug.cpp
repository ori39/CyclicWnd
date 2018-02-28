#if ( _DEBUG || _DEBUGMSG)
#include "Debug.h"

void DebugMsg(LPTSTR pFmt, ...)
{
	TCHAR szBuf[1024];
	va_list list;

	DWORD dwError = GetLastError();

	va_start(list, pFmt);

	wvsprintf(szBuf, pFmt, list);
	OutputDebugString(szBuf);

	va_end(list);

	SetLastError(dwError);
}

#endif
