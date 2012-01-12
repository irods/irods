// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "Inquisitor.h"

#include "MainFrm.h"
#include "LeftView.h"
//#include "MetaView.h"
#include "InquisitorView.h"
#include "GenericDialog.h"
#include "InquisitorDoc.h"
#include "MetadataDlg.h"
#include "irodsWinUtils.h"
#include "DialogWebDisp.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern bool gbIsDemo;
extern bool gCaptureOn;
extern bool gbExiting;
////////////////////////////////////////////////////////////////////////////
// CMainFrame
	// Global help commands
	//ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
	//ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
	//ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
	//ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)
//	ON_BN_CLICKED(IDC_NODE_UP, OnNodeUp)

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_UPDATE_COMMAND_UI_RANGE(AFX_ID_VIEW_MINIMUM, AFX_ID_VIEW_MAXIMUM, OnUpdateViewStyles)
	ON_COMMAND_RANGE(AFX_ID_VIEW_MINIMUM, AFX_ID_VIEW_MAXIMUM, OnViewStyle)
	//ON_COMMAND(ID_TREE_UP, OnTreeUp)
	ON_COMMAND(ID_TREE_HOME, OnTreeHome)
	ON_CBN_SELCHANGE(IDC_TREE_STACK, OnSelChangeTreeStack)
	ON_COMMAND(ID_BACK, OnBack)
	ON_COMMAND(ID_FOOFIGHTER, OnFooFighter)
	ON_COMMAND(ID_FORWARD, OnForward)
	
	//ON_COMMAND(ID_CHANGE_PASSWORD, OnChangePassword)
	//ON_UPDATE_COMMAND_UI(ID_CHANGE_PASSWORD, OnUpdateChangePassword)
	//ON_UPDATE_COMMAND_UI(ID_TREE_UP, OnUpdateTreeUp)
	//ON_UPDATE_COMMAND_UI(ID_TREE_HOME, OnUpdateTreeHome)
	//ON_UPDATE_COMMAND_UI(ID_FOOFIGHTER, OnUpdateFooFighter)
	//ON_UPDATE_COMMAND_UI(ID_VIEW_JUMPVIEWBAR, OnUpdateViewJumpviewbar)
	//ON_COMMAND(ID_VIEW_JUMPVIEWBAR, OnViewJumpviewbar)
	//}}AFX_MSG_MAP
	ON_REGISTERED_MESSAGE(msgStatusLine, OnStatusLine)
	ON_REGISTERED_MESSAGE(msgRefillResourceList, OnRefillResourceList)
	ON_REGISTERED_MESSAGE(msgRefresh, OnRefresh)
	//ON_REGISTERED_MESSAGE(msgGoAnimation, OnGoAnimation)
	//ON_REGISTERED_MESSAGE(msgStopAnimation, OnStopAnimation)
	//ON_REGISTERED_MESSAGE(msgDLProgress, OnDLProgress)
	//ON_REGISTERED_MESSAGE(msgULProgress, OnULProgress)
	//ON_REGISTERED_MESSAGE(msgConnectedUpdate, OnConnectedUpdate)
	//ON_REGISTERED_MESSAGE(msgProgressRotate, OnProgressRotate)

	ON_REGISTERED_MESSAGE(METADATA_DLG_CLOSE_MSG, onMetadataDlgClosed)
	ON_BN_CLICKED(IDC_BUTTON_DBAR3_STORAGE_RSOURCE, OnBnClickedStorageResource)
END_MESSAGE_MAP()

static UINT indicators[] =
{
ID_STATUS_TEXT,
IDS_STATUS_COUNT,
ID_STATUS_PBAR,
};


//extern CMutex gCS;
//extern int gCount;
/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	m_isPlaying = false;
	m_finishedInitializing = false;
	m_bProgressEnabled = false;
	
	m_metadataDlgIdCounter = 1;

	m_metaSearchDlg = NULL;

	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON33);
}

CMainFrame::~CMainFrame()
{
	for(int i=0;i<m_metadataDlgPointArray.size();i++)
	{
		CMetadataDlg *metaDlg = m_metadataDlgPointArray[i];
		metaDlg->DestroyWindow();
		delete metaDlg;
		m_metadataDlgPointArray[i] = NULL;
	}

	DestroyIcon(m_hIcon);
}

