#include "CommMgr.h"
#include "Lock.h"
#include "Debug.h"

/* Defines */
#define CYCLICWND_COMMMGR_MUTEX	TEXT("CyclicWndCommMgrMutex")
#define CYCLICWND_COMMMGR_MMF	TEXT("CyclicWndCommMgrMMF")
#define CYCLICWND_MAX_WND_COUNT	32
#define	CYCLICWND_MAX_MMF_SIZE	(sizeof(DWORD)+sizeof(cyclicWndInfo_t)*CYCLICWND_MAX_WND_COUNT)

/* Typedefs */
typedef struct {
	DWORD dwCount;
	cyclicWndInfo_t info[1];
} cyclicWndMMF_t;


CCommMgr::CCommMgr(void)
				: m_bInitialized(FALSE)
				, m_bJoined(FALSE)
				, m_hMutex(NULL)
				, m_hMMF(NULL)
				, m_pData(NULL)
{
	DBGMSG((TEXT("CCommMgr::CCommMgr: In\n")));
	DBGMSG((TEXT("CCommMgr::CCommMgr: Out\n")));
}


CCommMgr::~CCommMgr(void)
{
	DBGMSG((TEXT("CCommMgr::~CCommMgr: In\n")));
	DBGMSG((TEXT("CCommMgr::~CCommMgr: Out\n")));
}

BOOL CCommMgr::Initialize(void)
{
	BOOL	bRet = FALSE;
	HANDLE	hMutex = NULL;

	DBGMSG((TEXT("CCommMgr::Initialize: In\n")));
	if (!m_bInitialized)
	{
		hMutex = CreateMutex(NULL, FALSE, CYCLICWND_COMMMGR_MUTEX);
		if (hMutex != NULL)
		{
			m_hMutex = hMutex;
			m_bInitialized = TRUE;

			bRet = TRUE;
		}
	}
	else
	{
		DBGMSG((TEXT("CCommMgr::Initialize: Already initialized\n")));
	}

	DBGMSG((TEXT("CCommMgr::Initialize: Out,rc=%d\n"), bRet));
	return bRet;
}

BOOL CCommMgr::Finalize(void)
{
	BOOL bRet = FALSE;
	DBGMSG((TEXT("CCommMgr::Finalize: In\n")));

	if (m_bInitialized)
	{
		LeaveService();

		if (m_hMutex != NULL)
		{
			CloseHandle(m_hMutex);
			m_hMutex = NULL;

			bRet = TRUE;
		}

		m_bInitialized = FALSE;
	}
	else
	{
		DBGMSG((TEXT("CCommMgr::Finalize: not initialized\n")));
	}

	DBGMSG((TEXT("CCommMgr::Finalize: Out,rc=%d\n"), bRet));
	return bRet;
}

BOOL CCommMgr::OpenMMF(HANDLE* phMMF, LPBYTE* ppMMF, BOOL* pbAlready)
{
	BOOL	bRet		= FALSE;
	BOOL	bAlready	= FALSE;
	HANDLE	hFile		= NULL;

	DBGMSG((TEXT("CCommMgr::OpenMMF: In\n")));

	hFile = CreateFileMapping(	INVALID_HANDLE_VALUE,
								NULL,
								PAGE_READWRITE,
								0,
								CYCLICWND_MAX_MMF_SIZE,
								CYCLICWND_COMMMGR_MMF);
	if (hFile != NULL)
	{
		LPBYTE pData = NULL;

		bAlready = (GetLastError() == ERROR_ALREADY_EXISTS);

		pData = (LPBYTE)MapViewOfFile(hFile, FILE_MAP_ALL_ACCESS, 0, 0, CYCLICWND_MAX_MMF_SIZE);
		if (pData != NULL)
		{
			*phMMF = hFile;
			*ppMMF = pData;
			*pbAlready = bAlready;

			bRet = TRUE;
		}
		else
		{
			DBGMSG((TEXT("CCommMgr::OpenMMF: CreateFileMapping fail\n")));
			CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;
		}
	}

	DBGMSG((TEXT("CCommMgr::OpenMMF: Out,rc=%d\n"), bRet));
	return bRet;
}

BOOL CCommMgr::CloseMMF(HANDLE hMMF, LPBYTE pMMF)
{
	BOOL bRetUnmap = FALSE;
	BOOL bRetClose = FALSE;
	BOOL bRet = FALSE;

	DBGMSG((TEXT("CCommMgr::CloseMMF: In\n")));

	if (pMMF != NULL)
	{
		bRetUnmap = UnmapViewOfFile(pMMF);
	}
	if (hMMF != NULL)
	{
		bRetClose = CloseHandle(hMMF);
	}

	bRet = (bRetUnmap && bRetClose);

	DBGMSG((TEXT("CCommMgr::CloseMMF: Out,rc=%d\n"), bRet));
	return bRet;
}


