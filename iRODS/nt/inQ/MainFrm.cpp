// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "Inquisitor.h"

#include "MainFrm.h"
#include "LeftView.h"
#include "MetaView.h"
#include "InquisitorView.h"
#include "winiObjects.h"
#include "GenericDialog.h"

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
	ON_COMMAND(ID_TREE_UP, OnTreeUp)
	ON_COMMAND(ID_TREE_HOME, OnTreeHome)
	ON_CBN_SELCHANGE(IDC_TREE_STACK, OnSelChangeTreeStack)
	ON_CBN_SELCHANGE(IDC_RESOURCE, OnSelchangeResource)
	ON_COMMAND(ID_BACK, OnBack)
	ON_COMMAND(ID_FOOFIGHTER, OnFooFighter)
	ON_COMMAND(ID_FORWARD, OnForward)
	
	ON_COMMAND(ID_CHANGE_PASSWORD, OnChangePassword)
	ON_UPDATE_COMMAND_UI(ID_CHANGE_PASSWORD, OnUpdateChangePassword)
	ON_UPDATE_COMMAND_UI(ID_TREE_UP, OnUpdateTreeUp)
	ON_UPDATE_COMMAND_UI(ID_TREE_HOME, OnUpdateTreeHome)
	ON_UPDATE_COMMAND_UI(ID_FOOFIGHTER, OnUpdateFooFighter)
	ON_UPDATE_COMMAND_UI(ID_VIEW_JUMPVIEWBAR, OnUpdateViewJumpviewbar)
	ON_COMMAND(ID_VIEW_JUMPVIEWBAR, OnViewJumpviewbar)
	//}}AFX_MSG_MAP
	ON_REGISTERED_MESSAGE(msgStatusLine, OnStatusLine)
	ON_REGISTERED_MESSAGE(msgRefillResourceList, OnRefillResourceList)
	ON_REGISTERED_MESSAGE(msgRefresh, OnRefresh)
	ON_REGISTERED_MESSAGE(msgGoAnimation, OnGoAnimation)
	ON_REGISTERED_MESSAGE(msgStopAnimation, OnStopAnimation)
	ON_REGISTERED_MESSAGE(msgDLProgress, OnDLProgress)
	ON_REGISTERED_MESSAGE(msgULProgress, OnULProgress)
	ON_REGISTERED_MESSAGE(msgConnectedUpdate, OnConnectedUpdate)
	ON_REGISTERED_MESSAGE(msgProgressRotate, OnProgressRotate)
END_MESSAGE_MAP()

static UINT indicators[] =
{
ID_STATUS_TEXT,
IDS_STATUS_COUNT,
ID_STATUS_PBAR,
};


extern CMutex gCS;
extern int gCount;
/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	m_isPlaying = false;
	m_finishedInitializing = false;
	m_bProgressEnabled = false;
	
}

CMainFrame::~CMainFrame()
{
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
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndViewsBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_RIGHT
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndViewsBar.LoadToolBar(IDR_LISTVIEWS))
	{
		TRACE0("Failed to create viewsbar\n");
		return -1;      // fail to create
	}

	m_wndViewsBar.SetWindowText("WINI Command Bar");

	if (!m_wndDialogBar.Create(this, IDD_DIALOGBAR1, 
		CBRS_ALIGN_TOP , AFX_IDW_DIALOGBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;		// fail to create
	}

	if (!m_wndDialogBar3.Create(this, IDD_DIALOGBAR3, 
		CBRS_ALIGN_TOP , AFX_IDW_DIALOGBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;		// fail to create
	}
/*
	if (!m_wndDialogBar4.Create(this, IDD_DIALOGBAR4, 
		CBRS_ALIGN_TOP , AFX_IDW_DIALOGBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;		// fail to create
	}
*/
	m_wndAnimate.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 10, 10), this, AFX_IDW_TOOLBAR + 2);
	m_wndAnimate.Open(IDR_AVI1);

	if(	!m_wndReBar.Create(this)
		||!m_wndReBar.AddBar(&m_wndToolBar)
		||!m_wndReBar.AddBar(&m_wndDialogBar, NULL, NULL, RBBS_BREAK | RBBS_GRIPPERALWAYS | RBBS_FIXEDBMP)
		||!m_wndReBar.AddBar(&m_wndDialogBar3)
	//	||!m_wndReBar.AddBar(&m_wndDialogBar4)
		||!m_wndReBar.AddBar(&m_wndAnimate, NULL, NULL, RBBS_FIXEDSIZE | RBBS_FIXEDBMP)
		)
	{
		TRACE0("Failed to create rebar\n");
		return -1;      // fail to create
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

	m_wndViewsBar.EnableDocking(CBRS_ALIGN_ANY);

	EnableDocking(CBRS_ALIGN_ANY);
	
	DockControlBar(&m_wndViewsBar);

	m_finishedInitializing = true;

	return 0;
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/,
	CCreateContext* pContext)
{
	if(!m_wndSplitter.CreateStatic(this, 1, 2))
		return FALSE;

	if(!m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CLeftView), CSize(200, 100), pContext))
		return FALSE;

	if(!m_wndSplitter2.CreateStatic(&m_wndSplitter,2,1, WS_CHILD | WS_VISIBLE | WS_BORDER, m_wndSplitter.IdFromRowCol(0,1)))
		return FALSE;

	if(!m_wndSplitter2.CreateView(0, 0,RUNTIME_CLASS(CMetaView), CSize(100, 100), pContext))
		return FALSE;

	if(!m_wndSplitter2.CreateView(1, 0,RUNTIME_CLASS(CInquisitorView), CSize(100, 100), pContext))
		return FALSE;


	return TRUE;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	//cs.cx = 800;
	//cs.cy = 600;

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

