// InquisitorDoc.cpp : implementation of the CInquisitorDoc class
//
#include "stdafx.h"
#include "Inquisitor.h"

#include "InquisitorDoc.h"
#include "Login.h"
#include "MainFrm.h"
#include "Query.h"
#include "WorkerThread.h"
#include "winiCollectionOperator.h"
#include "AccessCtrl.h"
#include "winiStatusCode.h"

//#include "DS2Dialog.h"
//#include "DSThread.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CInquisitorApp theApp;
extern CRITICAL_SECTION gLock;
extern int gCount;
extern CMutex gCS;
extern std::vector<WINI::INode*> gNodeDragList;
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
	ON_COMMAND(ID_QUERY, OnQuery)
	ON_UPDATE_COMMAND_UI(ID_BACK, OnUpdateBack)
	ON_UPDATE_COMMAND_UI(ID_FORWARD, OnUpdateForward)
	ON_UPDATE_COMMAND_UI(ID_FILE_NEW, OnUpdateFileNew)
	ON_COMMAND(ID_REFRESH, OnRefresh)
	ON_UPDATE_COMMAND_UI(ID_REFRESH, OnUpdateRefresh)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInquisitorDoc construction/destruction

CInquisitorDoc::CInquisitorDoc()
{
	m_current_node = NULL;
	m_defaultResource = NULL;
	m_worker_thread = NULL;
	m_HistoryLast = -1;
	m_HistoryFirst = -1;
	m_HistoryCurrent = -1;

	for(int i = 0; i < 10; i++)
		m_History[i] = NULL;
	
	m_synchFlag = 0;
	m_pCThread = NULL;
	m_hCThread = NULL;
	mystupidcount = 0;
}

void CInquisitorDoc::SetSynchronizationFlags(bool purge, bool primary_only)
{
	m_synchFlag = 0;

//	if(purge)
//		m_synchFlag |= PURGE_FLAG;
//	if(primary_only)
//		m_synchFlag |= PRIMARY_FLAG;
}

WINI::StatusCode CInquisitorDoc::SetAccess(WINI::INode* target, WINI::IUserNode* owner, const char* permission, bool recursive)
{
	int type = target->GetType();
	WINI::IOperator* op = MyImpl.GetOperator(type);
	WINI::StatusCode status;

	switch(type)
	{
		case WINI_DATASET:
			status = ((WINI::IDatasetOperator*)op)->SetAccess((WINI::IDatasetNode*)target, owner, permission);
		break;
		case WINI_COLLECTION:
			status = ((WINI::ICollectionOperator*)op)->SetAccess((WINI::ICollectionNode*)target, owner, permission, recursive);
		break;
		default:
			status = WINI_ERROR_INVALID_PARAMETER;
	}

	return status;
}

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

CInquisitorDoc::~CInquisitorDoc()
{
	if(m_worker_thread)
	{
		if(m_worker_thread->m_session)
		{
			m_worker_thread->m_session->Disconnect();
			m_worker_thread->m_session = NULL;
		}
	}
}

void CInquisitorDoc::OnBack()
{
	if(m_HistoryCurrent == m_HistoryFirst)
		return;

	if(m_HistoryCurrent != m_HistoryFirst)	//can I go back farther in my history?
	{
		if(-1 == --m_HistoryCurrent)
			m_HistoryCurrent = 9;
		m_current_node = m_History[m_HistoryCurrent];
	}
}

