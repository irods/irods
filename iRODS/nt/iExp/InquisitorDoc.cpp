// InquisitorDoc.cpp : implementation of the CInquisitorDoc class
//
#include "stdafx.h"
#include "Inquisitor.h"
#include "irodsUserEnv.h"
#include "InquisitorDoc.h"
#include "Login.h"
#include "MainFrm.h"
#include "Query.h"
#include "WorkerThread.h"
#include "winiCollectionOperator.h"
#include "AccessCtrl.h"
#include "winiStatusCode.h"
#include "InquisitorView.h"
#include "irodsWinUtils.h"
#include "irodsWinCollection.h"
#include "PreferenceDlg.h"
#include "DispTextDlg.h"
#include "ChangePasswdDlg.h"
#include "SubmitIRodsRuleDlg.h"
#include "RuleStatusDlg.h"
#include "MetaSearchDlg.h"
#include "DialogWebDisp.h"
#include "irodsWinProgressCB.h"

// irods h files

//#include "DS2Dialog.h"
//#include "DSThread.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CInquisitorApp theApp;
extern CRITICAL_SECTION gLock;
//extern int gCount;
//extern CMutex gCS;
//extern std::vector<WINI::INode*> gNodeDragList;
extern unsigned int gOn;
extern HANDLE ghDragList;
extern CEvent killWorker;

int mystupidcount;
HWND mymainwindow;

/////////////////////////////////////////////////////////////////////////////
// CInquisitorDoc

IMPLEMENT_DYNCREATE(CInquisitorDoc, COleDocument)


BEGIN_MESSAGE_MAP(CInquisitorDoc, COleDocument)
	//{{AFX_MSG_MAP(CInquisitorDoc)
	//ON_COMMAND(ID_QUERY, OnQuery)
	//ON_UPDATE_COMMAND_UI(ID_BACK, OnUpdateBack)
	//ON_UPDATE_COMMAND_UI(ID_FORWARD, OnUpdateForward)
	//ON_UPDATE_COMMAND_UI(ID_FILE_NEW, OnUpdateFileNew)
	ON_COMMAND(ID_TREE_UP, OnTreeUp)
	//ON_COMMAND(ID_REFRESH, OnRefresh)
	//ON_UPDATE_COMMAND_UI(ID_REFRESH, OnUpdateRefresh)
	//}}AFX_MSG_MAP
	ON_UPDATE_COMMAND_UI(ID_QUERY, &CInquisitorDoc::OnUpdateQuery)
	ON_COMMAND(ID_USER_INFO, &CInquisitorDoc::OnUserInfo)
	//ON_COMMAND(ID_PREFERENCE_ALWAYSOVERRIDEWHENUPLOADINGFILE, &CInquisitorDoc::OnPreferenceAlwaysoverridewhenuploadingfile)
	ON_COMMAND(ID_IRODS_PREFERENCES, &CInquisitorDoc::OnIrodsPreferences)
	ON_COMMAND(ID_USERACCOUNT_CHANGEPASSWORD, &CInquisitorDoc::OnUseraccountChangepassword)
	ON_COMMAND(ID_RULE_SUBMITANIRODSRULE, &CInquisitorDoc::OnRuleSubmitanirodsrule)
	ON_COMMAND(ID_RULE_CHECKSTATUSOFPENDINGRULE, &CInquisitorDoc::OnRuleCheckstatusofpendingrule)
	ON_COMMAND(ID_RULE_DELETEPENDINGRULE, &CInquisitorDoc::OnRuleDeletependingrule)
	//ON_COMMAND(ID_SEARCH_METADATASEARCH, &CInquisitorDoc::OnSearchMetadatasearch)
	ON_COMMAND(ID_EDIT_SEARCH, &CInquisitorDoc::OnEditSearch)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInquisitorDoc construction/destruction

CInquisitorDoc::CInquisitorDoc()
{
	//m_current_node = NULL;
	//m_defaultResource = NULL;
	m_HistoryLast = -1;
	m_HistoryFirst = -1;
	m_HistoryCurrent = -1;

#if 0
	for(int i = 0; i < 10; i++)
		m_History[i] = NULL;
#endif
	
	m_synchFlag = 0;
	
	mystupidcount = 0;

	// initialize m_irodsUserEnv
	m_irodsUserEnv.irodsHost = "";
	m_irodsUserEnv.irodsPort = "";
	m_irodsUserEnv.irodsUserName = "";
	m_irodsUserEnv.irodsUserPasswd = "";
	m_irodsUserEnv.irodsZone = "";

	upload_always_override = false;
	paste_always_override = false;
	delete_obj_permanently = false;

	profile_id_from_login = -1;

	local_temp_dir = "";
}

void CInquisitorDoc::SetSynchronizationFlags(bool purge, bool primary_only)
{
	m_synchFlag = 0;

//	if(purge)
//		m_synchFlag |= PURGE_FLAG;
//	if(primary_only)
//		m_synchFlag |= PRIMARY_FLAG;
}

#if 0
WINI::StatusCode CInquisitorDoc::ModifyAccess(WINI::IAccessNode* target, const char* permission, bool recursive)
{
	WINI::IDatasetOperator* dOp;
	WINI::ICollectionOperator* cOp;

	switch(target->GetParent()->GetType())
	{
	case WINI_DATASET:
		dOp = (WINI::IDatasetOperator*)MyImpl.GetOperator(WINI_DATASET);
		return dOp->ModifyAccess(target, permission);
		break;
	case WINI_COLLECTION:
		cOp = (WINI::ICollectionOperator*)MyImpl.GetOperator(WINI_COLLECTION);
		return cOp->ModifyAccess(target, permission, recursive);
		break;
	}

	return WINI_ERROR_INVALID_PARAMETER;
}
#endif

CInquisitorDoc::~CInquisitorDoc()
{
	
}

void CInquisitorDoc::OnBack()
{
	if(m_HistoryCurrent == m_HistoryFirst)
		return;

	if(m_HistoryCurrent != m_HistoryFirst)	//can I go back farther in my history?
	{
		if(-1 == --m_HistoryCurrent)
			m_HistoryCurrent = 9;
		//m_current_node = m_History[m_HistoryCurrent];
	}
}

void CInquisitorDoc::OnForward()
{
	if(m_HistoryCurrent == m_HistoryLast)
		return;

	if(m_HistoryCurrent != m_HistoryLast)
	{
		m_HistoryCurrent = ++m_HistoryCurrent % 10;
		//m_current_node = m_History[m_HistoryCurrent];
	}
}

bool CInquisitorDoc::Disconnect()
{
#if 0
	if(0 == gCS.Lock(5))
	{
		AfxMessageBox("WINI is busy performing another action.");
		return false;
	}
#endif
	int foo;
	foo = 1;
	//m_current_node = NULL;
	UpdateAllViews(NULL,3);

	
#if 0
	if(!gCS.Unlock())
	{
		AfxMessageBox("Couldn't unlock working thread");
		return false;
	}
#endif

	return true;
}

struct ConnectStruct
{
	const char* name;
	const char* home;
	const char* host;
	const char* dhome;
	const char* port;
	const char* pass;
	const char* resource;
	const char* zone;
	//WINI::SessionImpl* connection;
	HWND handle;
};

#if 0
UINT ConnectToWINI(LPVOID pParam)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
	ConnectStruct* c = (ConnectStruct*)pParam;

	::PostMessage(c->handle, msgGoAnimation, NULL, NULL);
	::SendMessage(c->handle, msgStatusLine, (WPARAM)"beginning connect", 0);
	WINI::StatusCode status = c->connection->Init(c->host, atoi(c->port), c->name, c->zone, c->pass);
	::SendMessage(c->handle, msgStatusLine, (WPARAM)"finished connect", 0);
	::PostMessage(c->handle, msgConnectedUpdate, status.GetCode(), NULL);
	delete c;
	
	return 0;
}
#endif

char* CInquisitorDoc::GetTempPath()
{
	return ((CInquisitorApp*)AfxGetApp())->m_szTempPath;
}



BOOL CInquisitorDoc::OnNewDocument()
{
	/*For VS6, document was created after creation of main window frame.
	For VS2005, document is created before frame creation as AfxGetMainWnd()
	returns NULL. Hence, login process was moved.*/

 	if (!CDocument::OnNewDocument())
		return FALSE;

#if 0
	if(MyImpl.isConnected())
		if(!Disconnect())
		{
			AfxMessageBox("failed to disconnect");
			return FALSE;
		}
#endif

	return TRUE;

}