void CMainFrame::SetFocusMeta()
{
	CMetaView* meta = GetMetaPane();
	meta->SetFocus();
	meta->NewMeta();
}

CInquisitorView* CMainFrame::GetRightPane()
{
	CSplitterWnd* pSplit = (CSplitterWnd*)m_wndSplitter.GetPane(0, 1);
	CWnd* pWnd = pSplit->GetPane(1,0);
	CInquisitorView* pView = DYNAMIC_DOWNCAST(CInquisitorView, pWnd);
	return pView;
}

CMetaView* CMainFrame::GetMetaPane()
{
	CSplitterWnd* pSplit = (CSplitterWnd*)m_wndSplitter.GetPane(0, 1);
	CWnd* pWnd = pSplit->GetPane(0,0);
	CMetaView* pView = DYNAMIC_DOWNCAST(CMetaView, pWnd);
	return pView;
}

CLeftView* CMainFrame::GetLeftPane()
{
	CWnd* pWnd = m_wndSplitter.GetPane(0, 0);
	CLeftView* pView = DYNAMIC_DOWNCAST(CLeftView, pWnd);
	return pView;
}

void CMainFrame::OnUpdateViewStyles(CCmdUI* pCmdUI)
{
	int row, col;

	if(NULL == m_wndSplitter2.GetActivePane(&row, &col))
		return;		//if neither inqview or metaview is active

	CListView* pView;

	if(row)
	{
		pView = GetRightPane();
	}else
	{
		pView = GetMetaPane();
	}

	// if the right-hand pane hasn't been created or isn't a view,
	// disable commands in our range

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
}


void CMainFrame::OnViewStyle(UINT nCommandID)
{
	int row, col;

	if(NULL == m_wndSplitter2.GetActivePane(&row, &col))
		return;		//if neither inqview or metaview is active

	CListView* pView;

	if(row)
	{
		pView = GetRightPane();
	}else
	{
		pView = GetMetaPane();
	}


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
				((CMetaView*)pView)->Refresh();
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
	CComboBox* pCBox = (CComboBox*)m_wndDialogBar.GetDlgItem(IDC_TREE_STACK);
	pCBox->SetCurSel(i);
}

int CMainFrame::Push(const char* name)
{
	CComboBox* pCBox = (CComboBox*)m_wndDialogBar.GetDlgItem(IDC_TREE_STACK);

	return pCBox->InsertString(0,name);
}

void CMainFrame::FillResourceList(WINI::INode* node)
{
	if(NULL == node)
		return;

	CComboBox* pCBox = (CComboBox*)m_wndDialogBar3.GetDlgItem(IDC_RESOURCE);

	WINI::INode* child;

	int pos;

	for(int i = 0; i < node->CountChildren(); i++)
	{
		child = node->GetChild(i);

		pos = pCBox->AddString(child->GetName());

		pCBox->SetItemData(pos, (DWORD)child);

		if(child->CountChildren())
			FillResourceList(child);
	}
}

void CMainFrame::OnSelChangeTreeStack() 
{
	CString strItem;

	CComboBox* pCBox = (CComboBox*)m_wndDialogBar.GetDlgItem(IDC_TREE_STACK);

	int nIndex = pCBox->GetCurSel();

	if (nIndex == CB_ERR)
		return;

	pCBox->GetLBText(nIndex, strItem);

	CLeftView* pView = GetLeftPane();

	if (pView != NULL)
		pView->ShowName(strItem);
}

void CMainFrame::ClearStack(bool resourcetoo)
{
	CComboBox* pCBox = (CComboBox*)m_wndDialogBar.GetDlgItem(IDC_TREE_STACK);
	pCBox->ResetContent();

	if(resourcetoo)
	{
		pCBox = (CComboBox*)m_wndDialogBar3.GetDlgItem(IDC_RESOURCE);
		pCBox->ResetContent();
	}
};