BOOL CCommMgr::JoinService(HWND hWnd)
{
	BOOL	bRet	= FALSE;

	DBGMSG((TEXT("CCommMgr::JoinService: In\n")));

	if (m_bInitialized)
	{
		if (!m_bJoined)
		{
			BOOL	bAlready	= FALSE;
			HANDLE	hMMF	= NULL;
			LPBYTE	pData	= NULL;

			CLock lock(m_hMutex);

			bRet = OpenMMF(&hMMF, &pData, &bAlready);
			if (bRet)
			{
				DWORD dwIndex = 0;
				cyclicWndMMF_t* pMMF = (cyclicWndMMF_t*)pData;

				if (!bAlready)
				{
					DBGMSG((TEXT("CCommMgr::JoinService: first, dwCount set to zero\n")));
					pMMF->dwCount = 0;
				}
				if (pMMF->dwCount < CYCLICWND_MAX_WND_COUNT)
				{
					dwIndex = pMMF->dwCount;
					pMMF->info[dwIndex].dwProcessID = GetCurrentProcessId();
					pMMF->info[dwIndex].hWnd = hWnd;

					DBGMSG((TEXT("CCommMgr::JoinService: dwIndex=%d, dwProcessId=%ld, hWnd=%08x\n"),
							dwIndex, pMMF->info[dwIndex].dwProcessID, pMMF->info[dwIndex].hWnd));

					pMMF->dwCount++;

					DBGMSG((TEXT("CCommMgr::JoinService: dwIndex=%d, dwProcessId=%ld, hWnd=%08x\n")));

					m_hMMF	= hMMF;
					m_pData = pData;
					m_bJoined = TRUE;

					bRet = TRUE;
				}
				else
				{
					DBGMSG((TEXT("CCommMgr::JoinService: dwCount overflow\n")));
					CloseMMF(hMMF, pData);
				}
			}
			else
			{
				DBGMSG((TEXT("CCommMgr::JoinService: OpenMMF fail\n")));
			}
		}
		else
		{
			DBGMSG((TEXT("CCommMgr::JoinService: Already joined\n")));
		}
	}
	else
	{
			DBGMSG((TEXT("CCommMgr::JoinService: not initialized\n")));
	}

	DBGMSG((TEXT("CCommMgr::JoinService: Out,rc=%d\n"), bRet));
	return bRet;
}

BOOL CCommMgr::LeaveService()
{
	BOOL bRet = FALSE;
	DBGMSG((TEXT("CCommMgr::LeaveService: In\n")));

	if (m_bInitialized)
	{
		if (m_bJoined)
		{
			std::list<cyclicWndInfo_t*> list;

			CLock lock(m_hMutex);

			bRet = GetClientListInternal(list);
			if (bRet)
			{
				std::list<cyclicWndInfo_t*>::iterator itr;
				DWORD dwProcessID = GetCurrentProcessId();

				for (itr = list.begin(); itr != list.end();)
				{
					cyclicWndInfo_t* pInfo = NULL;

					pInfo = *itr;
					if (pInfo->dwProcessID == dwProcessID)
					{
						delete pInfo;
						itr = list.erase(itr);
						continue;
					}
					itr++;
				}

				int	nIndex = 0;
				cyclicWndMMF_t* pMMF = (cyclicWndMMF_t*)m_pData;

				pMMF->dwCount = list.size();
				for (itr = list.begin(); itr != list.end();itr++, nIndex++)
				{
					cyclicWndInfo_t* pInfo = NULL;

					pInfo = *itr;
					pMMF->info[nIndex].dwProcessID = pInfo->dwProcessID;
					pMMF->info[nIndex].hWnd = pInfo->hWnd;
				}

				FreeClientList(list);
			}

			m_bJoined = FALSE;
		}
		else
		{
			DBGMSG((TEXT("CCommMgr::LeaveService: not joined\n")));
		}
	}
	else
	{
		DBGMSG((TEXT("CCommMgr::LeaveService: not initialized\n")));
	}

	DBGMSG((TEXT("CCommMgr::LeaveService: Out,rc=%d\n"), bRet));
	return bRet;
}

BOOL CCommMgr::GetClientListInternal(std::list<cyclicWndInfo_t*>& list)
{
	BOOL			bRet = FALSE;
	cyclicWndMMF_t* pMMF = (cyclicWndMMF_t*)m_pData;

	DBGMSG((TEXT("CCommMgr::GetClientListInternal: In, dwCount=%d\n"), list.size()));

	list.clear();

	if (pMMF != NULL)
	{
		for (DWORD i = 0; i < pMMF->dwCount; i++)
		{
			cyclicWndInfo_t* pInfo = NULL;

			pInfo = new cyclicWndInfo_t;
			pInfo->dwProcessID = pMMF->info[i].dwProcessID;
			pInfo->hWnd = pMMF->info[i].hWnd;

			list.push_back(pInfo);
		}

		bRet = TRUE;
	}

	DBGMSG((TEXT("CCommMgr::GetClientListInternal: Out, rc=%d\n"), bRet));
	return bRet;
}

BOOL CCommMgr::GetClientList(std::list<cyclicWndInfo_t*>& list)
{
	BOOL bRet = FALSE;
	DBGMSG((TEXT("CCommMgr::GetClientList: In\n")));

	if (m_bInitialized)
	{
		CLock lock(m_hMutex);

		bRet = GetClientListInternal(list);
	}
	else
	{
		DBGMSG((TEXT("CCommMgr::GetClientList: not initialized\n")));
	}


	DBGMSG((TEXT("CCommMgr::GetClientList: Out, dwCount=%d\n"), list.size()));
	return bRet;
}

void CCommMgr::FreeClientList(std::list<cyclicWndInfo_t*>& list)
{
	std::list<cyclicWndInfo_t*>::iterator itr;

	DBGMSG((TEXT("CCommMgr::FreeClientList: In, dwCount=%d\n"), list.size()));
	
	for (itr = list.begin(); itr != list.end(); ++itr)
	{
		cyclicWndInfo_t* pInfo = *itr;

		delete pInfo;
	}
	list.clear();

	DBGMSG((TEXT("CCommMgr::FreeClientList: Out, dwCount=%d\n"), list.size()));
}