BOOL CInquisitorDoc::MyNewConnection()
{
	CLogin m_Login;

	int status = m_Login.DoModal();

	switch(status)
	{
	case IDOK:
		m_eventConnecting = true;
		//m_current_node = NULL;
		m_HistoryLast = -1;
		m_HistoryFirst = -1;
		m_HistoryCurrent = -1;
		break;
	case IDCANCEL:
		//AfxGetMainWnd()->PostMessage(WM_QUIT);	//initial doesn't need this, but subsequent calls, where mainframe has already been drawn, need this
		return FALSE;
		break;
	default:
		AfxMessageBox("Error!", MB_OK);
		return FALSE;
	}

	
	// passing the conn object to the document.
	CInquisitorApp* pApp = (CInquisitorApp *)AfxGetApp();
	this->conn = pApp->conn;    
	this->passwd = m_Login.m_szPassword;

	this->default_resc_from_login = m_Login.m_szDefaultResource;
	this->profile_id_from_login = m_Login.m_nProfileId;

	pApp->conn = NULL;

	/*
	ConnectStruct* c = new ConnectStruct;
	c->name = m_Login.GetMyName();
	c->home = m_Login.GetHome();
	c->host = m_Login.GetHost();
	c->zone = m_Login.GetZone();
	c->dhome = m_Login.GetDomainHome();
	c->port = m_Login.GetPort();
	c->pass = m_Login.GetPassword();
	c->resource = m_Login.GetDefaultResource();
	CWnd* mystarpointer = theApp.m_pMainWnd;
	c->handle = mystarpointer->m_hWnd;
	c->connection = &MyImpl;
	*/

	//char sztitle[256];

	//this->SetTitle("Not Connected");
	//frame->SetStatusMessage(sztitle);
	//m_pCThread = AfxBeginThread(ConnectToWINI, c, THREAD_PRIORITY_NORMAL, 0,CREATE_SUSPENDED);
	//::DuplicateHandle(GetCurrentProcess(), m_pCThread->m_hThread, GetCurrentProcess(), &m_hCThread, 0, FALSE, DUPLICATE_SAME_ACCESS);
	//m_pCThread->ResumeThread();
	//::Sleep(0);
	return TRUE;
}

#if 0
void CInquisitorDoc::OnFailedConnection()
{
	m_eventConnecting = false;
	AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_FILE_NEW, NULL);
}
#endif

#if 0
BOOL CInquisitorDoc::OnCompletedConnection()
{
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	
	return TRUE;
}
#endif

void CInquisitorDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

#ifdef _DEBUG
void CInquisitorDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CInquisitorDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

#if 0
WINI::INode* CInquisitorDoc::GetDomains()
{
	return NULL;
}
#endif

#if 0
WINI::INode* CInquisitorDoc::GetRoot()
{
	return MyImpl.GetRoot(m_zone);
}
#endif

#if 0
WINI::INode* CInquisitorDoc::GetLocalRoot()
{
	return MyImpl.GetLocalRoot();
}
#endif

#if 0
WINI::INode* CInquisitorDoc::GetHome()
{
	return MyImpl.GetHome(m_zone);
}
#endif

#if 0
WINI::INode* CInquisitorDoc::GetMCAT()
{
	return MyImpl.GetCatalog();
}
#endif

#if 0
WINI::INode* CInquisitorDoc::GetHomeZone()
{
	return (WINI::INode*)MyImpl.GetHomeZone();
}
#endif


#if 0
WINI::StatusCode CInquisitorDoc::OpenTree(WINI::INode* node, int levels, bool clear, unsigned int mask)
{
	open_node mystruct;

	mystruct.node = node;
	mystruct.levels = levels;
	mystruct.clear = clear;
	mystruct.mask = mask;

	m_worker_thread->PostThreadMessage(msgOpen, (WPARAM)&mystruct, NULL);
	m_worker_thread->ResumeThread();

	return 0;
}

WINI::StatusCode CInquisitorDoc::OpenTree(WINI::INode* node, int levels, bool clear, unsigned int mask)
{
	return MyImpl.OpenTree(node, levels, clear, mask);
}
#endif

#if 0
void CInquisitorDoc::SetCurrentNode(WINI::INode* node)
{
	if(node == NULL || node == m_current_node)
		return;

	m_current_node = node;

	if(-1 == m_HistoryFirst)
	{
		ASSERT(m_HistoryLast == -1);
		ASSERT(m_HistoryCurrent == -1);
		m_HistoryFirst = 0;
		m_HistoryLast = 0;
		m_HistoryCurrent = 0;
	}else
	{
		if(10 == ++m_HistoryCurrent)
		{
			m_HistoryCurrent = 0;
			m_HistoryFirst++;
		}

		m_HistoryLast = m_HistoryCurrent;
	}

	m_History[m_HistoryCurrent] = node;
}
#endif

#if 0
WINI::IStringNode* CInquisitorDoc::GetAccessConstraint()
{
	return MyImpl.GetAccessConstraints();
}
#endif

#if 0
WINI::INode* CInquisitorDoc::GetCurrentNode()
{
	return m_current_node;
}
#endif

#if 0
void CInquisitorDoc::OnQuery() 
{
	AfxMessageBox("This function is under development!");
	return;

	CQuery MyQuery(&MyImpl, m_current_node);

	WINI::StatusCode status;

	if(IDOK == MyQuery.DoModal())
	{
		CMainFrame* mainframe = (CMainFrame*)AfxGetMainWnd();
		mainframe->OnGoAnimation(NULL, NULL);
		BeginWaitCursor();
		status = MyImpl.OpenTree(MyQuery.m_query, 1, true);
		EndWaitCursor();
		mainframe->OnStopAnimation(NULL, NULL);

		if(status.isOk())
		{
			SetCurrentNode(MyQuery.m_query);
			UpdateAllViews(NULL, 1);
		}else
			((CMainFrame*)AfxGetMainWnd())->SetStatusMessage(status.GetError());
	}

}
#endif

#if 0
void CInquisitorDoc::SetComment(WINI::INode* node, char* comment)
{
	if(NULL == node)
		return;

	if(NULL == comment)
		return;
}
#endif

#if 0
void CInquisitorDoc::Delete(WINI::INode* node)
{
	if(m_current_node == node)
	{
		m_current_node = node->GetParent();
		ASSERT(m_current_node != NULL);
	}

	m_HistoryLast = -1;
	m_HistoryFirst = -1;
	m_HistoryCurrent = -1;
}
#endif

#if 0
void CInquisitorDoc::Rename(WINI::INode* node, char* name)
{
	if(m_current_node == node)
	{
		m_current_node = node->GetParent();
		ASSERT(m_current_node != NULL);
	}

	char* new_name = strdup(name);

}
#endif

void CInquisitorDoc::SelectLocalFolder(CString & locaDir)
{
	char buffer[MAX_PATH];
	LPITEMIDLIST itemids;
	char* folder_path;
	LPMALLOC shallocator;

	BROWSEINFO bwinfo;
	FillMemory((void*)&bwinfo,sizeof(bwinfo),0);
	bwinfo.hwndOwner=NULL;
	bwinfo.pidlRoot=NULL;
	bwinfo.pszDisplayName=(char*)buffer;
	bwinfo.lpszTitle="Place downloads in folder:";
	bwinfo.ulFlags=0;
	itemids = SHBrowseForFolder(&bwinfo); 

	if(NULL == itemids)
		return;

	folder_path = new char[MAX_PATH];
	SHGetPathFromIDList(itemids,(char*) folder_path);
	SHGetMalloc(&shallocator);
	shallocator->Free((void*) itemids);
	shallocator->Release();

	locaDir = folder_path;
}

#if 0
void CInquisitorDoc::Download(WINI::INode* node, const char* local_path, bool Execute)
{
	dl* mydl = new dl;

	mydl->node = node;
	mydl->local_path = strdup(local_path);

	WPARAM wParam = 0;

	if(Execute)
		wParam = 1;
}
#endif

#if 0
void CInquisitorDoc::Download(WINI::INode* node, bool Execute)
{
	dl* mydl = new dl;

	mydl->node = node;
	mydl->local_path = strdup(((CInquisitorApp*)AfxGetApp())->m_szTempPath);

	WPARAM wParam = 0;

	if(Execute)
		wParam = 1;
}
#endif

#if 0
void CInquisitorDoc::Copy(WINI::INode* target, WINI::INode* source, bool cut)
{
	copy_op* params = new copy_op;
	params->target = target;
	params->node = source;
	params->resource = m_defaultResource;

	if(cut)
		params->delete_original = true;
	else
		params->delete_original = false;

	if(m_current_node == source)
	{
		m_current_node = target;
	}
}
#endif