void CInquisitorDoc::OnForward()
{
	if(m_HistoryCurrent == m_HistoryLast)
		return;

	if(m_HistoryCurrent != m_HistoryLast)
	{
		m_HistoryCurrent = ++m_HistoryCurrent % 10;
		m_current_node = m_History[m_HistoryCurrent];
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
	m_current_node = NULL;
	UpdateAllViews(NULL,3);

	if(m_worker_thread)
	{
		//if initial connect was not successful, worker thread was never created
		//should have a waitonquit here...
		HANDLE progthread;
		::DuplicateHandle (GetCurrentProcess(), m_worker_thread->m_hThread, GetCurrentProcess (), &progthread, 0, FALSE, DUPLICATE_SAME_ACCESS);
		VERIFY(ResumeWorkThread());
		m_worker_thread->PostThreadMessage(WM_QUIT, NULL, NULL);
		::WaitForSingleObject(progthread, INFINITE);
		VERIFY(::CloseHandle(progthread));
		m_worker_thread = NULL;
	}
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
	WINI::SessionImpl* connection;
	HWND handle;
};

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
	int status = m_Login.DoModal();

	switch(status)
	{
	case IDOK:
		m_eventConnecting = true;
		m_current_node = NULL;
		m_HistoryLast = -1;
		m_HistoryFirst = -1;
		m_HistoryCurrent = -1;
		break;
	case IDCANCEL:
		ASSERT(NULL == m_worker_thread);
		AfxGetMainWnd()->PostMessage(WM_QUIT);	//initial doesn't need this, but subsequent calls, where mainframe has already been drawn, need this
		return FALSE;
		break;
	default:
		AfxMessageBox("Error!", MB_OK);
		return FALSE;
	}

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();

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

	char sztitle[256];

	sprintf(sztitle, "Connecting to %s and getting data...", m_Login.GetHost());
	this->SetTitle("Not Connected");
	frame->SetStatusMessage(sztitle);
	m_pCThread = AfxBeginThread(ConnectToWINI, c, THREAD_PRIORITY_NORMAL, 0,CREATE_SUSPENDED);
	::DuplicateHandle(GetCurrentProcess(), m_pCThread->m_hThread, GetCurrentProcess(), &m_hCThread, 0, FALSE, DUPLICATE_SAME_ACCESS);
	m_pCThread->ResumeThread();
	::Sleep(0);
	return TRUE;
}

void CInquisitorDoc::OnFailedConnection()
{
	m_eventConnecting = false;
	AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_FILE_NEW, NULL);
}

BOOL CInquisitorDoc::OnCompletedConnection()
{
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	static char szbuf[256];
	char* l_login = (char*)m_Login.GetMyName();
	char* l_host = (char*)MyImpl.GetUserHost();
	int l_port = MyImpl.GetUserPortValue();
	sprintf(szbuf, "%s | %s (%d)", l_login,l_host ,l_port );
	this->SetTitle(szbuf);

	sprintf(szbuf, "Data for %s received.  Initializing WINI...", m_Login.GetMyName());
	frame->SetStatusMessage(szbuf);
	::WaitForSingleObject(m_hCThread, INFINITE);
	VERIFY(::CloseHandle(m_hCThread));
	m_hCThread = NULL;
	m_pCThread = NULL;

	WINI::ISetNode* resources = MyImpl.GetResource(NULL);


	CWinApp* pApp = AfxGetApp();

	WINI::IResourceNode* resource_node = NULL;
	

	char rbuf[1024];
	if(m_Login.GetDefaultResource())
		sprintf(rbuf, "%s", m_Login.GetDefaultResource());

	int rlen = strlen(rbuf);

	if(rlen)
		for(--rlen; rlen != 0; --rlen)
			if(rbuf[rlen] == '/')
				break;
	
	if(resources)
		resource_node = (WINI::IResourceNode*)resources->GetChild(&rbuf[rlen]);

	if(NULL == resource_node)
		if(resources)
			resource_node = (WINI::IResourceNode*)resources->GetChild(0);

	m_defaultResource = resource_node;

	m_worker_thread = (CWorkerThread*)AfxBeginThread(RUNTIME_CLASS(CWorkerThread), THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED, NULL);

	//thread initialization
	m_worker_thread->SetThread2((AfxGetMainWnd())->m_hWnd, &MyImpl);
	m_eventConnecting = false;
	return TRUE;
}

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

WINI::INode* CInquisitorDoc::GetDomains()
{
	return NULL;
}

