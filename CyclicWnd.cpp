/*
	CyclicWnd TVTestプラグイン
*/

#include <windows.h>
#include <tchar.h>

#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"
#include "CommMgr.h"
#include "Debug.h"
#include "resource.h"


// プラグインのコマンド
#define COMMAND_MOVE_CYCLIC			 1
#define COMMAND_MOVE_CYCLIC_REVERSE  2

// プラグインクラス
class CCyclicWnd : public TVTest::CTVTestPlugin
{
	bool m_fInitialized;				// 初期化済みか?
	bool m_fEnabled;					// プラグインが有効か?
	CCommMgr m_CommMgr;

	bool InitializePlugin();
	bool OnEnablePlugin(bool fEnable);
	static LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);

public:
	CCyclicWnd();
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();
	BOOL	FeedHeadClientList(std::list<cyclicWndInfo_t*>& list);
	void	RemoveIconicClientList(std::list<cyclicWndInfo_t*>& list);
	void	OnMoveCyclic();
	void	OnMoveCyclicReverse();

private:
	void	MoveWindowCyclic(bool fReverse);

};


CCyclicWnd::CCyclicWnd()
	: m_fInitialized(false)
	, m_fEnabled(false)
{
}

// プラグインの情報を返す
bool CCyclicWnd::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"CyclicWnd";
	pInfo->pszCopyright   = L"ori";
	pInfo->pszDescription = L"サイクリックにTVTestのウィンドウを移動";
	return true;
}