void CMainFrame::FooFighter()
{
	this->OnFooFighter();
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_TOOLBAR3))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;     
	}

	/*
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME1))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;     
	}
	*/

	// TBSTYLE_BUTTON|TBSTYLE_FLAT, TBSTYLE_FLAT, CBRS_TOP
	/*
	if (!m_wndViewsBar.CreateEx(this, TBSTYLE_FLAT , WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP | CBRS_TOOLTIPS) || //| CBRS_FLYBY ) ||
		!m_wndViewsBar.LoadToolBar(IDR_LISTVIEWS1))
	{
		TRACE0("Failed to create viewsbar\n");
		return -1;   
	}
	*/
	
	/*   BBBBB
	if (!m_wndViewsBar.CreateEx(this, BTNS_BUTTON , WS_CHILD | WS_VISIBLE | CBRS_RIGHT
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndViewsBar.LoadToolBar(IDR_LISTVIEWS1))
	{
		TRACE0("Failed to create viewsbar\n");
		return -1;      // fail to create
	}
	*/

	/*
	m_wndViewsBar.SetWindowText("iRODS Command Bar");

	if (!m_wndDialogBar.Create(this, IDD_DIALOGBAR1, 
		CBRS_ALIGN_TOP , AFX_IDW_DIALOGBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;		// fail to create
	}
	*/

	/* BBB
	if (!m_wndDialogBar3.Create(this, IDD_DIALOGBAR3, 
		CBRS_ALIGN_TOP , AFX_IDW_DIALOGBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;		// fail to create
	}
	*/

	/*
	if (!m_wndDialogBar4.Create(this, IDD_DIALOGBAR4, 
		CBRS_ALIGN_TOP , AFX_IDW_DIALOGBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;		// fail to create
	}
	*/

	//m_wndAnimate.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 10, 10), this, AFX_IDW_TOOLBAR + 2);
	//m_wndAnimate.Open(IDR_AVI1);

	if(	!m_wndReBar.Create(this)
		||!m_wndReBar.AddBar(&m_wndToolBar)
	//	||!m_wndReBar.AddBar(&m_wndDialogBar, NULL, NULL, RBBS_BREAK | RBBS_GRIPPERALWAYS | RBBS_FIXEDBMP)
	//	||!m_wndReBar.AddBar(&m_wndViewsBar)   // BBB ||!m_wndReBar.AddBar(&m_wndDialogBar3)
	//	||!m_wndReBar.AddBar(&m_wndDialogBar4)
	//	||!m_wndReBar.AddBar(&m_wndAnimate, NULL, NULL, RBBS_FIXEDSIZE | RBBS_FIXEDBMP)
		)
	{
		TRACE0("Failed to create rebar\n");
		return -1;      // fail to create
	}

	if (!m_wndDialogBar3.Create(this, IDD_DIALOGBAR3,  CBRS_ALIGN_TOP , AFX_IDW_DIALOGBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;		// fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,3))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	m_wndStatusBar.SetPaneInfo(0, ID_STATUS_TEXT, SBPS_STRETCH, 100);
	m_wndStatusBar.SetPaneInfo(1, IDS_STATUS_COUNT, SBPS_NORMAL,260);
	m_wndStatusBar.SetPaneInfo(2, ID_STATUS_PBAR, SBPS_NORMAL,100);

	//m_wndToolBar.EnableDocking(CBRS_ALIGN_TOP);
	//m_wndViewsBar.EnableDocking(CBRS_ALIGN_TOP);
	//m_wndDialogBar3.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	//DockControlBar(&m_wndDialogBar3);

	// BBB_new LoadBarState("ToolBarSetttings");

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_finishedInitializing = true;

	return 0;
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/,
	CCreateContext* pContext)
{
	if(!m_wndSplitter.CreateStatic(this, 1, 2))
		return FALSE;

	if(!m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CLeftView), CSize(300, 100), pContext))
		return FALSE;

	if(!m_wndSplitter.CreateView(0, 1,RUNTIME_CLASS(CInquisitorView), CSize(100, 100), pContext))
		return FALSE;

	//if(!m_wndSplitter2.CreateStatic(&m_wndSplitter,2,1, WS_CHILD | WS_VISIBLE | WS_BORDER, m_wndSplitter.IdFromRowCol(0,1)))
	//	return FALSE;

	//if(!m_wndSplitter2.CreateView(0, 0,RUNTIME_CLASS(CMetaView), CSize(100, 100), pContext))
	//	return FALSE;

	//if(!m_wndSplitter2.CreateView(1, 0,RUNTIME_CLASS(CInquisitorView), CSize(100, 100), pContext))
	//	return FALSE;


	return TRUE;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.cx = 800;
	cs.cy = 600;

	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

/*
void CMainFrame::SetFocusMeta()
{
	CMetaView* meta = GetMetaPane();
	meta->SetFocus();
	meta->NewMeta();
}
*/

