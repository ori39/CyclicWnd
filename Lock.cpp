#include "Lock.h"


CLock::CLock(HANDLE hMutex) : m_hMutex(NULL)
{
	if (hMutex != NULL)
	{
		DWORD dwObject;

		dwObject = WaitForSingleObject(hMutex, 5*1000);
		if (dwObject == WAIT_OBJECT_0)
		{
			m_hMutex = hMutex;
		}
	}
}


CLock::~CLock(void)
{
	if (m_hMutex != NULL)
	{
		ReleaseMutex(m_hMutex);
		m_hMutex = NULL;
	}
}