// 初期化処理
bool CCyclicWnd::Initialize()
{
	// コマンドを登録
	TVTest::HostInfo Host;
	if (m_pApp->GetHostInfo(&Host)
			&& Host.SupportedPluginVersion >= TVTEST_PLUGIN_VERSION_(0,0,14)) {

		// 「サイクリックにTVTestのウィンドウを移動」の登録
		TVTest::PluginCommandInfo CommandInfo;
		CommandInfo.Size           = sizeof(CommandInfo);
		CommandInfo.Flags          = TVTest::PLUGIN_COMMAND_FLAG_ICONIZE;
		CommandInfo.State          = 0;
		CommandInfo.ID             = COMMAND_MOVE_CYCLIC;
		CommandInfo.pszText        = L"MoveCyclic";
		CommandInfo.pszName        = L"サイクリックにTVTestのウィンドウを移動";
		CommandInfo.pszDescription = L"サイクリックにTVTestのウィンドウを移動します";
		CommandInfo.hbmIcon = (HBITMAP)::LoadImage(g_hinstDLL, MAKEINTRESOURCE(IDB_ICON),
													IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
		m_pApp->RegisterPluginCommand(&CommandInfo);

		::DeleteObject(CommandInfo.hbmIcon);
		CommandInfo.hbmIcon = NULL;

		// 「サイクリックにTVTestのウィンドウを移動(逆順)」の登録
		CommandInfo.Size = sizeof(CommandInfo);
		CommandInfo.Flags = TVTest::PLUGIN_COMMAND_FLAG_ICONIZE;
		CommandInfo.State = 0;
		CommandInfo.ID = COMMAND_MOVE_CYCLIC_REVERSE;
		CommandInfo.pszText = L"MoveCyclicReverse";
		CommandInfo.pszName = L"サイクリックにTVTestのウィンドウを移動(逆順)";
		CommandInfo.pszDescription = L"サイクリックにTVTestのウィンドウを移動します(逆順)";
		CommandInfo.hbmIcon = (HBITMAP)::LoadImage(g_hinstDLL, MAKEINTRESOURCE(IDB_ICONREV),
													IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
		m_pApp->RegisterPluginCommand(&CommandInfo);

		::DeleteObject(CommandInfo.hbmIcon);
		CommandInfo.hbmIcon = NULL;
	}
	else
	{
		m_pApp->RegisterCommand(COMMAND_MOVE_CYCLIC, L"MoveCyclic", L"サイクリックにTVTestのウィンドウを移動");
		m_pApp->RegisterCommand(COMMAND_MOVE_CYCLIC_REVERSE, L"MoveCyclic", L"サイクリックにTVTestのウィンドウを移動(逆順)");
	}

	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback,this);

	m_CommMgr.Initialize();

	m_fInitialized = true;
	return true;
}

// 終了処理
bool CCyclicWnd::Finalize()
{
	m_CommMgr.Finalize();
	m_fInitialized = false;

	return true;
}


// プラグインの有効/無効が切り替わった時の処理
bool CCyclicWnd::OnEnablePlugin(bool fEnable)
{
	if (m_fEnabled != fEnable)
	{
		if (fEnable)
		{
			m_CommMgr.JoinService(m_pApp->GetAppWindow());
		}
		else
		{
			m_CommMgr.LeaveService();
		}

		m_fEnabled = fEnable;
	}

	m_pApp->SetPluginCommandState(COMMAND_MOVE_CYCLIC,
								  m_fEnabled?0:TVTest::PLUGIN_COMMAND_STATE_DISABLED);
	m_pApp->SetPluginCommandState(COMMAND_MOVE_CYCLIC_REVERSE,
								  m_fEnabled ? 0 : TVTest::PLUGIN_COMMAND_STATE_DISABLED);

	return true;
}

// 自Windowをリストの先頭に持ってくる
BOOL CCyclicWnd::FeedHeadClientList(std::list<cyclicWndInfo_t*>& list)
{
	BOOL	bFound	= FALSE;
	DWORD	dwSize	= 0;
	DWORD	dwProcessId = 0;
	
	dwProcessId = GetCurrentProcessId();

	dwSize = list.size();
	while (dwSize > 0)
	{
		cyclicWndInfo_t* pInfo = NULL;

		pInfo = list.front();
		if (pInfo->dwProcessID == dwProcessId)
		{
			bFound = TRUE;
			break;
		}
		list.pop_front();
		list.push_back(pInfo);
		dwSize--;
	}

	return bFound;
}

// 最小化しているWindowはリストから取り除く
void CCyclicWnd::RemoveIconicClientList(std::list<cyclicWndInfo_t*>& list)
{
	std::list<cyclicWndInfo_t*> nonIconicList;
	nonIconicList.clear();

	while(list.size() > 0)
	{
		cyclicWndInfo_t* pInfo = NULL;

		pInfo = list.front();
		list.pop_front();

		if (!IsIconic(pInfo->hWnd))
		{
			nonIconicList.push_back(pInfo);
		}
		else
		{
			delete pInfo;
		}
	}

	list = nonIconicList;
}

void CCyclicWnd::MoveWindowCyclic(bool fReverse)
{
	std::list<cyclicWndInfo_t*> list;
	BOOL bRet = FALSE;

	bRet = m_CommMgr.GetClientList(list);
	if (bRet)
	{
		// 最小化しているウィンドウは移動対象から除く
		RemoveIconicClientList(list);

		if (list.size() > 1)
		{
			// 逆順に移動する場合、リスト自体を逆順にしておく
			if (fReverse)
			{
				list.reverse();
			}

			// 自Windowをリストの先頭に持ってきておく
			bRet = FeedHeadClientList(list);
			if (bRet)
			{
				RECT	rc;
				RECT	rc_bak;
				std::list<cyclicWndInfo_t*>::iterator itr;
				cyclicWndInfo_t* pInfo = NULL;

				pInfo = list.back();
				GetWindowRect(pInfo->hWnd, &rc);

				for (itr = list.begin(); itr != list.end(); ++itr)
				{
					pInfo = *itr;
					GetWindowRect(pInfo->hWnd, &rc_bak);
					MoveWindow(pInfo->hWnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);

					DBGMSG((TEXT("CCyclicWnd::MoveCyclic: pid=%ld, before top=%d,left=%d,bottom=%d,right=%d, after top=%d,left=%d,bottom=%d,right=%d\n"),
								pInfo->dwProcessID,
								rc_bak.top,
								rc_bak.left,
								rc_bak.bottom,
								rc_bak.right,
								rc.top,
								rc.left,
								rc.bottom,
								rc.right									
								));
					memcpy(&rc, &rc_bak, sizeof(RECT));
				}

				itr = list.begin();
				itr++;

				SetForegroundWindow((*itr)->hWnd);
			}
		}

		m_CommMgr.FreeClientList(list);
	}
}

void CCyclicWnd::OnMoveCyclic()
{
	MoveWindowCyclic(false);
}

void CCyclicWnd::OnMoveCyclicReverse()
{
	MoveWindowCyclic(true);
}

// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CCyclicWnd::EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
{
	CCyclicWnd *pThis=static_cast<CCyclicWnd*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// プラグインの有効状態が変化した
		return pThis->OnEnablePlugin(lParam1!=0);

	case TVTest::EVENT_COMMAND:
		// コマンドが実行された
		if (lParam1 == COMMAND_MOVE_CYCLIC)
		{
			pThis->OnMoveCyclic();
		}
		else if (lParam1 == COMMAND_MOVE_CYCLIC_REVERSE)
		{
			pThis->OnMoveCyclicReverse();
		}
		return TRUE;
	}

	return 0;
}


// プラグインクラスのインスタンスを生成する
TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CCyclicWnd;
}