CInquisitorView* CMainFrame::GetRightPane()
{
	//CSplitterWnd* pSplit = (CSplitterWnd*)m_wndSplitter.GetPane(0, 1);
	//CWnd* pWnd = pSplit->GetPane(1,0);
	CWnd* pWnd = m_wndSplitter.GetPane(0, 1);
	CInquisitorView* pView = DYNAMIC_DOWNCAST(CInquisitorView, pWnd);
	return pView;
}

/*
CMetaView* CMainFrame::GetMetaPane()
{
	CSplitterWnd* pSplit = (CSplitterWnd*)m_wndSplitter.GetPane(0, 1);
	CWnd* pWnd = pSplit->GetPane(0,0);
	CMetaView* pView = DYNAMIC_DOWNCAST(CMetaView, pWnd);
	return pView;
}
*/

CLeftView* CMainFrame::GetLeftPane()
{
	CWnd* pWnd = m_wndSplitter.GetPane(0, 0);
	CLeftView* pView = DYNAMIC_DOWNCAST(CLeftView, pWnd);
	return pView;
}

void CMainFrame::OnUpdateViewStyles(CCmdUI* pCmdUI)
{
	int row, col;

	//if(NULL == m_wndSplitter2.GetActivePane(&row, &col))
	//	return;		//if neither inqview or metaview is active
	if(NULL == m_wndSplitter.GetActivePane(&row, &col))
		return;

	//CListView* pView = GetRightPane();

	/*
	if(row)
	{
		pView = GetRightPane();
	}else
	{
		pView = GetMetaPane();
	}
	*/

	// if the right-hand pane hasn't been created or isn't a view,
	// disable commands in our range

#if 0
	if (pView == NULL)
		pCmdUI->Enable(FALSE);
	else
	{
		DWORD dwStyle = pView->GetStyle() & LVS_TYPEMASK;

		// if the command is ID_VIEW_LINEUP, only enable command
		// when we're in LVS_ICON or LVS_SMALLICON mode

		if (pCmdUI->m_nID == ID_VIEW_LINEUP)
		{
			if (dwStyle == LVS_ICON || dwStyle == LVS_SMALLICON)
				pCmdUI->Enable();
			else
				pCmdUI->Enable(FALSE);
		}
		else
		{
			// otherwise, use dots to reflect the style of the view
			pCmdUI->Enable();
			BOOL bChecked = FALSE;

			switch (pCmdUI->m_nID)
			{
			case ID_VIEW_DETAILS:
				bChecked = (dwStyle == LVS_REPORT);
				break;

			case ID_VIEW_SMALLICON:
				bChecked = (dwStyle == LVS_SMALLICON);
				break;

			case ID_VIEW_LARGEICON:
				bChecked = (dwStyle == LVS_ICON);
				break;

			case ID_VIEW_LIST:
				bChecked = (dwStyle == LVS_LIST);
				break;

			default:
				bChecked = FALSE;
				break;
			}

			pCmdUI->SetRadio(bChecked ? 1 : 0);
		}
	}
#endif
}


void CMainFrame::OnViewStyle(UINT nCommandID)
{
	int row, col;

	//if(NULL == m_wndSplitter2.GetActivePane(&row, &col))
	//	return;		//if neither inqview or metaview is active

	CListView* pView = GetRightPane();

	/*
	if(row)
	{
		pView = GetRightPane();
	}else
	{
		pView = GetMetaPane();
	}
	*/


	// if the right-hand pane has been created and is a CInquisitorView,
	// process the menu commands...
	if (pView != NULL)
	{
		DWORD dwStyle = -1;

		switch (nCommandID)
		{
		case ID_VIEW_LINEUP:
			{
				// ask the list control to snap to grid
				CListCtrl& refListCtrl = pView->GetListCtrl();
				refListCtrl.Arrange(LVA_SNAPTOGRID);
			}
			break;

		// other commands change the style on the list control
		case ID_VIEW_DETAILS:
			dwStyle = LVS_REPORT;
			break;

		case ID_VIEW_SMALLICON:
			dwStyle = LVS_SMALLICON;
			break;

		case ID_VIEW_LARGEICON:
			dwStyle = LVS_ICON;
			break;

		case ID_VIEW_LIST:
			dwStyle = LVS_LIST;
			break;
		}

		// change the style; window will repaint automatically
		if (dwStyle != -1)
		{
			pView->ModifyStyle(LVS_TYPEMASK, dwStyle);
			//hack?
			//pView->Refresh();
			if(row)
			{
				//inq
				((CInquisitorView*)pView)->Refresh();
			}else
			{
				//meta
				//((CMetaView*)pView)->Refresh();
			}
			
		}
	}
}

void CMainFrame::SetStatusMessage(const char* message)
{
	if(message)
		m_wndStatusBar.SetPaneText(0, message);
}