WINI::INode* CInquisitorDoc::GetRoot()
{
	return MyImpl.GetRoot(m_zone);
}

WINI::INode* CInquisitorDoc::GetLocalRoot()
{
	return MyImpl.GetLocalRoot();
}


WINI::INode* CInquisitorDoc::GetHome()
{
	return MyImpl.GetHome(m_zone);
}

WINI::INode* CInquisitorDoc::GetMCAT()
{
	return MyImpl.GetCatalog();
}

WINI::INode* CInquisitorDoc::GetHomeZone()
{
	return (WINI::INode*)MyImpl.GetHomeZone();
}



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
#else
WINI::StatusCode CInquisitorDoc::OpenTree(WINI::INode* node, int levels, bool clear, unsigned int mask)
{
	return MyImpl.OpenTree(node, levels, clear, mask);
}
#endif


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

WINI::IStringNode* CInquisitorDoc::GetAccessConstraint()
{
	return MyImpl.GetAccessConstraints();
}

WINI::INode* CInquisitorDoc::GetCurrentNode()
{
	return m_current_node;
}

void CInquisitorDoc::OnQuery() 
{
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

void CInquisitorDoc::SetComment(WINI::INode* node, char* comment)
{
	if(NULL == node)
		return;

	if(NULL == comment)
		return;

	m_worker_thread->PostThreadMessage(msgSetComment, (WPARAM)strdup(comment), (LPARAM)node);
	m_worker_thread->ResumeThread();
}

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

	m_worker_thread->PostThreadMessage(msgDelete, 0, (LPARAM)node);
	m_worker_thread->ResumeThread();
}

void CInquisitorDoc::Rename(WINI::INode* node, char* name)
{
	if(m_current_node == node)
	{
		m_current_node = node->GetParent();
		ASSERT(m_current_node != NULL);
	}

	char* new_name = strdup(name);

	m_worker_thread->PostThreadMessage(msgRename, (WPARAM)new_name, (LPARAM)node);
	m_worker_thread->ResumeThread();
}

bool CInquisitorDoc::SuspendWorkThread()
{
	ASSERT(m_worker_thread);

	if(m_worker_thread->m_pPThread)
		if(-1 == m_worker_thread->m_pPThread->SuspendThread())
			if(ERROR_SUCCESS != GetLastError())
				return false;

	if(-1 == m_worker_thread->SuspendThread())
		if(ERROR_SUCCESS != GetLastError())
			return false;

	return true;
}

//The error_success test is crucial for Win9X systems. Unlike NT systems,
//ResumeThread seems to report an error if you attempt to resume a thread
//that is already running.
bool CInquisitorDoc::ResumeWorkThread()
{
	ASSERT(m_worker_thread);

	int i;
	do
	{
		i = m_worker_thread->ResumeThread();
		if(-1 == i)
			if(ERROR_SUCCESS != GetLastError())
				return false;
	} while(i > 1);

	if(m_worker_thread->m_pPThread)
	{
		do
		{
			i = m_worker_thread->m_pPThread->ResumeThread();
			if(-1 == i)
				if(ERROR_SUCCESS != GetLastError())
					return false;
		} while(i > 1);
	}

	return true;
}

void CInquisitorDoc::Download(WINI::INode* node, const char* local_path, bool Execute)
{
	dl* mydl = new dl;

	mydl->node = node;
	mydl->local_path = strdup(local_path);

	WPARAM wParam = 0;

	if(Execute)
		wParam = 1;

	m_worker_thread->PostThreadMessage(msgDownload, wParam, (LPARAM)mydl);
	m_worker_thread->ResumeThread();
}

