#pragma once

#include <windows.h>
#include <tchar.h>
#include <list>

typedef struct
{
	DWORD	dwProcessID;
	HWND	hWnd;
	HWND	hLastExchangeWnd;
} cyclicWndInfo_t;

class CCommMgr
{
public:
	CCommMgr(void);
	virtual ~CCommMgr(void);

	BOOL Initialize(void);
	BOOL Finalize(void);

	BOOL JoinService(HWND hWnd);
	BOOL LeaveService();
	BOOL GetClientList(std::list<cyclicWndInfo_t*>& list);
	void FreeClientList(std::list<cyclicWndInfo_t*>& list);
	BOOL ExchangeClient(cyclicWndInfo_t* pSrcInfo, cyclicWndInfo_t* pDstInfo);


private:
	BOOL OpenMMF(HANDLE* phMMF, LPBYTE* ppMMF, BOOL* pbAlready);
	BOOL CloseMMF(HANDLE hMMF, LPBYTE pMMF);
	BOOL GetClientListInternal(std::list<cyclicWndInfo_t*>& list);

	BOOL	m_bInitialized;
	BOOL	m_bJoined;
	HANDLE	m_hMutex;
	HANDLE	m_hMMF;
	LPBYTE	m_pData;
};

