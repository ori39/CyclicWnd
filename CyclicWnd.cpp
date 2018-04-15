/*
	CyclicWnd TVTest�v���O�C��
*/

#include <windows.h>
#include <tchar.h>

#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"
#include "CommMgr.h"
#include "Debug.h"
#include "resource.h"


// �v���O�C���̃R�}���h
#define COMMAND_MOVE_CYCLIC			 1
#define COMMAND_MOVE_CYCLIC_REVERSE  2
#define COMMAND_EXCHANGE_WINDOW      3

// �v���O�C���N���X
class CCyclicWnd : public TVTest::CTVTestPlugin
{
	bool m_fInitialized;				// �������ς݂�?
	bool m_fEnabled;					// �v���O�C�����L����?
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
	void	OnExchangeWindow();

private:
	void	MoveWindowCyclic(bool fReverse);

};


CCyclicWnd::CCyclicWnd()
	: m_fInitialized(false)
	, m_fEnabled(false)
{
}

// �v���O�C���̏���Ԃ�
bool CCyclicWnd::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"CyclicWnd";
	pInfo->pszCopyright   = L"ori";
	pInfo->pszDescription = L"�T�C�N���b�N��TVTest�̃E�B���h�E���ړ�";
	return true;
}

// ����������
bool CCyclicWnd::Initialize()
{
	// �R�}���h��o�^
	TVTest::HostInfo Host;
	if (m_pApp->GetHostInfo(&Host)
			&& Host.SupportedPluginVersion >= TVTEST_PLUGIN_VERSION_(0,0,14)) {

		// �u�T�C�N���b�N��TVTest�̃E�B���h�E���ړ��v�̓o�^
		TVTest::PluginCommandInfo CommandInfo;
		CommandInfo.Size           = sizeof(CommandInfo);
		CommandInfo.Flags          = TVTest::PLUGIN_COMMAND_FLAG_ICONIZE;
		CommandInfo.State          = TVTest::PLUGIN_COMMAND_STATE_DISABLED;
		CommandInfo.ID             = COMMAND_MOVE_CYCLIC;
		CommandInfo.pszText        = L"MoveCyclic";
		CommandInfo.pszName        = L"�T�C�N���b�N��TVTest�̃E�B���h�E���ړ�";
		CommandInfo.pszDescription = L"�T�C�N���b�N��TVTest�̃E�B���h�E���ړ����܂�";
		CommandInfo.hbmIcon = (HBITMAP)::LoadImage(g_hinstDLL, MAKEINTRESOURCE(IDB_ICON),
													IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
		m_pApp->RegisterPluginCommand(&CommandInfo);

		::DeleteObject(CommandInfo.hbmIcon);
		CommandInfo.hbmIcon = NULL;

		// �u�T�C�N���b�N��TVTest�̃E�B���h�E���ړ�(�t��)�v�̓o�^
		CommandInfo.Size = sizeof(CommandInfo);
		CommandInfo.Flags = TVTest::PLUGIN_COMMAND_FLAG_ICONIZE;
		CommandInfo.State = TVTest::PLUGIN_COMMAND_STATE_DISABLED;
		CommandInfo.ID = COMMAND_MOVE_CYCLIC_REVERSE;
		CommandInfo.pszText = L"MoveCyclicReverse";
		CommandInfo.pszName = L"�T�C�N���b�N��TVTest�̃E�B���h�E���ړ�(�t��)";
		CommandInfo.pszDescription = L"�T�C�N���b�N��TVTest�̃E�B���h�E���ړ����܂�(�t��)";
		CommandInfo.hbmIcon = (HBITMAP)::LoadImage(g_hinstDLL, MAKEINTRESOURCE(IDB_ICONREV),
													IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
		m_pApp->RegisterPluginCommand(&CommandInfo);

		::DeleteObject(CommandInfo.hbmIcon);
		CommandInfo.hbmIcon = NULL;

		// �uTVTest�̃E�B���h�E�ʒu�������v�̓o�^
		CommandInfo.Size = sizeof(CommandInfo);
		CommandInfo.Flags = TVTest::PLUGIN_COMMAND_FLAG_ICONIZE;
		CommandInfo.State = TVTest::PLUGIN_COMMAND_STATE_DISABLED;
		CommandInfo.ID = COMMAND_EXCHANGE_WINDOW;
		CommandInfo.pszText = L"ExchangeWindow";
		CommandInfo.pszName = L"TVTest�̃E�B���h�E�ʒu������";
		CommandInfo.pszDescription = L"TVTest�̃E�B���h�E�ʒu������";
		CommandInfo.hbmIcon = (HBITMAP)::LoadImage(g_hinstDLL, MAKEINTRESOURCE(IDB_ICONEXCHANGE),
													IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
		m_pApp->RegisterPluginCommand(&CommandInfo);

		::DeleteObject(CommandInfo.hbmIcon);
		CommandInfo.hbmIcon = NULL;
	}
	else
	{
		m_pApp->RegisterCommand(COMMAND_MOVE_CYCLIC, L"MoveCyclic", L"�T�C�N���b�N��TVTest�̃E�B���h�E���ړ�");
		m_pApp->RegisterCommand(COMMAND_MOVE_CYCLIC_REVERSE, L"MoveCyclicReverse", L"�T�C�N���b�N��TVTest�̃E�B���h�E���ړ�(�t��)");
		m_pApp->RegisterCommand(COMMAND_EXCHANGE_WINDOW, L"ExchangeWindow", L"TVTest�̃E�B���h�E�ʒu������");
	}

	// �C�x���g�R�[���o�b�N�֐���o�^
	m_pApp->SetEventCallback(EventCallback,this);

	m_CommMgr.Initialize();

	m_fInitialized = true;
	return true;
}

// �I������
bool CCyclicWnd::Finalize()
{
	m_CommMgr.Finalize();
	m_fInitialized = false;

	return true;
}


// �v���O�C���̗L��/�������؂�ւ�������̏���
bool CCyclicWnd::OnEnablePlugin(bool fEnable)
{
	if (m_fEnabled != fEnable)
	{
		TVTest::HostInfo Host;

		if (fEnable)
		{
			m_CommMgr.JoinService(m_pApp->GetAppWindow());
		}
		else
		{
			m_CommMgr.LeaveService();
		}

		m_fEnabled = fEnable;

		if (m_pApp->GetHostInfo(&Host)
			&& Host.SupportedPluginVersion >= TVTEST_PLUGIN_VERSION_(0, 0, 14))
		{
			m_pApp->SetPluginCommandState(COMMAND_MOVE_CYCLIC,
				m_fEnabled ? 0 : TVTest::PLUGIN_COMMAND_STATE_DISABLED);
			m_pApp->SetPluginCommandState(COMMAND_MOVE_CYCLIC_REVERSE,
				m_fEnabled ? 0 : TVTest::PLUGIN_COMMAND_STATE_DISABLED);
			m_pApp->SetPluginCommandState(COMMAND_EXCHANGE_WINDOW,
				m_fEnabled ? 0 : TVTest::PLUGIN_COMMAND_STATE_DISABLED);
		}
	}

	return true;
}

// ��Window�����X�g�̐擪�Ɏ����Ă���
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

// �ŏ������Ă���Window�̓��X�g�����菜��
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
		// �ŏ������Ă���E�B���h�E�͈ړ��Ώۂ��珜��
		RemoveIconicClientList(list);

		if (list.size() > 1)
		{
			// �t���Ɉړ�����ꍇ�A���X�g���̂��t���ɂ��Ă���
			if (fReverse)
			{
				list.reverse();
			}

			// ��Window�����X�g�̐擪�Ɏ����Ă��Ă���
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
	DBGMSG((TEXT("CCyclicWnd::OnMoveCyclic:\n")));
	MoveWindowCyclic(false);
}

void CCyclicWnd::OnMoveCyclicReverse()
{
	DBGMSG((TEXT("CCyclicWnd::OnMoveCyclicReverse:\n")));
	MoveWindowCyclic(true);
}

void CCyclicWnd::OnExchangeWindow()
{
	std::list<cyclicWndInfo_t*> list;
	BOOL bRet = FALSE;

	DBGMSG((TEXT("CCyclicWnd::OnExchangeWindow: In\n")));

	bRet = m_CommMgr.GetClientList(list);
	if (bRet)
	{
		// �ŏ������Ă���E�B���h�E�͈ړ��Ώۂ��珜��
		RemoveIconicClientList(list);

		if (list.size() > 1)
		{
			// ��Window�����X�g�̐擪�Ɏ����Ă��Ă���
			bRet = FeedHeadClientList(list);
			if (bRet)
			{
				RECT				rcSrc;
				RECT				rcDst;
				LONG				lAreaSize		= 0;
				LONG				lMaxAreaSize	= 0;
				cyclicWndInfo_t*	pSrcInfo		= NULL;
				cyclicWndInfo_t*	pDstInfo		= NULL;
				std::list<cyclicWndInfo_t*>::iterator itr;

				/* ��Window�̃T�C�Y���v�Z */
				pSrcInfo = list.front();
				list.pop_front();

				GetWindowRect(pSrcInfo->hWnd, &rcSrc);
				lMaxAreaSize = (rcSrc.bottom - rcSrc.top)*(rcSrc.right - rcSrc.left);

				DBGMSG((TEXT("CCyclicWnd::OnExchangeWindow: pid=%ld, top=%d,left=%d,bottom=%d,right=%d, size=%ld\n"),
					pSrcInfo->dwProcessID,
					rcSrc.top,
					rcSrc.left,
					rcSrc.bottom,
					rcSrc.right,
					lMaxAreaSize
				));

				/* �T�C�Y����ԑ傫�ȃE�B���h�E��T�� */
				for (itr = list.begin(); itr != list.end(); ++itr)
				{
					RECT				rc;
					cyclicWndInfo_t*	pInfo = NULL;

					pInfo = *itr;
					GetWindowRect(pInfo->hWnd, &rc);
					lAreaSize = (rc.bottom - rc.top)*(rc.right - rc.left);

					DBGMSG((TEXT("CCyclicWnd::OnExchangeWindow: pid=%ld, top=%d,left=%d,bottom=%d,right=%d, size=%ld\n"),
						pInfo->dwProcessID,
						rc.top,
						rc.left,
						rc.bottom,
						rc.right,
						lAreaSize
					));

					if (lAreaSize > lMaxAreaSize)
					{
						pDstInfo = pInfo;
						memcpy(&rcDst, &rc, sizeof(RECT));
						lMaxAreaSize = lAreaSize;
					}
				}

				if (pDstInfo != NULL)
				{
					/* ���E�B���h�E�̖ʐς��ő�ł͂Ȃ�*/

					MoveWindow(pSrcInfo->hWnd, rcDst.left, rcDst.top,
									rcDst.right - rcDst.left, rcDst.bottom - rcDst.top, TRUE);
					MoveWindow(pDstInfo->hWnd, rcSrc.left, rcSrc.top,
									rcSrc.right - rcSrc.left, rcSrc.bottom - rcSrc.top, TRUE);

					SetForegroundWindow(pDstInfo->hWnd);

					pSrcInfo->hLastExchangeWnd = pDstInfo->hWnd;
					pDstInfo->hLastExchangeWnd = NULL;

					m_CommMgr.ExchangeClient(pSrcInfo, pDstInfo);
				}
				else
				{
					/* ���E�B���h�E�̖ʐς��ő� */

					/* ���O�Ɍ��������E�B���h�E�ƍēx�������� */
					if (pSrcInfo->hLastExchangeWnd != NULL)
					{
						/* ���O�Ɍ��������E�B���h�E���܂����݂��Ă��邩�m�F */
						for (itr = list.begin(); itr != list.end(); ++itr)
						{
							cyclicWndInfo_t*	pInfo = NULL;

							pInfo = (*itr);
							if (pInfo->hWnd == pSrcInfo->hLastExchangeWnd)
							{
								pDstInfo = pInfo;
								break;
							}
						}

						if (pDstInfo != NULL)
						{
							GetWindowRect(pDstInfo->hWnd, &rcDst);

							MoveWindow(pSrcInfo->hWnd, rcDst.left, rcDst.top,
								rcDst.right - rcDst.left, rcDst.bottom - rcDst.top, TRUE);
							MoveWindow(pDstInfo->hWnd, rcSrc.left, rcSrc.top,
								rcSrc.right - rcSrc.left, rcSrc.bottom - rcSrc.top, TRUE);

							SetForegroundWindow(pDstInfo->hWnd);

							pSrcInfo->hLastExchangeWnd = NULL;
							pDstInfo->hLastExchangeWnd = pSrcInfo->hWnd;

							m_CommMgr.ExchangeClient(pSrcInfo, pDstInfo);
						}
					}
				}
			}
		}

		m_CommMgr.FreeClientList(list);
	}
	DBGMSG((TEXT("CCyclicWnd::OnExchangeWindow: Out\n")));
}

// �C�x���g�R�[���o�b�N�֐�
// �����C�x���g���N����ƌĂ΂��
LRESULT CALLBACK CCyclicWnd::EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
{
	CCyclicWnd *pThis=static_cast<CCyclicWnd*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// �v���O�C���̗L����Ԃ��ω�����
		return pThis->OnEnablePlugin(lParam1!=0);

	case TVTest::EVENT_COMMAND:
		// �R�}���h�����s���ꂽ
		if (lParam1 == COMMAND_MOVE_CYCLIC)
		{
			pThis->OnMoveCyclic();
		}
		else if (lParam1 == COMMAND_MOVE_CYCLIC_REVERSE)
		{
			pThis->OnMoveCyclicReverse();
		}
		else if (lParam1 == COMMAND_EXCHANGE_WINDOW)
		{
			pThis->OnExchangeWindow();
		}
		return TRUE;
	}

	return 0;
}


// �v���O�C���N���X�̃C���X�^���X�𐶐�����
TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CCyclicWnd;
}