#if 0
void CInquisitorDoc::Copy(WINI::INode* node, int type, bool cut, char* name, char* path, int replication_index)
{
	WINI::INode* root = MyImpl.GetRoot(m_zone);

	char* adjusted_path;

	int len = strlen(path);

	int i;
	for(i = 1; i < len; i++)
	{
		if(path[i] == '/')
		{
			adjusted_path = &path[i];
			break;
		}
	}

	//root is /home so any path should also adjust.

	adjusted_path += (5*sizeof(char));

	ASSERT(len != i);

	WINI::INode* blah;
	root->GetChild(adjusted_path, &blah);
	int repl;

	if(WINI_DATASET == blah->GetType())
	{
		repl = atoi(((WINI::IDatasetNode*)blah)->GetReplicantNumber());
		if(replication_index != repl)
		{
			blah = ((WINI::IDatasetNode*)blah)->GetReplicant(replication_index);
		}
	}

	char string[1024];

	if(NULL == blah)
	{
		sprintf(string, "could not find '%s'.", path);
		AfxMessageBox(string);
		return;
	}

	copy_op* params = new copy_op;
	params->target = node;
	params->node = blah;
	params->resource = m_defaultResource;

	if(cut)
		params->delete_original = true;
	else
		params->delete_original = false;
}
#endif

#if 0
WINI::StatusCode CInquisitorDoc::AddNewMetadata(const char* attribute, const char* value)
{
	if(attribute == NULL)
		return WINI_ERROR_INVALID_PARAMETER;

	if(!MyImpl.isConnected())
		return WINI_ERROR;

	WINI::IDatasetOperator* dOp;
	WINI::ICollectionOperator* cOp;

	if(!m_current_node)
		return WINI_OK;

	WINI::IMetadataNode* metaNode;

	WINI::StatusCode status;

	switch(m_current_node->GetType())
	{
	case WINI_DATASET:
		dOp = (WINI::IDatasetOperator*)MyImpl.GetOperator(m_current_node->GetType());
		status = dOp->AddMeta((WINI::IDatasetNode*)m_current_node, attribute, &metaNode);
		if(status.isOk())
			status = dOp->ModifyMeta(metaNode, value);
		break;
	case WINI_COLLECTION:
		cOp = (WINI::ICollectionOperator*)MyImpl.GetOperator(m_current_node->GetType());
		status = cOp->AddMeta((WINI::ICollectionNode*)m_current_node, attribute, &metaNode);
		if(status.isOk())
			status = cOp->ModifyMeta(metaNode, value);
		break;
	default:
		return WINI_ERROR_TYPE_NOT_SUPPORTED;
	}

	if(status.isOk())
		UpdateAllViews(NULL, 2);
	else
		AfxMessageBox(status.GetError());

	return WINI_OK;
}
#endif

#if 0
WINI::StatusCode CInquisitorDoc::SetMetadataValue(const char* attribute, const char* value)
{
	if(value == NULL)
		return WINI_ERROR_INVALID_PARAMETER;

	WINI::IDatasetOperator* dOp;
	WINI::ICollectionOperator* cOp;
	WINI::INode* blah;
	WINI::IMetadataNode* metadata = NULL;

	if(!m_current_node)
		return WINI_OK;
	int i;
	for(i = 0; i < m_current_node->CountChildren(); i++)
	{
		blah = m_current_node->GetChild(i);

		if(blah->GetType() == WINI_METADATA)
		{
			if(0 == strcmp(attribute,((WINI::IMetadataNode*)blah)->GetAttribute()))
			{
				metadata = (WINI::IMetadataNode*)blah;
				break;
			}
		}
	}

	if(NULL == metadata)
		return WINI_ERROR;

	WINI::StatusCode status;

	switch(m_current_node->GetType())
	{
	case WINI_DATASET:
		dOp = (WINI::IDatasetOperator*)MyImpl.GetOperator(m_current_node->GetType());
		status = dOp->ModifyMeta(metadata, value);
		break;
	case WINI_COLLECTION:
		cOp = (WINI::ICollectionOperator*)MyImpl.GetOperator(m_current_node->GetType());
		status = cOp->ModifyMeta(metadata, value);
		break;
	default:
		return WINI_ERROR_TYPE_NOT_SUPPORTED;
	}

	if(status.isOk())
		UpdateAllViews(NULL, 2);
	else
		AfxMessageBox(status.GetError());

	return status;
}
#endif

#if 0
WINI::StatusCode CInquisitorDoc::SetMetadataValue(WINI::IMetadataNode* node, const char* value)
{
	if(value == NULL || node == NULL)
		return WINI_ERROR_INVALID_PARAMETER;

	WINI::IDatasetOperator* dOp;
	WINI::ICollectionOperator* cOp;
	WINI::StatusCode status;

	switch(m_current_node->GetType())
	{
	case WINI_DATASET:
		dOp = (WINI::IDatasetOperator*)MyImpl.GetOperator(m_current_node->GetType());
		status = dOp->ModifyMeta(node, value);
		break;
	case WINI_COLLECTION:
		cOp = (WINI::ICollectionOperator*)MyImpl.GetOperator(m_current_node->GetType());
		status = cOp->ModifyMeta(node, value);
		break;
	default:
		return WINI_ERROR_TYPE_NOT_SUPPORTED;
	}

	if(status.isOk())
		UpdateAllViews(NULL, 2);
	else
		AfxMessageBox(status.GetError());

	return status;
}
#endif

#if 0
void CInquisitorDoc::Upload(WINI::INode* node, const char* local_path)
{
	dl* mydl = new dl;

	mydl->node = node;
	mydl->local_path = strdup(local_path);

	mydl->binding = m_defaultResource;
}
#endif

#if 0
WINI::ISetNode* CInquisitorDoc::GetResources(WINI::IZoneNode* node)
{
	return MyImpl.GetResource(node);
}
#endif

#if 0
void CInquisitorDoc::Replicate(WINI::INode* node)
{
	new_replicant* blah = new new_replicant;
	blah->node = node;
	blah->binding = m_defaultResource;
}
#endif

#if 0
void CInquisitorDoc::CreateNode(WINI::INode* parent, const char* name)
{
	new_node* blah = new new_node;

	blah->parent = parent;

	blah->name = strdup(name);
}
#endif

#if 0
WINI::StatusCode CInquisitorDoc::SetDefaultResource(WINI::INode* node)
{
	if(NULL == node)
		return WINI_ERROR_INVALID_PARAMETER;

	if(WINI_RESOURCE != node->GetType())
		return WINI_ERROR_INVALID_PARAMETER;

	m_defaultResource = node;

	
	return WINI_OK;
}
#endif

void CInquisitorDoc::SetAccess(CString & irodsPath, int obj_type)
{
	CAccessCtrl Access(this, irodsPath, obj_type, AfxGetMainWnd());
	Access.DoModal();
	//this->UpdateAllViews(NULL,2);
}

//extern bool g_bIsDragging;
//extern UINT gInqClipboardFormat;
//extern bool gbExiting;
#if 0
CharlieSource::~CharlieSource()
{


}
#endif

#if 0
BOOL CharlieSource::OnRenderData(LPFORMATETC lpFormatEtc, LPSTGMEDIUM lpStgMedium)
{

	TRACE("OnRenderData\n");
	if(gbExiting)
		return FALSE;
	TRACE("OnRenderData2\n");

	if(COleDataSource::OnRenderData(lpFormatEtc, lpStgMedium))
		return TRUE;

	TRACE("OnRenderData2.5\n");
	SHORT nLButtonPressed = ::GetAsyncKeyState(::GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON);

	if(0 > nLButtonPressed)
		return FALSE;	//Left mouse button is still pressed down - don't do anything yet

	TRACE("OnRenderData3\n");
	((CMainFrame*)AfxGetMainWnd())->OnGoAnimation(NULL, NULL);

	if(g_bIsDragging)
	{
		if(lpFormatEtc->cfFormat == CF_HDROP && lpFormatEtc->tymed & TYMED_HGLOBAL)
		{
			return RenderFileData(lpFormatEtc, lpStgMedium);
		}

		//should never get here
		ASSERT(1);
	}
	else
	{
		//we are being called to render data for a file paste to an outside application
		return RenderInqData(lpFormatEtc, lpStgMedium);
	}

	return FALSE;
}
#endif