void CMainFrame::SetCountMessage(const char* message)
{
	if(message)
		m_wndStatusBar.SetPaneText(1, message);
}

void CMainFrame::SetLoginMessage(const char* message)
{
	if(message)
		m_wndStatusBar.SetPaneText(2, message);
}

void CMainFrame::SetStack(int i)
{
	//CComboBox* pCBox = (CComboBox*)m_wndDialogBar.GetDlgItem(IDC_TREE_STACK);
	//pCBox->SetCurSel(i);
}

int CMainFrame::Push(const char* name)
{
	//CComboBox* pCBox = (CComboBox*)m_wndDialogBar.GetDlgItem(IDC_TREE_STACK);

	//return pCBox->InsertString(0,name);
	return 0;
}

void CMainFrame::FillResourceList(CStringArray & RescNames, CString & default_resc_from_login)
{
	// BING: it is done!
	CComboBox* pCBox = (CComboBox*)m_wndDialogBar3.GetDlgItem(IDC_RESOURCE);
	int pos;
	int got_it = 0;
	for(int i=0;i<RescNames.GetSize(); i++)
	{
		(void)pCBox->AddString(RescNames[i]);

		if((default_resc_from_login.GetLength() > 0)&&(default_resc_from_login.Compare(RescNames[i])==0))
		{
			got_it = 1;
		}
	}

	if(got_it)
	{
		//pCBox->SetWindowText(default_resc_from_login);
		pCBox->SelectString(0, default_resc_from_login);
	}
}

void CMainFrame::OnSelChangeTreeStack() 
{
#if 0
	CString strItem;

	CComboBox* pCBox = (CComboBox*)m_wndDialogBar.GetDlgItem(IDC_TREE_STACK);

	int nIndex = pCBox->GetCurSel();

	if (nIndex == CB_ERR)
		return;

	pCBox->GetLBText(nIndex, strItem);

	CLeftView* pView = GetLeftPane();

	if (pView != NULL)
		pView->ShowName(strItem)
#endif
}

void CMainFrame::ClearStack(bool resourcetoo)
{
#if 0
	CComboBox* pCBox = (CComboBox*)m_wndDialogBar.GetDlgItem(IDC_TREE_STACK);
	pCBox->ResetContent();

	if(resourcetoo)
	{
		pCBox = (CComboBox*)m_wndDialogBar3.GetDlgItem(IDC_RESOURCE);
		pCBox->ResetContent();
	}
#endif
};

LRESULT CMainFrame::OnRefillResourceList(WPARAM wParam, LPARAM lParam)
{
	CInquisitorDoc* pDoc = (CInquisitorDoc*)((CMainFrame*)AfxGetMainWnd())->GetActiveDocument();

	return 0;
}

#if 0
void CMainFrame::OnTreeUp() 
{
	CLeftView* pView = GetLeftPane();


	if (pView != NULL)
		pView->ShowParent();	

}
#endif

void CMainFrame::OnTreeHome() 
{
	CLeftView* pView = GetLeftPane();

#if 0
	if (pView != NULL)
		pView->ShowHome();
#endif
}

#if 0	
LRESULT CMainFrame::OnGoAnimation(WPARAM wParam, LPARAM lParam)
{

	if(!m_isPlaying)
	{
		BeginWaitCursor();
		SetCapture();
		gCaptureOn = true;
		CMenu* pTopMenu = AfxGetMainWnd()->GetMenu();
		pTopMenu->EnableMenuItem(0, MF_BYPOSITION | MF_GRAYED);
		pTopMenu->EnableMenuItem(1, MF_BYPOSITION | MF_GRAYED);
		SetMenu(pTopMenu);
		m_wndAnimate.Play(0, -1, -1);

		m_isPlaying = true;
	}

	return TRUE;
}
#endif

#if 0
LRESULT CMainFrame::OnStopAnimation(WPARAM wParam, LPARAM lParam)
{

	if(m_isPlaying)
	{
		m_wndAnimate.Stop();
		m_wndAnimate.Seek(0);
		EndWaitCursor();
		CMenu* pTopMenu = AfxGetMainWnd()->GetMenu();
		pTopMenu->EnableMenuItem(0, MF_BYPOSITION | MF_ENABLED);
		pTopMenu->EnableMenuItem(1, MF_BYPOSITION | MF_ENABLED);
		SetMenu(pTopMenu);
		ReleaseCapture();
		gCaptureOn = false;
		m_isPlaying = false;
	}

	return TRUE;
}
#endif

LRESULT CMainFrame::OnStatusLine(WPARAM wParam, LPARAM lParam)
{
	char* message = (char*)wParam;

	m_wndStatusBar.SetPaneText(0, message);

	if(lParam)
		free(message);

	return 0;
}