LRESULT CMainFrame::OnRefillResourceList(WPARAM wParam, LPARAM lParam)
{
	WINI::INode *node, *selection;
	CInquisitorDoc* pDoc = (CInquisitorDoc*)((CMainFrame*)AfxGetMainWnd())->GetActiveDocument();

	node = NULL;

	selection = pDoc->GetDefaultResource();

	if(wParam)
		if(WINI_ZONE == ((WINI::INode*)wParam)->GetType())
		{
			node = pDoc->GetResources((WINI::IZoneNode*)wParam);
		}else if(selection == (WINI::INode*)lParam)
		{
			selection = (WINI::INode*)wParam;
			pDoc->SetDefaultResource(selection);
		}

	if(!node)
		node = pDoc->GetResources(NULL);



	CComboBox* pCBox = (CComboBox*)m_wndDialogBar3.GetDlgItem(IDC_RESOURCE);
	pCBox->ResetContent();

	FillResourceList(node);
	DWORD val;

	if(selection)
	{
		int i = -1;
		do
		{
			i = pCBox->FindString(i, selection->GetName());
			val = pCBox->GetItemData(i);
		} while((selection != (WINI::INode*)val) && (val != CB_ERR) && (i != CB_ERR));

		if(CB_ERR == i)
			pCBox->SetCurSel(0);
		else
			pCBox->SetCurSel(i);
	}else
	{
		pCBox->SetCurSel(0);
	}

	return 0;
}
 
void CMainFrame::OnTreeUp() 
{
	CLeftView* pView = GetLeftPane();

	if (pView != NULL)
		pView->ShowParent();		
}

void CMainFrame::OnTreeHome() 
{
	CLeftView* pView = GetLeftPane();

	if (pView != NULL)
		pView->ShowHome();
}
	
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

WINI::StatusCode CMainFrame::SetResource(WINI::INode* ResourceORContainer)
{
	if(NULL == ResourceORContainer)
		return WINI_ERROR_INVALID_PARAMETER;

	CComboBox* pCBox = (CComboBox*)m_wndDialogBar3.GetDlgItem(IDC_RESOURCE);

	int i = pCBox->FindString(-1, ResourceORContainer->GetName());

	pCBox->SetCurSel(i);

	return WINI_OK;
}


void CMainFrame::OnSelchangeResource() 
{
	CComboBox* pCBox = (CComboBox*)m_wndDialogBar3.GetDlgItem(IDC_RESOURCE);

	DWORD val = pCBox->GetItemData(pCBox->GetCurSel());

	if(CB_ERR == val)
		return;

	CInquisitorDoc* pDoc = (CInquisitorDoc*)GetActiveDocument();

	ASSERT(pDoc != NULL);

	pDoc->SetDefaultResource((WINI::INode*)val);	
}

LRESULT CMainFrame::OnStatusLine(WPARAM wParam, LPARAM lParam)
{
	char* message = (char*)wParam;

	m_wndStatusBar.SetPaneText(0, message);

	if(lParam)
		free(message);

	return 0;
}

LRESULT CMainFrame::OnRefresh(WPARAM wParam, LPARAM lParam) 
{
	GetActiveDocument()->UpdateAllViews(NULL, lParam);

	return 0;
}

void CMainFrame::OnFooFighter() 
{
CInquisitorDoc* pDoc = (CInquisitorDoc*)GetActiveDocument();
pDoc->Disconnect();
pDoc->MyNewConnection();
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



LRESULT CMainFrame::OnConnectedUpdate(WPARAM wParam, LPARAM lParam)
{
	CLeftView* view = GetLeftPane();
	view->ConnectedUpdate(wParam, lParam);

	return TRUE;
}

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



void CMainFrame::OnChangePassword() 
{
	CInquisitorDoc* pDoc = (CInquisitorDoc*)GetActiveDocument();

	if(NULL == pDoc)
		return;

	CGenericDialog myDlg("L");

	WINI::StatusCode status;

	if(IDOK == myDlg.DoModal())
	{
		status = pDoc->ChangePassword(myDlg.m_Edit.LockBuffer());
		myDlg.m_Edit.UnlockBuffer();
	}

	char buf[256];

	if(status.isOk())
		sprintf(buf, "password change succeeded.");
	else
		sprintf(buf, "couldn't change password: %s", status.GetError());

	SetStatusMessage(buf);
}

void CMainFrame::OnUpdateChangePassword(CCmdUI* pCmdUI) 
{
	CInquisitorDoc* pDoc = (CInquisitorDoc*)GetActiveDocument();

	if(NULL != pDoc)
		if(pDoc->isConnected())
			if(!gbIsDemo)
				return;

	pCmdUI->Enable(FALSE);
}

void CMainFrame::OnUpdateFooFighter(CCmdUI* pCmdUI) 
{
	


	pCmdUI->Enable(TRUE);
}

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

void CMainFrame::OnUpdateTreeHome(CCmdUI* pCmdUI) 
{

	CInquisitorDoc* pDoc = (CInquisitorDoc*)GetActiveDocument();

	if(pDoc->isConnected())
		if(pDoc->GetHome())
			return;

	pCmdUI->Enable(FALSE);	

}

void CMainFrame::OnUpdateViewJumpviewbar(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck((m_wndViewsBar.GetStyle() & WS_VISIBLE) != 0);
}

void CMainFrame::OnViewJumpviewbar() 
{
	BOOL bVisible = ((m_wndViewsBar.GetStyle() & WS_VISIBLE) != 0);

	ShowControlBar(&m_wndViewsBar, !bVisible, FALSE);
	RecalcLayout();	
}

void CMainFrame::OnClose() 
{
	if(IDYES == AfxMessageBox("Are you sure you want to exit this program?", MB_YESNO))
	{
		gbExiting;
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