#if 0
BOOL CharlieSource::RenderFileData(LPFORMATETC lpFormatEtc, LPSTGMEDIUM lpStgMedium)
{
	TRACE("RenderFile\n");



	TRACE("RenderFile2\n");

	if(lpStgMedium->tymed == TYMED_NULL)
	{
		TRACE("RenderFile3\n");

		if(lpFormatEtc->tymed & TYMED_HGLOBAL)	//stgmedium not preallocated
		{
			TRACE("RenderFile5\n");
			BeginWaitCursor();

		

			CMainFrame* blah = (CMainFrame*)(AfxGetApp()->GetMainWnd());

			TRACE("Render7\n");
			if(NULL == blah)
				return FALSE;

			CInquisitorDoc* pDoc = (CInquisitorDoc*)blah->GetActiveDocument();

	


			TRACE("HHHHHHHH\n");
			WINI::StatusCode status = pDoc->DragOutDownload(lpStgMedium);


			if(!status.isOk())
			{
				TRACE("R8\n");
				AfxGetMainWnd()->PostMessage(msgStopAnimation, NULL, NULL);
				return FALSE;
			}

			AfxGetMainWnd()->PostMessage(msgStopAnimation, NULL, NULL);
			return TRUE;
		}else
		{
			TRACE("RenderFile6\n");
		}
	}else
	{
		TRACE("RenderFile4\n");

		//stgmedium preallocated
		if(lpStgMedium->tymed == TYMED_HGLOBAL)
		{
			TRACE("Render9\n");
			//render data into global memory block with handle stored in lpstgmedium->hGlobal
			ASSERT(1);
			AfxGetMainWnd()->PostMessage(msgStopAnimation, NULL, NULL);
			return FALSE;
		}
	}

	return FALSE;
}
#endif


#if 0
WINI::StatusCode CInquisitorDoc::DragOutDownload(LPSTGMEDIUM lpStgMedium)
{
	gOn++;


	TRACE("DragoutDown\n");
	//TRACE("Current Thread ID = 0x%X\tParam Thread: 0x%X\n", AfxGetThread()->m_nThreadID, thread);

	ASSERT(NULL != ((CInquisitorApp*)AfxGetApp())->m_szTempPath);

	int path_len = strlen(((CInquisitorApp*)AfxGetApp())->m_szTempPath);

	int count = gNodeDragList.size();

	//ASSERT(count > 0);
		
	WINI::INode* node;

	int char_count = 0;

	WINI::StatusCode status;

	int i;
	for(i = 0; i < count; i++)
	{
		node = gNodeDragList.at(i);
		VERIFY(NULL != node);
			

		if(1 == gOn)
		{
			TRACE("DOWNLOADING\n");
			status = ((WINI::ITransferOperator*)MyImpl.GetOperator(node->GetType()))->Download(node, ((CInquisitorApp*)AfxGetApp())->m_szTempPath);
		}


		TRACE("Transfer Status: %d\n", status.GetCode());
		if(!status.isOk())
			return status;

		char_count += strlen(node->GetName());

	}

#if 1
	/*CHARLIE CHARLIE this section is what i just commented out - please comment in again to make inQ do DnD.*/
	TRACE("dragoutdown2\n");
	char_count += (count * strlen(((CInquisitorApp*)AfxGetApp())->m_szTempPath)) + count + 1;

	int nSize = sizeof(DROPFILES) + (sizeof(TCHAR) * char_count);
	HANDLE hData = ::GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, nSize);
	LPDROPFILES pDropFiles = (LPDROPFILES) ::GlobalLock(hData);
	pDropFiles->pFiles = sizeof(DROPFILES);


#if 1
	#ifdef UNICODE
	pDropFiles->fWide = TRUE;
	#else
	pDropFiles->fWide = FALSE;
	#endif
#endif

	LPBYTE pData = (LPBYTE)pDropFiles + sizeof(DROPFILES);

	const char* name;
	int len;

	int sizeofpath = sizeof(TCHAR) * path_len;

	for(i = 0; i < count; i++)
	{
		::CopyMemory(pData, _T(((CInquisitorApp*)AfxGetApp())->m_szTempPath), sizeofpath);
		pData += sizeofpath;
		name = gNodeDragList.at(i)->GetName();
		len = sizeof(TCHAR) * (strlen(name) + 1);
		::CopyMemory(pData, _T(name), len);
		pData += len;
	}

	::GlobalUnlock(hData);
	lpStgMedium->hGlobal = hData;


	lpStgMedium->tymed = TYMED_HGLOBAL;

#endif
	EndWaitCursor();

	//turning off the clear made this work, but it screws up subsequent drag outs.
	//also we turned off a count 

	//gNodeDragList.clear();		

	return WINI_OK;
}
#endif

#if 0
BOOL CharlieSource::RenderInqData(LPFORMATETC lpFormatEtc, LPSTGMEDIUM lpStgMedium)
{
	ASSERT(ghDragList != NULL);

	void* ptr = ::GlobalLock(ghDragList);

	if(NULL == ptr)
	{
		//assume that this is because of a 2nd or 3rd call to RenderInqData(Explorer does this more than once)
		AfxGetMainWnd()->PostMessage(msgStopAnimation, NULL, NULL);
		return TRUE;
	}

	int* repl = (int*)ptr;
	char* szptr = (char*)ptr + sizeof(int);
	int len;

	int type;
	bool cut;
	char* path;
	char* name;

	WINI::INode *pppp, *root;

	CMainFrame* blah = (CMainFrame*)AfxGetMainWnd();

	if(NULL == blah)
	{
		ASSERT(1);
		::GlobalUnlock(ghDragList);
		::GlobalFree(ghDragList);
		AfxGetMainWnd()->PostMessage(msgStopAnimation, NULL, NULL);
		return FALSE;
	}

	CInquisitorDoc* pDoc = ((CInquisitorDoc*)blah->GetActiveDocument());
		
	ASSERT(pDoc != NULL);

	root = pDoc->GetRoot();
	char* adjusted_path;
	int len2;
	int myrepl;

	do
	{
		if('C' == szptr[0])
			type = WINI_COLLECTION;
		else
			type = WINI_DATASET;

		len = strlen(szptr);
		szptr += len + 1;
		ASSERT(szptr != NULL);

		if('Y' == szptr[0])
			cut = true;
		else
			cut = false;

		len = strlen(szptr);
		szptr += len + 1;
		ASSERT(szptr != NULL);

		name = szptr;

		len = strlen(szptr);
		szptr += len + 1;
		ASSERT(szptr != NULL);

		path = szptr;
		len2 = strlen(path);

		int z = 0;
		int i;
		for(i = 1; i < len2; i++)
		{
			if(path[i] == '/')
			{
				z++;
				adjusted_path = &path[i];
				if(z == 2)
					break;
			}
		}

		
		ASSERT(len2 != i);

		root->GetChild(adjusted_path, &pppp);
		
		if(WINI_DATASET == pppp->GetType())
		{
			myrepl = atoi(((WINI::IDatasetNode*)pppp)->GetReplicantNumber());
			if(*repl != myrepl)
			{
				pppp = ((WINI::IDatasetNode*)pppp)->GetReplicant(*repl);
			}
		}

		if(pppp)
			m_NodeDragList.push_back(pppp);

		len = strlen(szptr);
		szptr += len + 1;

		ptr = szptr;
		repl = (int*)ptr;
		szptr = (char*)ptr + sizeof(int);

	} while(szptr[0] != '\0');

	::GlobalUnlock(ghDragList);
	//::GlobalFree(ghDragList);	this is for some reason causing Win9X explorer to crash! maybe it's freeing already freed memory

	if(DownloadForCopy(lpStgMedium).isOk())
	{
		AfxGetMainWnd()->PostMessage(msgStopAnimation, NULL, NULL);
		return TRUE;
	}

	AfxGetMainWnd()->PostMessage(msgStopAnimation, NULL, NULL);
	return FALSE;
}
#endif

