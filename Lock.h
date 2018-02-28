#pragma once
#include <windows.h>

class CLock
{
public:
	CLock(HANDLE hMutex);
	virtual ~CLock(void);

private:
	HANDLE m_hMutex;
};