void CInquisitorDoc::Download(WINI::INode* node, bool Execute)
{
	dl* mydl = new dl;

	mydl->node = node;
	mydl->local_path = strdup(((CInquisitorApp*)AfxGetApp())->m_szTempPath);

	WPARAM wParam = 0;

	if(Execute)
		wParam = 1;
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

	m_worker_thread->PostThreadMessage(msgDownload, wParam, (LPARAM)mydl);
	m_worker_thread->ResumeThread();

}

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

	m_worker_thread->PostThreadMessage(msgCopy, NULL, (LPARAM)params);
	m_worker_thread->ResumeThread();
}

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

	m_worker_thread->PostThreadMessage(msgCopy, NULL, (LPARAM)params);
	m_worker_thread->ResumeThread();
}

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

void CInquisitorDoc::Upload(WINI::INode* node, const char* local_path)
{
	dl* mydl = new dl;

	mydl->node = node;
	mydl->local_path = strdup(local_path);

	mydl->binding = m_defaultResource;

	m_worker_thread->PostThreadMessage(msgUpload, 0, (LPARAM)mydl);
	m_worker_thread->ResumeThread();
}


WINI::ISetNode* CInquisitorDoc::GetResources(WINI::IZoneNode* node)
{
	return MyImpl.GetResource(node);
}


void CInquisitorDoc::Replicate(WINI::INode* node)
{
	new_replicant* blah = new new_replicant;
	blah->node = node;
	blah->binding = m_defaultResource;

	m_worker_thread->PostThreadMessage(msgReplicate, 0, (LPARAM)blah);
	m_worker_thread->ResumeThread();
}

void CInquisitorDoc::CreateNode(WINI::INode* parent, const char* name)
{
	new_node* blah = new new_node;

	blah->parent = parent;

	blah->name = strdup(name);

	m_worker_thread->PostThreadMessage(msgNewNode, 0, (LPARAM)blah);
	m_worker_thread->ResumeThread();
}

WINI::StatusCode CInquisitorDoc::SetDefaultResource(WINI::INode* node)
{
	if(NULL == node)
		return WINI_ERROR_INVALID_PARAMETER;

	if(WINI_RESOURCE != node->GetType())
		return WINI_ERROR_INVALID_PARAMETER;

	m_defaultResource = node;

	m_Login.SetDefaultResource(node->GetPath());

	return WINI_OK;
}

void CInquisitorDoc::SetAccess(WINI::INode* node)
{
	CAccessCtrl Access(this, node, AfxGetMainWnd());
	Access.DoModal();
	this->UpdateAllViews(NULL,2);
}

void CInquisitorDoc::Synchronize(WINI::INode* node)
{
	m_worker_thread->PostThreadMessage(msgSynchronize, m_synchFlag, (LPARAM)node);
	m_worker_thread->ResumeThread();
}

extern bool g_bIsDragging;
extern UINT gInqClipboardFormat;
extern bool gbExiting;

CharlieSource::~CharlieSource()
{


}

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

void CInquisitorDoc::OnCloseDocument() 
{
	if(m_worker_thread)
		SuspendWorkThread();

	if(m_hCThread)
	{
		ASSERT(NULL != m_pCThread);
		m_pCThread->SuspendThread();
	}

	COleDocument::OnCloseDocument();
}


WINI::StatusCode CInquisitorDoc::ChangePassword(const char* password)
{
	if(NULL == password)
		return WINI_ERROR_INVALID_PARAMETER;

	return MyImpl.ChangePassword(password);
}

bool CInquisitorDoc::isConnected()
{
	return MyImpl.isConnected();
}

void CInquisitorDoc::OnUpdateBack(CCmdUI* pCmdUI) 
{
	if(m_HistoryCurrent == m_HistoryFirst)
		pCmdUI->Enable(FALSE);
}

void CInquisitorDoc::OnUpdateForward(CCmdUI* pCmdUI) 
{
	if(m_HistoryCurrent == m_HistoryLast)
		pCmdUI->Enable(FALSE);
}

void CInquisitorDoc::OnUpdateFileNew(CCmdUI* pCmdUI) 
{
	if(m_eventConnecting)
		pCmdUI->Enable(FALSE);
}

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