#if 0
WINI::StatusCode CharlieSource::DownloadForCopy(LPSTGMEDIUM lpStgMedium)
{
	ASSERT(NULL != ((CInquisitorApp*)AfxGetApp())->m_szTempPath);

	int path_len = strlen(((CInquisitorApp*)AfxGetApp())->m_szTempPath);

	CMainFrame* blah = (CMainFrame*)AfxGetMainWnd();

	if(NULL == blah)
	{
		ASSERT(1);
		::GlobalUnlock(ghDragList);
		::GlobalFree(ghDragList);
		return WINI_ERROR;
	}

	CInquisitorDoc* pDoc = ((CInquisitorDoc*)blah->GetActiveDocument());
		
	ASSERT(pDoc != NULL);

	int count = m_NodeDragList.size();

	ASSERT(count > 0);

	WINI::INode* node;

	int char_count = 0;

	WINI::StatusCode status;

	int i;
	for(i = 0; i < count; i++)
	{
		node = m_NodeDragList.at(i);
		VERIFY(NULL != node);
		status = ((WINI::ITransferOperator*)pDoc->MyImpl.GetOperator(node->GetType()))->Download(node, ((CInquisitorApp*)AfxGetApp())->m_szTempPath);
		
		if(!status.isOk())
			return status;

		char_count += strlen(node->GetName());
	}

	char_count += (count * strlen(((CInquisitorApp*)AfxGetApp())->m_szTempPath)) + count + 1;

	int nSize = sizeof(DROPFILES) + (sizeof(TCHAR) * char_count);
	HANDLE hData = ::GlobalAlloc(GHND, nSize);
	LPDROPFILES pDropFiles = (LPDROPFILES) ::GlobalLock(hData);
	pDropFiles->pFiles = sizeof(DROPFILES);

	#ifdef UNICODE
	pDropFiles->fWide = TRUE;
	#else
	pDropFiles->fWide = FALSE;
	#endif

	LPBYTE pData = (LPBYTE)pDropFiles + sizeof(DROPFILES);

	const char* name;
	int len;

	int sizeofpath = sizeof(TCHAR) * path_len;

	for(i = 0; i < count; i++)
	{
		::CopyMemory(pData, _T(((CInquisitorApp*)AfxGetApp())->m_szTempPath), sizeofpath);
		pData += sizeofpath;
		name = m_NodeDragList.at(i)->GetName();
		len = sizeof(TCHAR) * (strlen(name) + 1);
		::CopyMemory(pData, _T(name), len);
		pData += len;
	}

	::GlobalUnlock(hData);
	lpStgMedium->hGlobal = hData;

	lpStgMedium->tymed = TYMED_HGLOBAL;
	m_NodeDragList.clear();
	return WINI_OK;
}
#endif

void CInquisitorDoc::OnCloseDocument() 
{
	COleDocument::OnCloseDocument();
}

#if 0
WINI::StatusCode CInquisitorDoc::ChangePassword(const char* password)
{
	if(NULL == password)
		return WINI_ERROR_INVALID_PARAMETER;

	return MyImpl.ChangePassword(password);
}
#endif

#if 0
bool CInquisitorDoc::isConnected()
{
	return MyImpl.isConnected();
}
#endif

#if 0
void CInquisitorDoc::OnUpdateBack(CCmdUI* pCmdUI) 
{
	if(m_HistoryCurrent == m_HistoryFirst)
		pCmdUI->Enable(FALSE);
}
#endif

#if 0
void CInquisitorDoc::OnUpdateForward(CCmdUI* pCmdUI) 
{
	if(m_HistoryCurrent == m_HistoryLast)
		pCmdUI->Enable(FALSE);
}
#endif

#if 0
void CInquisitorDoc::OnUpdateFileNew(CCmdUI* pCmdUI) 
{
	if(m_eventConnecting)
		pCmdUI->Enable(FALSE);
}
#endif

#if 0
void CInquisitorDoc::OnRefresh() 
{

	m_HistoryLast = -1;
	m_HistoryFirst = -1;
	m_HistoryCurrent = -1;

	//sprintf(buf, "%s", m_current_node->GetName());
	//WINI::INode* parent = m_current_node->GetParent();
	MyImpl.CloseTree(m_current_node);
	//MyImpl.OpenTree(parent, 1);
	//m_current_node = parent->GetChild(buf);
	MyImpl.OpenTree(m_current_node, 1, true);	

	WINI::INode *root, *localRoot;
	root = m_current_node->GetParent();
	localRoot = GetLocalRoot();

	while(root != NULL)
	{
		if(root == localRoot)
			break;

		root = root->GetParent();
	}

	if(root == localRoot)
		UpdateAllViews(NULL, 6);
	else
		UpdateAllViews(NULL, 4);
}
#endif

#if 0
void CInquisitorDoc::OnUpdateRefresh(CCmdUI* pCmdUI) 
{

	if(m_current_node)
	{

	if(0 == strcmp(m_current_node->GetName(), "home"))
		pCmdUI->Enable(FALSE);
	else if(m_current_node->GetType() == WINI_ZONE)
		pCmdUI->Enable(FALSE);

	}

}
#endif