void CMainFrame::statusbar_msg(CString & msg)
{
	m_wndStatusBar.SetPaneText(0, msg);
}

void CMainFrame::statusbar_msg2(CString & msg)
{
	m_wndStatusBar.SetPaneText(1, msg);
}

void CMainFrame::statusbar_clear()
{
	CString msg = "";
	m_wndStatusBar.SetPaneText(0, msg);
}

LRESULT CMainFrame::OnRefresh(WPARAM wParam, LPARAM lParam) 
{
	GetActiveDocument()->UpdateAllViews(NULL, lParam);

	return 0;
}

void CMainFrame::OnFooFighter() 
{
	CInquisitorDoc* pDoc = (CInquisitorDoc*)GetActiveDocument();
	//pDoc->Disconnect();
	if(!pDoc->MyNewConnection())
	{
		CFrameWnd::OnClose();
	}
	ClearStack(true);
}
void CMainFrame::OnBack() 
{
	CLeftView* view = GetLeftPane();
	view->OnBack();
}

void CMainFrame::OnForward() 
{
	CLeftView* view = GetLeftPane();
	view->OnForward();
}

#if 0
LRESULT CMainFrame::OnDLProgress(WPARAM wParam, LPARAM lParam)
{
	static char message[1024];

	char* szptr = (char*)wParam;

	int value = lParam;
	
	sprintf(message, "Downloading %s : %d%% completed", szptr, value);

	m_wndStatusBar.SetPaneText(0, message);

	static bool enabled = false;

	CRect rc;

	if(!m_bProgressEnabled)
	{
		m_wndStatusBar.GetItemRect(2, &rc);
		VERIFY(m_wndProgress.Create(WS_CHILD | WS_VISIBLE | PBS_SMOOTH , rc, &m_wndStatusBar, 1));
		m_wndProgress.SetRange(1, 100);
		m_bProgressEnabled = true;
	}

	if(0 == value && NULL == szptr)
	{
		m_wndStatusBar.SetPaneText(0, "");
		m_wndProgress.SetPos(0);
	}

	m_wndProgress.SetPos(value);

	return TRUE;
}
#endif

#if 0
LRESULT CMainFrame::OnProgressRotate(WPARAM wParam, LPARAM lParam)
{
	static char message[1024];

	char* szptr = (char*)wParam;

	int value;
	

	static bool increasing = true;

	CRect rc;

	if(!m_bProgressEnabled)
	{
		m_wndStatusBar.GetItemRect(2, &rc);
		VERIFY(m_wndProgress.Create(WS_CHILD | WS_VISIBLE | PBS_SMOOTH , rc, &m_wndStatusBar, 1));
		m_wndProgress.SetRange(1, 100);
		m_bProgressEnabled = true;
	}


		value = m_wndProgress.GetPos();
		m_wndStatusBar.SetPaneText(0, "");

		if(value == 1)
			value = 0;

		if(increasing)
			value += 5;
		else
			value -= 5;

		if(value == 100)
			increasing = false;

		if(value == 0)
			increasing = true;

		m_wndProgress.SetPos(value);


	Sleep(500);
	return TRUE;
}
#endif

#if 0
LRESULT CMainFrame::OnConnectedUpdate(WPARAM wParam, LPARAM lParam)
{
	CLeftView* view = GetLeftPane();
	//view->ConnectedUpdate(wParam, lParam);

	return TRUE;
}
#endif

#if 0
LRESULT CMainFrame::OnULProgress(WPARAM wParam, LPARAM lParam)
{
	static char message[1024];

	char* szptr = (char*)wParam;

	int value = lParam;

	if(value == 100)
	{
		sprintf(message, "Upload Completed. Registering data into WINI...");
	}else
	{
		sprintf(message, "Uploading %s : %d%% completed", szptr, value);
	}

	//itoa(value, message, 10);

	m_wndStatusBar.SetPaneText(0, message);

	CRect rc;

	if(!m_bProgressEnabled)
	{
		m_wndStatusBar.GetItemRect(2, &rc);
		VERIFY(m_wndProgress.Create(WS_CHILD | WS_VISIBLE | PBS_SMOOTH , rc, &m_wndStatusBar, 1));
		m_wndProgress.SetRange(1, 100);
		m_bProgressEnabled = true;
	}

	if(0 == value && NULL == szptr)
	{
		m_wndStatusBar.SetPaneText(0, "");
		m_wndProgress.SetPos(0);
	}

	m_wndProgress.SetPos(value);

	return TRUE;
}
#endif