#if 0
STDMETHODIMP 
CharlieSource::QueryInterface(REFIID riid, void** ppv)
{
	if(riid == IID_IUnknown || riid == IID_IAsyncOperation)
		*ppv = static_cast<IAsyncOperation*>(this);
	else
	{
		*ppv = 0;
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG) CharlieSource::AddRef()
{	return ++m_cRef; }

STDMETHODIMP_(ULONG) CharlieSource::Release()
{
	ULONG tmp = --m_cRef;
	if (!tmp)
		delete this;
	return tmp;
}

HRESULT STDMETHODCALLTYPE CharlieSource::SetAsyncMode(BOOL fDoOpAsync)
{
	m_AsyncModeOn = fDoOpAsync;
	return S_OK;
}
HRESULT STDMETHODCALLTYPE CharlieSource::GetAsyncMode(BOOL *pfIsOpAsync)
{
	*pfIsOpAsync = m_AsyncModeOn;
	return S_OK;
}
HRESULT STDMETHODCALLTYPE CharlieSource::StartOperation(IBindCtx *pbcReserved)
{
	return S_OK;
}
HRESULT STDMETHODCALLTYPE CharlieSource::InOperation(BOOL *pfInAsyncOp)
{
	*pfInAsyncOp = m_AsyncTransferInProgress;
	return S_OK;
}
HRESULT STDMETHODCALLTYPE CharlieSource::EndOperation(HRESULT hResult, IBindCtx *pbcReserved, DWORD dwEffects)
{
	return S_OK;
}
#endif

void CInquisitorDoc::update_rlist(void)
{
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorView *rView = frame->GetRightPane();
	rView->update_rlist();
}

void CInquisitorDoc::insert_one_coll_in_rlist(irodsWinCollection & newColl)
{
	this->colls_in_list.Add(newColl);
	
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorView *rView = frame->GetRightPane();
	rView->rlist_insert_one_coll(newColl);
	
}

void CInquisitorDoc::insert_one_dataobj_in_rlist(irodsWinDataobj & newDataObj)
{
	this->dataobjs_in_list.Add(newDataObj);

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorView *rView = frame->GetRightPane();
	rView->rlist_insert_one_dataobj(newDataObj);
}

void CInquisitorDoc::disp_err_msgbox(CString & msgHead, int errCode, CWnd *pWin /* =NULL */)
{
	char *errName = NULL;
	char *errSubName = NULL;

	errName = rodsErrorName(errCode, &errSubName);
	CString errMsg = msgHead + CString(": ") + CString(errName) + CString(" ") + CString(errSubName);

	if(pWin == NULL)
	{
		AfxMessageBox(errMsg);
	}
	else
	{
		::MessageBox(pWin->m_hWnd, errMsg, "Error Message", MB_ICONWARNING);
	}
}

void CInquisitorDoc::OnUpdateQuery(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(FALSE);
}

void CInquisitorDoc::OnUserInfo()
{
	// TODO: Add your command handler code here
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	frame->BeginWaitCursor();

	iRODSUserInfo user_info;
	CString user_name = this->conn->clientUser.userName;
	int t = irodsWinGetUserInfo(this->conn, user_name, user_info);
	if(t < 0)
	{
		frame->EndWaitCursor();
		CString msgHead = CString("irods Get User Info() error: ");
		this->disp_err_msgbox(msgHead, t);
		return;
	}
	frame->EndWaitCursor();

	CString html_info = "<html><body bgcolor=#CCCC99><h1>iRODS User Account Info</h1>\n";
	html_info += CString("<table border=2 cellpadding=2 cols=2 frame=box>\n");

	html_info += CString("<tr><td>iRODS User Name</td><td><I>") +
		CString(this->conn->clientUser.userName) + CString("</I></td></tr>\n");

	CString msg;
	//msg  = "iRODS User Name:\t\t";
	//msg += this->conn->clientUser.userName; msg += "\r\n";

	html_info += CString("<tr><td>iRODS Zone</td><td><I>") +
		CString(this->conn->clientUser.rodsZone) + CString("</I></td></tr>\n");
	//msg += "iRODS Zone:\t\t";
	//msg += this->conn->clientUser.rodsZone; msg += "\r\n";

	html_info += CString("<tr><td>iRODS Host</td><td><I>") +
		CString(this->conn->host) + CString("</I></td></tr>\n");
	//msg += "iRODS Host:\t\t"; 
	//msg += this->conn->host; msg += "\r\n";

    CString tmpstr;
	tmpstr.Format("%d", this->conn->portNum);
	html_info += CString("<tr><td>iRODS Port</td><td><I>") +
		tmpstr + CString("</I></td></tr>\n");
	//msg += "iRODS port:\t\t";
	//msg += tmpstr;

	//msg += "\r\n";

	html_info += CString("<tr><td>ID</td><td><I>") +
		CString(user_info.id) + CString("</I></td></tr>\n");
	//msg += "id:\t\t\t";
	//msg += user_info.id; msg += "\r\n";

	html_info += CString("<tr><td>Type</td><td><I>") +
		CString(user_info.type) + CString("</I></td></tr>\n");
	//msg += "Type:\t\t\t";
	//msg += user_info.type; msg += "\r\n";

	html_info += CString("<tr><td>DN</td><td><I>") +
		CString(user_info.DN) + CString("</I></td></tr>\n");
	//msg += "DN:\t\t\t";
	//msg += user_info.DN; msg += "\r\n";

	html_info += CString("<tr><td>Info</td><td><I>") +
		CString(user_info.info) + CString("</I></td></tr>\n");
	//msg += "Info:\t\t\t";
	//msg += user_info.info; msg += "\r\n";

	html_info += CString("<tr><td>Comment</td><td><I>") +
		CString(user_info.comment) + CString("</I></td></tr>\n");
	//msg += "Comment:\t\t";
	//msg += user_info.comment; msg += "\r\n";

	html_info += CString("<tr><td>Create Time</td><td><I>") +
		CString(user_info.create_time) + CString("</I></td></tr>\n");
	//msg += "Create time:\t\t";
	//msg += user_info.create_time; msg += "\r\n";

	html_info += CString("<tr><td>Modify Time</td><td><I>") +
		CString(user_info.modify_time) + CString("</I></td></tr>\n");
	//msg += "Modify time:\t\t";
	//msg += user_info.modify_time; 

	if(user_info.groups.GetSize() > 0)
	{
		for(int i=0;i<user_info.groups.GetSize();i++)
			html_info = html_info + CString("<tr><td>Member of Group</td> <td><I>") +
				CString(user_info.groups[i]) + CString("</I></td></tr>\n");
	}

	html_info += CString("</table></body></html>");

	CDialogWebDisp dlg(html_info);
	dlg.DoModal();

	//CString dlgTitle = "User Account Info";
	//CDispTextDlg tdlg(dlgTitle, msg, frame);
	//tdlg.DoModal();
}

// it is called by InquisitorView class
void CInquisitorDoc::rlist_mode_coll_delete_one_coll(CString & collFullPath)
{
	for(int i=0;i<this->colls_in_list.GetSize();i++)
	{
		if(collFullPath == this->colls_in_list[i].fullPath)
		{
			this->colls_in_list.RemoveAt(i);
			break;
		}
	}

	// delete the collection in parent coll
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CLeftView *leftView = frame->GetLeftPane();
	leftView->delete_node_from_selected_parent(collFullPath);
}

// it is called by InquisitorView class
void CInquisitorDoc::rlist_mode_coll_delete_one_dataobj(CString & name, CString & ParCollPath, int replNum)
{
	for(int i=0; i<dataobjs_in_list.GetSize(); i++)
	{
		if((name == dataobjs_in_list[i].name)&&(ParCollPath == dataobjs_in_list[i].parentCollectionFullPath)
			&&(replNum == dataobjs_in_list[i].replNum))
		{
			dataobjs_in_list.RemoveAt(i);
			break;
		}
	}

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CLeftView *leftView = frame->GetLeftPane();
	leftView->delete_dataobj_from_selected_parent(name, ParCollPath, replNum);
}

void CInquisitorDoc::rlist_mode_coll_change_one_collname(CString & collOldFullPath, CString & newName)
{
	for(int i=0;i<this->colls_in_list.GetSize();i++)
	{
		if(collOldFullPath == this->colls_in_list[i].fullPath)
		{
			CString parPath, newFullPath;
			irodsWinUnixPathGetParent(collOldFullPath, parPath);
			newFullPath = collOldFullPath + "/" + newName;
			this->colls_in_list[i].fullPath = newFullPath;
			this->colls_in_list[i].name = newName;
			break;
		}
	}

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CLeftView *leftView = frame->GetLeftPane();
	leftView->change_one_collname_from_selected_parent(collOldFullPath, newName);
}

bool CInquisitorDoc::is_only_data_copy(CString & dataName)
{
	int cnt = 0;
	for(int k=0;k<dataobjs_in_list.GetSize();k++)
	{
		if(dataName.Compare(dataobjs_in_list[k].name) == 0)
		{
			cnt++;
		}
	}

	if(cnt > 1)
		return false;

	return true;
}

void CInquisitorDoc::rlist_mode_coll_change_one_dataname(CString & parFullPath, CString & oldName, CString newName)
{
	for(int i=0; i<dataobjs_in_list.GetSize(); i++)
	{
		if(dataobjs_in_list[i].name == oldName)
		{
			dataobjs_in_list[i].name = newName;
		}
	}

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CLeftView *leftView = frame->GetLeftPane();
	leftView->change_one_dataname_from_selected_parent(oldName, newName);
}

void CInquisitorDoc::refresh_disp()
{
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CLeftView *leftView = frame->GetLeftPane();
	leftView->OnViewRefresh();
}

bool CInquisitorDoc::is_irods_trash_area(CString & collFullPath)
{
	int i = collFullPath.Find("/", 1);
	if(i < 0)  // something is wrong.
		return false;

	CString tmpstr = collFullPath.Right(collFullPath.GetLength() - i - 1);
	i = tmpstr.Find("/", 1);
	if(i < 0)
		return false;
	tmpstr = tmpstr.Left(i);
	if(tmpstr == "trash")
		return true;

	return false;
}

int CInquisitorDoc::upload_a_localfolder_into_coll(CString & parCollFullPath, irodsWinCollection & newColl)
{
	// check if this is a place that does not allow such operation.
	if(is_irods_trash_area(parCollFullPath))
	{
		//AfxMessageBox("Are you sure you want to upload data into trash area?", MB_YESNO);
		AfxMessageBox("The upload operation is not allowed in trash area!");
		return -1;
	}

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CString irodsResc = frame->get_default_resc();
	if(irodsResc.GetLength() == 0)
	{
		AfxMessageBox("Please select a deafult resource for this operation!");
		return -1;
	}

	// select a folder
	CString selectedLocalDir = "";
	this->SelectLocalFolder(selectedLocalDir);
	if(selectedLocalDir.GetLength() == 0)
		return -1;

	frame->BeginWaitCursor();
	CString msg = "uploading a folder ...";
	frame->statusbar_msg(msg);

	frame->StartProgressBar();
	gGuiProgressCB = &irodsWinMainFrameProgressCB;
	
	int t = irodsWinUploadOneFolder(this->conn, irodsResc, 
				parCollFullPath, selectedLocalDir, this->upload_always_override, newColl);
	if(t < 0)
	{
		CString msgHead = CString("Uploading folder error ");
		this->disp_err_msgbox(msgHead, t);
		msg = "";
		frame->statusbar_msg(msg);
		frame->EndProgressBar();
		gGuiProgressCB = NULL;
		return -1;
	}

	msg = "Upload Local Folder finished.";
	frame->statusbar_msg(msg);
	frame->EndProgressBar();
	gGuiProgressCB = NULL;

	return 0;
}

int CInquisitorDoc::upload_localfiles_into_coll(CString & parCollFullPath, CArray<irodsWinDataobj, irodsWinDataobj> & newDataobjs)
{
	if(is_irods_trash_area(parCollFullPath))
	{
		AfxMessageBox("The upload operation is not allowed in trash area!");
		return -1;
	}

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	
	CString irodsResc = frame->get_default_resc();
	if(irodsResc.GetLength() == 0)
	{
		AfxMessageBox("Please select a deafult resource for this operation!");
		return -1;
	}

	// let user select files to be uploaded.
	CFileDialog FileDlg(TRUE, NULL, NULL,  OFN_NOVALIDATE | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST, NULL, NULL);

	FileDlg.m_ofn.lpstrTitle = "Upload Files";

	CString winFilenameWithPath;
	
	int buf_size = 8192;
	char *sel_files_buf = new char[buf_size];
	memset(sel_files_buf,'\0',buf_size*sizeof(char));
	FileDlg.m_ofn.lpstrFile = sel_files_buf;
	FileDlg.m_ofn.nMaxFile = buf_size;

	if(FileDlg.DoModal() != IDOK)
		return -1;

	CString msg = "uploading file(s) ...";
	frame->statusbar_msg(msg);
	frame->BeginWaitCursor();

	// get selected count
	int total_num_sel_files = 0;
	POSITION POS = FileDlg.GetStartPosition();
	while(POS != NULL)
	{
		CStringW tttstr = FileDlg.GetNextPathName(POS);
		//winFilenameWithPath = FileDlg.GetNextPathName(POS);
		winFilenameWithPath = tttstr;
		if(winFilenameWithPath.Compare(".")==0)
			continue;

		total_num_sel_files++;
	}

	// attach the global progress CB
	gGuiProgressCB = &irodsWinMainFrameProgressCB;

	float ct;
	int curcnt = 0;
	frame->StartProgressBar();
	POS = FileDlg.GetStartPosition();
	while(POS != NULL)
	{
		winFilenameWithPath = FileDlg.GetNextPathName(POS);
		if(winFilenameWithPath.Compare(".")==0)
			continue;

		msg.Format("Uploading %s", winFilenameWithPath);
		irodsWinMainFrameProgressMsg((char *)LPCTSTR(msg), 0.2);

		irodsWinDataobj newDataObj;
		int t = irodsWinUploadOneFile(this->conn, irodsResc, 
					parCollFullPath, winFilenameWithPath, this->upload_always_override,
					newDataObj);
		if(t < 0)
		{
			CString msgHead = CString("Uploading File(s) error for ") + parCollFullPath;
			this->disp_err_msgbox(msgHead, t);
			frame->statusbar_clear();
			frame->EndWaitCursor();
			frame->EndProgressBar();
			delete sel_files_buf;
			return -1;
		}

		// insert the new dataset into internal data of selected node
		newDataobjs.Add(newDataObj);

		frame->ProgressBarMove2Full();

		curcnt++;
	}

	winFilenameWithPath.ReleaseBuffer();

	msg = "Upload Local File(s) finished!";
	frame->statusbar_msg(msg);
	frame->EndWaitCursor();
	frame->EndProgressBar();
	delete sel_files_buf;

	// put the global progress CB back to NULL.
	gGuiProgressCB = NULL;

	return 0;
}

void CInquisitorDoc::rlist_dbclick_coll_action(CString & collFullPath)
{
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CLeftView *leftView = frame->GetLeftPane();
	leftView->rlist_dbclick_coll_action(collFullPath);
}

#if 0
void CInquisitorDoc::OnPreferenceAlwaysoverridewhenuploadingfile()
{
	// TODO: Add your command handler code here
	CMenu *menuItem = AfxGetMainWnd()->GetMenu();

	UINT theState = menuItem->GetMenuState(ID_PREFERENCE_ALWAYSOVERRIDEWHENUPLOADINGFILE, MF_BYCOMMAND);

	if (theState & MF_CHECKED)
	{
		upload_always_override = false;
		menuItem->CheckMenuItem(ID_PREFERENCE_ALWAYSOVERRIDEWHENUPLOADINGFILE, MF_UNCHECKED | MF_BYCOMMAND);
	}
	else
	{
		upload_always_override = true;
		menuItem->CheckMenuItem(ID_PREFERENCE_ALWAYSOVERRIDEWHENUPLOADINGFILE, MF_CHECKED | MF_BYCOMMAND);
	}
}
#endif

int CInquisitorDoc::check_coll_with_buffered_objs(CString & mycollPath)   
{
	if(colls_in_copy_buffer.GetSize() <= 0)
		return NO_RELATION_WITH_BUFFERED_DATA;

	for(int i = 0;i < colls_in_copy_buffer.GetSize(); i++)
	{
		if(colls_in_copy_buffer[i].fullPath == mycollPath)
			return HAS_A_SAME_COLL_IN_BUFFERED_DATA;

		if(irodsWinIsDescendant(mycollPath, colls_in_copy_buffer[i].fullPath))
			return IS_A_DESCENDANT_OF_A_COLL_IN_BUFFERED_DATA;
	}

	return NO_RELATION_WITH_BUFFERED_DATA;
}

void CInquisitorDoc::move_buffered_irods_objs(CString & destinationColl)
{
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CString msg;
	
	if((colls_in_copy_buffer.GetSize()<=0)&&(dataobjs_in_copy_buffer.GetSize()<=0))
	{
		msg = "No buffered data for the operation.";
		frame->statusbar_msg(msg);
		return;
	}

	int t = check_coll_with_buffered_objs(destinationColl);
	if( t == HAS_A_SAME_COLL_IN_BUFFERED_DATA)
	{
		msg = "The destinated collection is among buffered data.";
		frame->statusbar_msg(msg);
		return;
	}
	else if(t == IS_A_DESCENDANT_OF_A_COLL_IN_BUFFERED_DATA)
	{
		msg = "The destinated collection is descendant of buffered data.";
		frame->statusbar_msg(msg);
		return;
	}

	msg = "Moving buffered irods data/collections into selected collection...";
	frame->statusbar_msg(msg);
	frame->BeginWaitCursor();

	int i;
	CStringArray irods_objs;
	if(colls_in_copy_buffer.GetSize() > 0)
	{
		for(i=0;i<colls_in_copy_buffer.GetSize();i++)
		{
			irods_objs.Add(this->colls_in_copy_buffer[i].fullPath);
			t = irodsWinMoveObjects(this->conn, destinationColl, irods_objs);
			if(t < 0)
			{
				frame->statusbar_clear();
				CString msgHead = CString("Move irods data/collection() error: ");
				this->disp_err_msgbox(msgHead, t);
				frame->EndWaitCursor();
				return;
			}
			irods_objs.RemoveAll();
		}
	}

	irods_objs.RemoveAll();
	if(dataobjs_in_copy_buffer.GetSize() > 0)
	{
		for(i=0;i<dataobjs_in_copy_buffer.GetSize();i++)
		{
			CString fullname = dataobjs_in_copy_buffer[i].parentCollectionFullPath + "/" + dataobjs_in_copy_buffer[i].name;
			irods_objs.Add(fullname);
			t = irodsWinMoveObjects(this->conn, destinationColl, irods_objs);
			if(t < 0)
			{
				frame->statusbar_clear();
				CString msgHead = CString("Move irods data/collection() error: ");
				this->disp_err_msgbox(msgHead, t);
				frame->EndWaitCursor();
				return;
			}
			irods_objs.RemoveAll();
		}
	}

	msg = "Move Data finished.";
	this->statusbar_msg(msg);
	frame->EndWaitCursor();
}

void CInquisitorDoc::paste_buffered_irods_objs(CString & pasteToCollFullPath)
{
	int i,t;

	if((dataobjs_in_copy_buffer.GetSize() ==0)&&(colls_in_copy_buffer.GetSize()==0))
		return;

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CString irodsResc = frame->get_default_resc();
	if(irodsResc.GetLength() == 0)
	{
		AfxMessageBox("Please select a deafult resource for this operation!");
		return;
	}

	CString msg = "copying buffered irods data/collections into selected collection...";
	frame->statusbar_msg(msg);
	frame->BeginWaitCursor();

	CStringArray irods_objs; 
	// check if the collec is the parent of the buffer paste to
	if(colls_in_copy_buffer.GetSize() > 0)
	{
	
		for(i=0;i<colls_in_copy_buffer.GetSize();i++)
		{
			irods_objs.RemoveAll();
			if((this->colls_in_copy_buffer[i].fullPath==pasteToCollFullPath)||
				irodsWinIsDescendant(pasteToCollFullPath, colls_in_copy_buffer[i].fullPath))
			{
				frame->statusbar_clear();
				msg = CString("The collection, ") + colls_in_copy_buffer[i].fullPath + CString(", cannot be pasted into itself or its descendant!");
				AfxMessageBox(msg);
				frame->EndWaitCursor();
				return;
			}
			irods_objs.Add(this->colls_in_copy_buffer[i].fullPath);
			t = irodsWinPasteObjects(this->conn, irodsResc, pasteToCollFullPath, irods_objs, this->paste_always_override);
			if(t < 0)
			{
				msg = "";
				frame->statusbar_msg(msg);
				CString msgHead = CString("Paste irods data/collection() error: ");
				this->disp_err_msgbox(msgHead, t);
				frame->EndWaitCursor();
				return;
			}
		}	
	}

	irods_objs.RemoveAll();
	if(dataobjs_in_copy_buffer.GetSize() > 0)
	{
		for(i=0;i<dataobjs_in_copy_buffer.GetSize();i++)
		{
			irods_objs.RemoveAll();
			CString fullname = dataobjs_in_copy_buffer[i].parentCollectionFullPath + "/" + dataobjs_in_copy_buffer[i].name;
			irods_objs.Add(fullname);
			t = irodsWinPasteObjects(this->conn, irodsResc, pasteToCollFullPath, irods_objs, this->paste_always_override);
			if(t < 0)
			{
				msg = "";
				frame->statusbar_msg(msg);
				CString msgHead = CString("Paste irods data/collection() error: ");
				this->disp_err_msgbox(msgHead, t);
				frame->EndWaitCursor();
				return;
			}
		}
	}

	msg = "Paste Buffered Data/Collection finished.";
	frame->statusbar_msg(msg);
	frame->EndWaitCursor();

	if(this->rlist_disp_mode == CInquisitorDoc::RLIST_MODE_COLL)
	{
		CLeftView *leftView = frame->GetLeftPane();
		leftView->OnViewRefresh();
	}
}

void CInquisitorDoc::OnIrodsPreferences()
{
	// TODO: Add your command handler code here
	CPreferenceDlg PrefDlg(this);
	PrefDlg.DoModal();
}

void CInquisitorDoc::OnTreeUp()
{
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CLeftView *leftView = frame->GetLeftPane();
	leftView->tree_goto_parent_node();
}

void CInquisitorDoc::disp_file_in_local_machine(CString & parPath, CString & fname, CWnd *pWin /* =NULL */)
{
	// check file type;
	int num_warning_types = 3;
	char *wanring_types[3] = {"exe", "bat", "pl"};
	int t = fname.ReverseFind('.') ;
	if(t >= 0)
	{
		t  = fname.GetLength() - t -1;
		CString fextension = fname.Right(t);
		for(int i=0;i<num_warning_types;i++)
		{
			if(fextension.CompareNoCase(wanring_types[i]) == 0)
			{
				if(AfxMessageBox("Wanring: The selected file is an executable. Are you sure you want to run the executable in your machine?", MB_YESNO) != IDYES)
					return;

			}
		}
	}

	BeginWaitCursor();
	// If the file exists in tmp dir, remove it.
	CString file_in_tmpdir = local_temp_dir + CString("\\") + fname;
	file_in_tmpdir.Replace("/", "\\");
	struct __stat64 mystat;
	if(_stat64((char *)LPCTSTR(file_in_tmpdir), &mystat) == 0)
	{
		if(remove(file_in_tmpdir) == -1)
		{
			AfxMessageBox("Failed to delete the file with same filename in temp directory.");
			EndWaitCursor();
			return;
		}
		// else sleep a little while
		::Sleep(500);
	}

	// download the file and launch into a default place.
	CString irods_fname_wpath = parPath + CString("/") + fname;
	t = irodsWinDownloadOneObject(conn, irods_fname_wpath, local_temp_dir, 0, 1);
	if(t < 0)
	{
		CString msgHead = CString("Downlod Operation error ");
		disp_err_msgbox(msgHead, t);
		EndWaitCursor();
		return;
	}

	// change the permission of the newly created temp file
	_chmod(file_in_tmpdir, _S_IREAD | _S_IWRITE);

	// launch the item in Windows.
	char local_sname[1024];
	char tmpname[1024];
	sprintf(tmpname, "%s\\%s", (char *)LPCTSTR(local_temp_dir), fname);
	t = GetShortPathName(tmpname, local_sname, 1024);
	if(t <= 0)
	{
		CString msgHead = CString("GetShortPathName() Operation error ");
		disp_err_msgbox(msgHead, t);
		EndWaitCursor();
		return;
	}
	system(local_sname);
}

void CInquisitorDoc::OnUseraccountChangepassword()
{
	// TODO: Add your command handler code here
	CChangePasswdDlg dlg;
	dlg.DoModal();
}

void CInquisitorDoc::OnRuleSubmitanirodsrule()
{
	// TODO: Add your command handler code here
	CSubmitIRodsRuleDlg dlg;
	dlg.DoModal();
}

void CInquisitorDoc::OnRuleCheckstatusofpendingrule()
{
	// TODO: Add your command handler code here
	CString dlg_title = "Pending Rule Status Dialog";
	CRuleStatusDlg dlg(dlg_title);
	dlg.DoModal();
}

void CInquisitorDoc::OnRuleDeletependingrule()
{
	// TODO: Add your command handler code here
	CString dlg_title = "Delete Pending Rule Dialog";
	CRuleStatusDlg dlg(dlg_title);
	dlg.DoModal();
}

/*
void CInquisitorDoc::OnSearchMetadatasearch()
{
	// TODO: Add your command handler code here
	CMetaSearchDlg dlg(cur_selelected_coll_in_tree->fullPath);
	dlg.DoModal();
}
*/

void CInquisitorDoc::OnEditSearch()
{
	// TODO: Add your command handler code here
	//CMetaSearchDlg dlg(cur_selelected_coll_in_tree->fullPath);
	//dlg.DoModal();
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	frame->CreateMetaSeachDlg(cur_selelected_coll_in_tree->fullPath);
}

void CInquisitorDoc::statusbar_msg(CString & msg)
{
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	frame->statusbar_msg(msg);
}

bool CInquisitorDoc::dragdrop_upload_local_data(COleDataObject* pDataObject, CString & parentColl)
{
	HGLOBAL hg;
	HDROP   hdrop;

	hg = pDataObject->GetGlobalData (CF_HDROP );

	if ( NULL == hg )
        return false;
	// http://www.codeproject.com/KB/shell/explorerdragdrop.aspx

	hdrop = (HDROP) GlobalLock ( hg );
	if ( NULL == hdrop )
    {
		GlobalUnlock ( hg );
		return false;
    }

	// query files and upload ..
	int NumFiles = DragQueryFile ( hdrop, -1, NULL, 0 );
	if(NumFiles <= 0)
	{
		GlobalUnlock ( hg );
		return false;
	}

	// confirmation
	CString msg;
	msg.Format("iRODS will upload selected local files and folders into '%s'. Are you Sure?", parentColl);
	if(AfxMessageBox(msg, MB_YESNO) != IDYES)
	{
		GlobalUnlock ( hg );
		return false;
	}

	int t;
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CString irodsResc = frame->get_default_resc();

	frame->BeginWaitCursor();
	msg = "uploading local file(s) or folder(s) from drag-drop ...";
	frame->statusbar_msg(msg);
	frame->StartProgressBar();
	gGuiProgressCB = &irodsWinMainFrameProgressCB;

	TCHAR szNextFile [MAX_PATH];
    for (int i = 0; i < NumFiles; i++ )
    {
		// Get the next filename from the HDROP info.
		if (DragQueryFile(hdrop, i, szNextFile, MAX_PATH ) > 0 )
		{
			struct irodsntstat statbuf;
			if(iRODSNt_stat(szNextFile, &statbuf) == -1)
			{
				msg.Format("Failed to stat file: %s.", szNextFile);
				frame->EndProgressBar();
				gGuiProgressCB = NULL;
				GlobalUnlock(hg);
				AfxMessageBox(msg);
				return false;
			}

			CString local_file_or_dir_fullname = szNextFile;
			t = 0;
			if(statbuf.st_mode & _S_IFREG) // a file
			{
				irodsWinDataobj newDataObj;
				t = irodsWinUploadOneFile(this->conn, irodsResc, parentColl,
						local_file_or_dir_fullname, 1, newDataObj);
				if(t < 0)
				{
					msg.Format("Upload file '%s' failed. Uploading operation quit.", local_file_or_dir_fullname);
				}
			}
			else if(statbuf.st_mode & _S_IFDIR)  // a dir
			{
				irodsWinCollection newColl;
				t = irodsWinUploadOneFolder(this->conn, irodsResc, parentColl, 
						local_file_or_dir_fullname, 1, newColl);
				if(t < 0)
				{
					msg.Format("Upload folder '%s' failed. Uploading operation quit.", local_file_or_dir_fullname);
				}
			}
			if(t < 0)
			{
				frame->statusbar_clear();

				frame->EndProgressBar();
				gGuiProgressCB = NULL;

				GlobalUnlock(hg);
				this->disp_err_msgbox(msg, t);
				return false;
			}
		}
    }

	GlobalUnlock(hg);

	msg = "Upload file(s) or folder(s) finished.";
	frame->statusbar_msg(msg);

	frame->EndProgressBar();
	gGuiProgressCB = NULL;

	return true;
}