/*
void CMainFrame::OnChangePassword() 
{
	CInquisitorDoc* pDoc = (CInquisitorDoc*)GetActiveDocument();

	if(NULL == pDoc)
		return;

	CGenericDialog myDlg("Change Password");

	if(myDlg.DoModal() != IDOK)
	{
		return;
	}

	// change the password for user
	status = pDoc->ChangePassword(myDlg.m_Edit.LockBuffer());
		myDlg.m_Edit.UnlockBuffer();
	char buf[256];

	if(status.isOk())
		sprintf(buf, "password change succeeded.");
	else
		sprintf(buf, "couldn't change password: %s", status.GetError());

	SetStatusMessage(buf);
}
*/

/*
void CMainFrame::OnUpdateChangePassword(CCmdUI* pCmdUI) 
{
#if 0
	CInquisitorDoc* pDoc = (CInquisitorDoc*)GetActiveDocument();

	if(NULL != pDoc)
		if(pDoc->isConnected())
			if(!gbIsDemo)
				return;

	pCmdUI->Enable(FALSE);
#endif
}
*/

#if 0
void CMainFrame::OnUpdateFooFighter(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);
}
#endif

#if 0
void CMainFrame::OnUpdateTreeUp(CCmdUI* pCmdUI) 
{
	CInquisitorDoc* pDoc = (CInquisitorDoc*)GetActiveDocument();

	WINI::INode* node = pDoc->GetCurrentNode();

	if(NULL == node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	int type = node->GetType();

	if(type == WINI_SET || type == WINI_ZONE)
		pCmdUI->Enable(FALSE);

	WINI::INode* parent = node->GetParent();

	if(NULL == parent)
		pCmdUI->Enable(FALSE);

	type = parent->GetType();

	if(type == WINI_SET || type == WINI_ZONE)
		pCmdUI->Enable(FALSE);


}
#endif

#if 0
void CMainFrame::OnUpdateTreeHome(CCmdUI* pCmdUI) 
{

	CInquisitorDoc* pDoc = (CInquisitorDoc*)GetActiveDocument();

	if(pDoc->isConnected())
		if(pDoc->GetHome())
			return;

	pCmdUI->Enable(FALSE);	

}
#endif

#if 0
void CMainFrame::OnUpdateViewJumpviewbar(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck((m_wndViewsBar.GetStyle() & WS_VISIBLE) != 0);
}
#endif

#if 0
void CMainFrame::OnViewJumpviewbar() 
{
	BOOL bVisible = ((m_wndViewsBar.GetStyle() & WS_VISIBLE) != 0);

	ShowControlBar(&m_wndViewsBar, !bVisible, FALSE);
	RecalcLayout();	
}
#endif

void CMainFrame::OnClose() 
{
	if(IDYES == AfxMessageBox("Are you sure you want to exit this program?", MB_YESNO))
	{
		CInquisitorApp* pApp = (CInquisitorApp *)AfxGetApp();
		
		CInquisitorDoc* pDoc = (CInquisitorDoc*)GetActiveDocument();
		if(pDoc->profile_id_from_login >= 1)
		{
			CString default_resc = get_default_resc();
			
			if((default_resc.GetLength() > 0)&&(default_resc.Compare(pDoc->default_resc_from_login) != 0))  // save the default resource info
			{
				static char buf[255];
				sprintf(buf, "History\\%d", pDoc->profile_id_from_login);
				AfxGetApp()->WriteProfileString(buf, "DefaultResource", default_resc);
			}
		}

		if(pDoc->conn != NULL)
			rcDisconnect(pDoc->conn);

		gbExiting = true;

		// clean the left tree and right list 
		CLeftView *leftView = GetLeftPane(); 
		leftView->clear_tree();

		CInquisitorView *pView = GetRightPane();
		pView->rlist_remove_all(RLIST_CLEAN_ALL);

		if(m_metaSearchDlg != NULL)
			m_metaSearchDlg->OnCancel();

		// SaveBarState("ToolBarSetttings");
		CFrameWnd::OnClose();
	}
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) 
{
	if(gCaptureOn)
	{
		if(pMsg->message == WM_MOUSEMOVE)		//OnMouseMove only catches mouse-moves sent to frame
		{
			CMainFrame* wnd = dynamic_cast<CMainFrame*>(AfxGetMainWnd());
			POINT pt;
			CRect rect;
			if(wnd)
			{
				GetCursorPos(&pt);
				wnd->ScreenToClient(&pt);
				GetClientRect(&rect);
				if(rect.PtInRect(pt))
				{
					SetCapture();
					RestoreWaitCursor();
				}
				else
				{
					ReleaseCapture();
				}
			}
		}
	}

	return CFrameWnd::PreTranslateMessage(pMsg);
}

CString CMainFrame::get_default_resc()
{
	CComboBox* pCBox = (CComboBox*)m_wndDialogBar3.GetDlgItem(IDC_RESOURCE);
	CString resc;
	pCBox->GetWindowTextA(resc);
	return resc;
}

LRESULT CMainFrame::onMetadataDlgClosed(WPARAM wParam, LPARAM lParam)
{
	int nid = (int)wParam;
	int j = -1;

	for(int i=0;i<m_dlgIdArray.size();i++)
	{
		if(m_dlgIdArray[i] == nid)
		{
			j = i;
			break;
		}
	}

	if(j >= 0)
	{
		CMetadataDlg *metaDlg = m_metadataDlgPointArray[j];
		delete metaDlg;
		m_metadataDlgPointArray[j] = NULL;
		
		// remove from 
		m_dlgIdArray.erase(m_dlgIdArray.begin()+j);
		m_objIdArrayForMetadataDlg.RemoveAt(j);
		m_metadataDlgPointArray.erase(m_metadataDlgPointArray.begin() + j);
	}

	return TRUE;
}

void CMainFrame::CreateMetadataDlg(CString & ObjNameWpath, int ObjType)
{
	CInquisitorDoc* pDoc = (CInquisitorDoc*)GetActiveDocument();
	dataObjInp_t dataObjInp;
	memset(&dataObjInp, 0, sizeof (dataObjInp));
	rstrcpy (dataObjInp.objPath, (char *)LPCTSTR(ObjNameWpath), MAX_NAME_LEN);
	rodsObjStat_t *rodsObjStatOut = NULL;
	int t = rcObjStat (pDoc->conn, &dataObjInp, &rodsObjStatOut);
	if(t < 0)
	{
		CString msgHead = CString("Query Object State error: ");
		pDoc->disp_err_msgbox(msgHead, t, this);
		return;
	}

	if((rodsObjStatOut->specColl != NULL)&&(rodsObjStatOut->specColl->collClass == MOUNTED_COLL))
	{
		AfxMessageBox("The metadata operation does not apply for mounted collection(s) or data in mounted collection(s).");
		return;
	}

	// check if the dialog is already there
	for(int i=0;i<m_objIdArrayForMetadataDlg.GetSize();i++)
	{
		if(ObjNameWpath == m_objIdArrayForMetadataDlg[i])
		{
			CMetadataDlg *pDlg = m_metadataDlgPointArray[i];
			::SetForegroundWindow(pDlg->m_hWnd);
			return;
		}
	}

	int nid = m_metadataDlgIdCounter++;

	CMetadataDlg *metaDlg = new CMetadataDlg(ObjNameWpath, ObjType);
	metaDlg->m_registeredDlgID = nid;
	metaDlg->Create(IDD_DIALOG_METADATA, GetDesktopWindow());
	//metaDlg->CreateIndirect(IDD_DIALOG_METADATA, GetDesktopWindow());
	metaDlg->ShowWindow(SW_SHOW);

	// register the dialog	
	m_dlgIdArray.push_back(nid);
	m_objIdArrayForMetadataDlg.Add(ObjNameWpath);
	m_metadataDlgPointArray.push_back(metaDlg);
}

void CMainFrame::CreateMetaSeachDlg(CString & TopColl)
{
	if(m_metaSearchDlg == NULL)
	{
		m_metaSearchDlg = new CMetaSearchDlg(TopColl);
		m_metaSearchDlg->Create(IDD_DIALOG_META_SEARCH, GetDesktopWindow());
		m_metaSearchDlg->ShowWindow(SW_SHOW);
	}
	else
	{
		m_metaSearchDlg->ShowWindow(SW_SHOW);
		m_metaSearchDlg->SetTopCollection(TopColl);
	}
	m_metaSearchDlg->BringWindowToTop();
}

void CMainFrame::OnBnClickedStorageResource()
{
	CString current_resc = get_default_resc();
	if(current_resc.GetLength() <= 0)
		return;

	iRODSResourceInfo resc_info;
	CInquisitorDoc* pDoc = (CInquisitorDoc*)GetActiveDocument();
	int t = irodsWinGetRescInfo(pDoc->conn, (char *)LPCTSTR(current_resc), resc_info);

	if(t < 0)
	{
		CString msgHead = CString("Query error for ") + current_resc;
		pDoc->disp_err_msgbox(msgHead, t, this);
		return;
	}

	CString html_info = CString("<html><body bgcolor=#CCCC99><h1>Info for the Resource: ");
	html_info += current_resc;
	html_info += CString("</h1>\n");

	int i;
	CString tmpstr;
	if(resc_info.is_group_resc)
	{
		html_info += CString("<p>It is a group resouce and includes following resouces:</p>\n");
		html_info += CString("<table border=2 cellpadding=2 cols=2 frame=box>\n");
		for(i=0;i<resc_info.sub_rescs.GetSize();i++)
		{
			tmpstr.Format("%d", i+1);
			html_info += CString("<tr><td>Resource #") + tmpstr + CString("</td> <td><I>") +
				resc_info.sub_rescs[i] + CString("</I></td></tr>\n");
		}
	}
	else  // a physical resouce
	{
		html_info += CString("<table border=2 cellpadding=2 cols=2 frame=box>\n");
		html_info += CString("<tr><td>Name") + CString("</td> <td><I>") +
				resc_info.name + CString("</I></td></tr>\n");
		html_info += CString("<tr><td>ID") + CString("</td> <td><I>") +
				resc_info.id + CString("</I></td></tr>\n");
		html_info += CString("<tr><td>Zone") + CString("</td> <td><I>") +
				resc_info.zone + CString("</I></td></tr>\n");
		html_info += CString("<tr><td>Type") + CString("</td> <td><I>") +
				resc_info.type + CString("</I></td></tr>\n");
		html_info += CString("<tr><td>Class") + CString("</td> <td><I>") +
				resc_info.resc_class + CString("</I></td></tr>\n");
		html_info += CString("<tr><td>Location") + CString("</td> <td><I>") +
				resc_info.location + CString("</I></td></tr>\n");
		html_info += CString("<tr><td>Vault") + CString("</td> <td><I>") +
				resc_info.vault_path + CString("</I></td></tr>\n");
		html_info += CString("<tr><td>Free Space") + CString("</td> <td><I>") +
				resc_info.free_space + CString("</I></td></tr>\n");
		html_info += CString("<tr><td>Status") + CString("</td> <td><I>") +
				resc_info.status + CString("</I></td></tr>\n");
		html_info += CString("<tr><td>info") + CString("</td> <td><I>") +
				resc_info.info + CString("</I></td></tr>\n");
		html_info += CString("<tr><td>comment") + CString("</td> <td><I>") +
				resc_info.comment + CString("</I></td></tr>\n");
		html_info += CString("<tr><td>Create Time") + CString("</td> <td><I>") +
				resc_info.create_time + CString("</I></td></tr>\n");
		html_info += CString("<tr><td>Modify Time") + CString("</td> <td><I>") +
				resc_info.modify_time + CString("</I></td></tr>\n");
	}

	html_info += CString("</table></body></html>");

	CDialogWebDisp dlg(html_info);
	dlg.DoModal();
}

void CMainFrame::StartProgressBar()
{
	CProgressCtrl * pProgress = (CProgressCtrl*)m_wndDialogBar3.GetDlgItem(IDC_PROGRESS_DLGBAR3);
	pProgress->ShowWindow(SW_SHOW);
	pProgress->SetRange(1, 100);
	pProgress->SetStep(10);
	pProgress->SetPos(10);
}

void CMainFrame::EndProgressBar()
{
	CProgressCtrl * pProgress = (CProgressCtrl*)m_wndDialogBar3.GetDlgItem(IDC_PROGRESS_DLGBAR3);
	pProgress->ShowWindow(SW_HIDE);
}

void CMainFrame::ProgressBarStepIt()
{
	CProgressCtrl * pProgress = (CProgressCtrl*)m_wndDialogBar3.GetDlgItem(IDC_PROGRESS_DLGBAR3);
	int n = pProgress->GetPos();

	n = (n + 10)%100;
	if(n == 0)
		n = 10;

	//pProgress->StepIt();
	pProgress->SetPos(n);
}

void CMainFrame::ProgressBarMove2Full()
{
	CProgressCtrl * pProgress = (CProgressCtrl*)m_wndDialogBar3.GetDlgItem(IDC_PROGRESS_DLGBAR3);
	int lowd, upd;
	pProgress->GetRange(lowd, upd);
	int n = pProgress->GetPos();
	int t = (int)((100-n)/5)+1;
	//int t = (int)(upd - n)/2;
	int i;
	for(i=0;i<t;i++)
	{
		n = n + 5;
		if(n > 100)
			n = 100;
		pProgress->SetPos(n);
		Sleep(30);
	}
	Sleep(100);
	pProgress->SetPos(100);
	Sleep(100);
}

void CMainFrame::SetProgressBarPos(float t)
{
	if((t<0.0)||(t > 1.0))
		return;

	int n = (int)(t*100)%100;
	if(n == 0)
		n = 10;
	CProgressCtrl * pProgress = (CProgressCtrl*)m_wndDialogBar3.GetDlgItem(IDC_PROGRESS_DLGBAR3);
	pProgress->SetPos(n);
}