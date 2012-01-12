// WorkerThread.cpp : implementation file
//

#include "stdafx.h"
#include "Inquisitor.h"
#include "WorkerThread.h"
#include "winiSession.h"
#include "winiObjects.h"
#include "LeftView.h"
#include <Shlwapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//extern CMutex gMutex;
//extern CCriticalSection gCS;
//extern CMutex gCS;
extern int gCount;
extern CEvent killWorker;

//function takes as a parameter a pointer to a sessionimpl pointer.
//when function returns the pointer points to the newly-created session
UINT CloneWINIConnection(LPVOID pParam)
{
	WINI::SessionImpl** connection = (WINI::SessionImpl**)pParam;

	WINI::SessionImpl* blah = new WINI::SessionImpl;

	WINI::StatusCode status = (*connection)->Clone(blah);

	if(status.isOk())
	{
		*connection = blah;
		return 0;
	}

	*connection = NULL;
	return 1;
}

//
class CMainWindow : public CFrameWnd
{
public:
	CMainWindow();
protected:
	afx_msg void OnLButtonDown(UINT, CPoint);
	DECLARE_MESSAGE_MAP()
};
BEGIN_MESSAGE_MAP(CMainWindow, CFrameWnd)
	ON_WM_LBUTTONDOWN()
	END_MESSAGE_MAP()

	CMainWindow::CMainWindow()
	{
		Create(NULL, _T("TEST"));
	}

	void CMainWindow::OnLButtonDown(UINT nFlags, CPoint point)
	{
		//PostMessage(WM_CLOSE, 0,0);
	}

/////////////////////////////////////////////////////////////////////////////
// CWorkerThread


IMPLEMENT_DYNCREATE(CWorkerThread, CWinThread)

CWorkerThread::CWorkerThread()
{
	m_pPThread = NULL;
	m_session = NULL;
}

BOOL CWorkerThread::SetThread(HWND main_thread, WINI::SessionImpl* session)
{
	//m_pView = pView;
	m_idMainThread = main_thread;
	m_session = session;
	return TRUE;
}

void CWorkerThread::SetThread2(HWND main_thread, WINI::SessionImpl* session)
{
	if(NULL == session)
		return;

	m_idMainThread = main_thread;

	m_session = session;

	//just have the worker thread use the same thread as the gui thread
//	CWinThread* MyThread = AfxBeginThread(CloneSRBConnection, &m_session, THREAD_PRIORITY_NORMAL, 0,0, NULL);
}

CWorkerThread::~CWorkerThread()
{
//	As long as both threads use the same struct we do not delete datastruct in worker thread
//	This code was to be used in case a terminate without an exit instance was called.
//	if(m_session)
//		delete m_session;
}

BOOL CWorkerThread::InitInstance()
{
	// TODO:  perform and per-thread initialization here
//	m_pMainWnd = new CMainWindow;
//	m_pMainWnd->ShowWindow(SW_SHOW);
//	m_pMainWnd->UpdateWindow();

	return TRUE;
}

int CWorkerThread::ExitInstance()
{
	//As long as both threads use the same struct we do not delete datastruct in worker thread
	//if(m_session)
	//{
	//	delete m_session;
	//	m_session = NULL;
	//}

	//VERIFY(::WaitForSingleObject (killWorker.m_hObject, 0) == WAIT_OBJECT_0);

	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CWorkerThread, CWinThread)
	//{{AFX_MSG_MAP(CWorkerThread)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	ON_REGISTERED_THREAD_MESSAGE(msgOpen, OnOpen)
	ON_REGISTERED_THREAD_MESSAGE(msgDownload, OnDownload)
	ON_REGISTERED_THREAD_MESSAGE(msgUpload, OnUpload)
	ON_REGISTERED_THREAD_MESSAGE(msgDelete, OnDelete)
	ON_REGISTERED_THREAD_MESSAGE(msgNewNode, OnNewNode)
	ON_REGISTERED_THREAD_MESSAGE(msgReplicate, OnReplicate)
	ON_REGISTERED_THREAD_MESSAGE(msgCopy, OnCopy)
	ON_REGISTERED_THREAD_MESSAGE(msgSynchronize, OnSynchronize)
	ON_REGISTERED_THREAD_MESSAGE(msgRename, OnRename)
	ON_REGISTERED_THREAD_MESSAGE(msgSetComment, OnSetComment)
		//}}AFX_MSG_MAP

END_MESSAGE_MAP()
/*

*/

void CWorkerThread::OnOpen(WPARAM wParam, LPARAM lParam)
{
	open_node* open = (open_node*)wParam;
	::SendMessage(m_idMainThread, msgGoAnimation, NULL, NULL);
	m_session->OpenTree(open->node, open->levels, open->clear, open->mask);
	::SendMessage(m_idMainThread, msgStopAnimation, NULL, NULL);
	::SendMessage(m_idMainThread, msgOpenCompleted, (WPARAM)(open->node), NULL);
	//need an update tree message here

//	return 0;
}


UINT ProgressThreadFunc(LPVOID pParam)
{
	ProgThreadParams* param = (ProgThreadParams*)pParam;
	CEvent* pFinished = param->finished;
	WINI::ITransferOperator* pOp = param->op;
	HWND handle = param->handle;
	char* pszName = param->name;
	UINT msg = param->msg;

	int value, old_value;

	value = old_value = 0;

	int op_type = pOp->GetType();

	while(true)
	{
		if(WINI_OP_COLLECTION == op_type)
			//value = pOp->GetProgress(&pszName);
			value = pOp->GetProgress(NULL);
		else
			value = pOp->GetProgress(NULL);

		if(value >= 0)
		{
			if(value != old_value)
			{
				::SendMessage(handle, msg, (WPARAM)pszName, value);
				old_value = value;
			}
		}

		//have some code here to raise this flag when somebody's exiting the program
		//in the middle of a download.
		if(::WaitForSingleObject (pFinished->m_hObject, 0) == WAIT_OBJECT_0)
		{
			free(param->name);
			delete param;
			return 0;
		}

		::Sleep(1000);
	}

	free(param->name);

	return -1;
}

/////////////////////////////////////////////////////////////////////////////
// CWorkerThread message handlers

//Download datasets/collections to your local machine
void CWorkerThread::OnDownload(WPARAM wParam, LPARAM lParam)
{
	dl* mydl = (dl*)lParam;

	::SendMessage(m_idMainThread, msgGoAnimation, NULL, NULL);

	CEvent finished;
	CEvent* gfinished = &finished;

	WINI::ITransferOperator* dOp = (WINI::ITransferOperator*)m_session->GetOperator(mydl->node->GetType());

	struct ProgThreadParams* params = new ProgThreadParams;

	params->op = dOp;
	params->finished = &finished;
	params->name = strdup(mydl->node->GetName());
	params->handle = m_idMainThread;
	params->msg = msgDLProgress;

	//thread is created suspended so that we may grab a copy of the thread's handle.
	//by copying the handle, we increase the thread's handle count, so it will not free.

	HANDLE progthread;
	m_pPThread = AfxBeginThread (ProgressThreadFunc, params, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	::DuplicateHandle (GetCurrentProcess (), m_pPThread->m_hThread, GetCurrentProcess (), &progthread, 0, FALSE, DUPLICATE_SAME_ACCESS);
	m_pPThread->ResumeThread();

	WINI::StatusCode status = dOp->Download(mydl->node, mydl->local_path);
	
	//after we signal the progress thread to end, we _must_ make sure the thread completes
	//before this thread completes. Otherwise, we can run into problems where memory is not
	//freed and/or the progress thread is sending messages with values from previous downloads.
	VERIFY(finished.SetEvent());
	::WaitForSingleObject(progthread, INFINITE);
	m_pPThread = NULL;
	VERIFY(::CloseHandle(progthread));

	gCount--;

	char buf[1024];

	if(status.isOk())
	{
		if(wParam)
		{
			status = Execute(mydl->node, mydl->local_path);	//download and execute

			if(status.isOk())
				sprintf(buf, "executing %s succeeded", mydl->node->GetName());
			else
				sprintf(buf, "failed to execute %s : %s", mydl->node->GetName(), status.GetError());
		}else
		{
			sprintf(buf, "downloading %s succeeded", mydl->node->GetName());
		}
	}else
	{
		sprintf(buf, "failed to download %s : %s", mydl->node->GetName(), status.GetError());
	}

	char *message = strdup(buf);

	::SendMessage(m_idMainThread, msgDLProgress, NULL, 0);
	::SendMessage(m_idMainThread, msgStatusLine, (WPARAM)message, 1);	//1 toggles free after use

	free((void*)mydl->local_path);
	delete mydl;

	if(0 == gCount)
	{
		::SendMessage(m_idMainThread, msgStopAnimation, NULL, NULL);
	}

	//return 0;
}

//Upload files from your local machine into WINI
void CWorkerThread::OnUpload(WPARAM wParam, LPARAM lParam)
{
	dl* mydl = (dl*)lParam;

	::SendMessage(m_idMainThread, msgGoAnimation, NULL, NULL);

	CEvent finished;

	WINI::ITransferOperator* pOp;

	if(PathIsDirectory(mydl->local_path))
		pOp = (WINI::ITransferOperator*)m_session->GetOperator(WINI_COLLECTION);
	else
		pOp = (WINI::ITransferOperator*)m_session->GetOperator(WINI_DATASET);

	pOp->Bind(mydl->binding);

	struct ProgThreadParams* params = new ProgThreadParams;

	params->op = pOp;
	params->finished = &finished;
	params->name = strdup(mydl->local_path);
	params->handle = m_idMainThread;
	params->msg = msgULProgress;

	HANDLE progthread;
	m_pPThread = AfxBeginThread (ProgressThreadFunc, params, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	::DuplicateHandle (GetCurrentProcess (), m_pPThread->m_hThread, GetCurrentProcess (), &progthread, 0, FALSE, DUPLICATE_SAME_ACCESS);
	m_pPThread->ResumeThread();

	WINI::StatusCode status = pOp->Upload(mydl->node, mydl->local_path, WINI_NO_OVERWRITE);

	static char buf[MAX_PATH];

	//fix this for WINI if(DATA_NOT_UNIQUE_IN_COLLECTION == status)
	if(0 != status)
	{
		if(mydl->node->GetType() == WINI_COLLECTION)
		{
			int len = strlen(mydl->local_path);
			const char* name = NULL;
			int i;
			for(i = len - 1; i != 0; i--)
			{
				if('\\' == (mydl->local_path)[i])
				{
					name = &(mydl->local_path[i+1]);	//found the beginning of our name
					break;
				}
			}
			if(NULL == name)
				name = mydl->local_path;

			int match = 0;
			for(int i = 0; i < mydl->node->CountChildren(); i++)
			{
				if(0 == strcmp(name, mydl->node->GetChild(i)->GetName()))
					if(++match > 1)
						break;
			}

			ASSERT(0 != match);
			//overwrites are allowed in 118p for singles, but not replicas (overwriting replicas not handled correctly in 118p)
			if(1 == match)
				if(IDYES == AfxMessageBox("Overwrite existing data?", MB_YESNO))
					status = pOp->Upload(mydl->node, mydl->local_path, WINI_OVERWRITE);
		}
	}
	
	VERIFY(finished.SetEvent());
	::WaitForSingleObject(progthread, INFINITE);
	m_pPThread = NULL;
	VERIFY(::CloseHandle(progthread));

	gCount--;

	if(status.isOk())
		sprintf(buf, "%s was uploaded successfully.", mydl->local_path);
	else
		sprintf(buf, "could not upload %s : %s", mydl->local_path, status.GetError());

	char* message = strdup(buf);

	::SendMessage(m_idMainThread, msgULProgress, NULL, 0);
	::SendMessage(m_idMainThread, msgStatusLine, (WPARAM)message, 1);

	free((void*)mydl->local_path);
	delete mydl;

	::SendMessage(m_idMainThread, msgRefresh, NULL, 2);

	if(0 == gCount)
		::SendMessage(m_idMainThread, msgStopAnimation, NULL, NULL);

	//return 0;
}


void CWorkerThread::OnRename(WPARAM wParam, LPARAM lParam)
{
	::SendMessage(m_idMainThread, msgGoAnimation, NULL, NULL);

	WINI::INode* node = (WINI::INode*)lParam;

	char* new_name = (char*)wParam;

	ASSERT(NULL != new_name);

	int type = node->GetType();

	WINI::IOperator* dOp = m_session->GetOperator(type);

	WINI::StatusCode status;

	char* name = strdup(node->GetName());

	switch(node->GetType())
	{
	case WINI_DATASET:
		status = ((WINI::IDatasetOperator*)dOp)->Rename((WINI::IDatasetNode*)node, new_name);
		break;
	case WINI_COLLECTION:
		status = ((WINI::ICollectionOperator*)dOp)->Rename((WINI::ICollectionNode*)node, new_name);
		break;
	default:
		status = WINI_ERROR_INVALID_PARAMETER;
		break;
	}

	static char buf[256];

	gCount--;

	if(status.isOk())
	{
		sprintf(buf, "renamed %s", name);
	}else
	{
		sprintf(buf, "could not rename %s : %s", name, status.GetError());
	}

	free(name);
	free(new_name);

	char* message = strdup(buf);

	::PostMessage(m_idMainThread, msgStatusLine, (WPARAM)message, 1);	//1 toggles free after use

	if(0 == gCount)
		::SendMessage(m_idMainThread, msgStopAnimation, NULL, NULL);

	::PostMessage(m_idMainThread, msgRefresh, NULL, 2);

	//return 0;
}

void CWorkerThread::OnDelete(WPARAM wParam, LPARAM lParam)
{
	::SendMessage(m_idMainThread, msgGoAnimation, NULL, NULL);

	WINI::INode* node = (WINI::INode*)lParam;

	int type = node->GetType();

	WINI::IOperator* dOp = m_session->GetOperator(type);

	WINI::StatusCode status;

	char* name = strdup(node->GetName());

	WINI::INode* parent;
	WINI::INode* blahblah;

	switch(type)
	{
	case WINI_DATASET:
		status = ((WINI::IDatasetOperator*)dOp)->Delete((WINI::IDatasetNode*)node);
		break;
	case WINI_COLLECTION:
		status = ((WINI::ICollectionOperator*)dOp)->Delete((WINI::ICollectionNode*)node);
		break;
	//case WINI_CONTAINER:
	//	blahblah = node->GetParent();
	//	status = ((WINI::IContainerOperator*)dOp)->Delete((WINI::IContainerNode*)node);
	//	break;
	case WINI_METADATA:
		parent = node->GetParent();
		dOp = m_session->GetOperator(parent->GetType());
		switch(parent->GetType())
		{
		case WINI_DATASET:
			status = ((WINI::IDatasetOperator*)dOp)->DeleteMeta((WINI::IMetadataNode*)node);
			break;
		case WINI_COLLECTION:
			status = ((WINI::ICollectionOperator*)dOp)->DeleteMeta((WINI::IMetadataNode*)node);
			break;
		case WINI_QUERY:
			status = WINI_ERROR_QUERY_DELETE_FORBIDDEN;
			break;
		default:
			status = WINI_ERROR_INVALID_PARAMETER;
			break;
		}
		break;
	default:
		status = WINI_ERROR_INVALID_PARAMETER;
		break;
	}

	static char buf[256];

	gCount--;

	if(status.isOk())
	{
		sprintf(buf, "deleted %s", name);
	}else
	{
		sprintf(buf, "could not delete %s : %s", name, status.GetError());
	}

	free(name);

	char* message = strdup(buf);

	::PostMessage(m_idMainThread, msgStatusLine, (WPARAM)message, 1);	//1 toggles free after use

	if(0 == gCount)
		::PostMessage(m_idMainThread, msgStopAnimation, NULL, NULL);

	::PostMessage(m_idMainThread, msgRefresh, NULL, 2);

//	if(WINI_CONTAINER == type)
//		::PostMessage(m_idMainThread, msgRefillResourceList, (WPARAM)blahblah, (LPARAM)node);	//for containers
		//::PostMessage(m_idMainThread, msgRefillResourceList, (WPARAM)m_session->GetResource(), NULL);	//for containers
//
	//return 0;
}

void CWorkerThread::OnCopy(WPARAM wParam, LPARAM lParam)
{
	::SendMessage(m_idMainThread, msgGoAnimation, NULL, NULL);

	copy_op* params = (copy_op*)lParam;

	WINI::ICollectionOperator* cOp = (WINI::ICollectionOperator*)m_session->GetOperator(WINI_COLLECTION);

	WINI::StatusCode status = cOp->Bind(params->resource);

	char* name = strdup((char*)params->node->GetName());

	switch(params->node->GetType())
	{
	case WINI_DATASET:
		if(status.isOk())
			if(params->delete_original)
				status = cOp->Move((WINI::ICollectionNode*)params->target, (WINI::IDatasetNode*)params->node);
			else
				status = cOp->Copy((WINI::ICollectionNode*)params->target, (WINI::IDatasetNode*)params->node);
		break;
	case WINI_COLLECTION:
		if(status.isOk())
			if(params->delete_original)
				status = cOp->Move((WINI::ICollectionNode*)params->target, (WINI::ICollectionNode*)params->node);
			else
				status = cOp->Copy((WINI::ICollectionNode*)params->target, (WINI::ICollectionNode*)params->node);
		break;
	default:
		status = WINI_ERROR_INVALID_PARAMETER;
		break;
	}

	static char buf[256];

	gCount--;

	if(status.isOk())
	{
		if(params->delete_original)
			sprintf(buf, "moved %s", name);
		else
			sprintf(buf, "copied %s", name);
	}else
	{
		if(params->delete_original)
			sprintf(buf, "could not complete move %s : %s", name, status.GetError());
		else
			sprintf(buf, "could not copy %s : %s", name, status.GetError());
	}

	free(name);

	char* message = strdup(buf);

	::PostMessage(m_idMainThread, msgStatusLine, (WPARAM)message, 1);	//1 toggles free after use

	::PostMessage(m_idMainThread, msgRefresh, NULL, 4);

	if(0 == gCount)
		::PostMessage(m_idMainThread, msgStopAnimation, NULL, NULL);

	delete params;

	//return 0;
}

void CWorkerThread::OnReplicate(WPARAM wParam, LPARAM lParam)
{
	::SendMessage(m_idMainThread, msgGoAnimation, NULL, NULL);

	new_replicant* blah = (new_replicant*)lParam;

	int type = blah->node->GetType();

	WINI::IOperator* dOp;

	WINI::StatusCode status;

	switch(type)
	{
	case WINI_COLLECTION:
		dOp = m_session->GetOperator(WINI_COLLECTION);
		((WINI::ICollectionOperator*)dOp)->Bind(blah->binding);
		status = ((WINI::ICollectionOperator*)dOp)->Replicate((WINI::ICollectionNode*)blah->node);
		break;
	case WINI_DATASET:
		dOp = m_session->GetOperator(WINI_DATASET);
		((WINI::IDatasetOperator*)dOp)->Bind(blah->binding);
		status = ((WINI::IDatasetOperator*)dOp)->Replicate((WINI::IDatasetNode*)blah->node);
		break;
	default:
		status = WINI_ERROR_INVALID_PARAMETER;
	}

	static char buf[256];

	gCount--;

	if(status.isOk())
		sprintf(buf, "replicated %s", blah->node->GetName());
	else
		sprintf(buf, "could not replicate %s : %s", blah->node->GetName(), status.GetError());


	char* message = strdup(buf);

	::PostMessage(m_idMainThread, msgStatusLine, (WPARAM)message, 1);	//1 toggles free after use

	delete blah;

	if(0 == gCount)
		::SendMessage(m_idMainThread, msgStopAnimation, NULL, NULL);

	::PostMessage(m_idMainThread, msgRefresh, NULL, 5);

//	return 0;

}

void CWorkerThread::OnNewNode(WPARAM wParam, LPARAM lParam)
{
	::SendMessage(m_idMainThread, msgGoAnimation, NULL, NULL);

	new_node* blah = (new_node*)lParam;

	int type = blah->parent->GetType();

	WINI::IOperator* dOp;

	WINI::StatusCode status;

	if(!blah->parent->isOpen())
		m_session->OpenTree(blah->parent, 1, true);

	switch(type)
	{
	case WINI_COLLECTION:
		dOp = m_session->GetOperator(WINI_COLLECTION);
		status = ((WINI::ICollectionOperator*)dOp)->Create((WINI::ICollectionNode*)blah->parent, blah->name);
		break;
	case WINI_RESOURCE:
		//dOp = m_session->GetOperator(WINI_RESOURCE);
		//status = ((WINI::IResourceOperator*)dOp)->Create((WINI::IResourceNode*)blah->parent, blah->name);
		break;
	default:
		status = WINI_ERROR_INVALID_PARAMETER;
	}

	static char buf[256];

	gCount--;

	if(status.isOk())
		sprintf(buf, "created %s", blah->name);
	else
		sprintf(buf, "could not create %s : %s", blah->name, status.GetError());

	char* message = strdup(buf);

	::PostMessage(m_idMainThread, msgStatusLine, (WPARAM)message, 1);	//1 toggles free after use

	free(blah->name);
	delete blah;

	if(0 == gCount)
		::SendMessage(m_idMainThread, msgStopAnimation, NULL, NULL);

	if(WINI_COLLECTION == type)
		::PostMessage(m_idMainThread, msgRefresh, NULL, 2);
	else
	{
		::PostMessage(m_idMainThread, msgRefresh, NULL, 2);	//for containers
		::PostMessage(m_idMainThread, msgRefillResourceList, NULL, NULL);	//for containers
	}
	
	//return 0;
}

BOOL CWorkerThread::OnIdle(LONG lCount) 
{
	if(lCount > 2000)
	{
		this->SuspendThread();
		return 0;
	}

	return CWinThread::OnIdle(lCount);
}

LRESULT CWorkerThread::Execute(WINI::INode* node, const char* local_path)
{
	if(NULL == local_path)
		return WINI_ERROR_INVALID_PARAMETER;

	static char buf[MAX_PATH];

	sprintf(buf, "%s\\%s", local_path, node->GetName());

	SHELLEXECUTEINFO myinfo;

    myinfo.cbSize = sizeof(SHELLEXECUTEINFO);
    myinfo.fMask = SEE_MASK_NOCLOSEPROCESS;

    myinfo.hwnd = NULL;
    myinfo.lpVerb = "open";
    myinfo.lpFile = buf;
    myinfo.lpParameters = NULL;
    myinfo.lpDirectory = NULL;
    myinfo.nShow = SW_SHOW;
    myinfo.hInstApp;

	int status = WINI_ERROR;

	if(ShellExecuteEx(&myinfo))
		status = WINI_OK;

	return status;
}

void CWorkerThread::OnSynchronize(WPARAM wParam, LPARAM lParam)
{
#if 0
	::SendMessage(m_idMainThread, msgGoAnimation, NULL, NULL);

	WINI::IContainerNode* blah = (WINI::IContainerNode*)lParam;

	if(WINI_CONTAINER != blah->GetType())
		return;// WINI_ERROR_INVALID_PARAMETER;

	WINI::IContainerOperator* Op = (WINI::IContainerOperator*)m_session->GetOperator(WINI_CONTAINER);

	WINI::StatusCode status = ((WINI::IContainerOperator*)Op)->Sync(blah, (unsigned int)wParam);

	static char buf[256];

	gCount--;

	if(status.isOk())
		sprintf(buf, "synchronized %s", blah->GetName());
	else
		sprintf(buf, "could not synchronize %s : %s", blah->GetName(), status.GetError());

	char* message = strdup(buf);

	::PostMessage(m_idMainThread, msgStatusLine, (WPARAM)message, 1);	//1 toggles free after use

	if(0 == gCount)
		::SendMessage(m_idMainThread, msgStopAnimation, NULL, NULL);

	::PostMessage(m_idMainThread, msgRefresh, NULL, NULL);

	//return 0;
#endif
}

void CWorkerThread::OnSetComment(WPARAM wParam, LPARAM lParam)
{
	::SendMessage(m_idMainThread, msgGoAnimation, NULL, NULL);

	char* comment = (char*)wParam;
	WINI::IDatasetNode* node = (WINI::IDatasetNode*)lParam;
	WINI::IDatasetOperator* dOp = (WINI::IDatasetOperator*)m_session->GetOperator(WINI_DATASET);
	WINI::StatusCode status = dOp->SetComment(node, comment);

	gCount--;

	char buf[1024];

	if(status.isOk())
		sprintf(buf, "setting %s's system comment successful", node->GetName());
	else
		sprintf(buf, "failed to set %s's comment: %s", node->GetName(), status.GetError());

	char *message = strdup(buf);

	::SendMessage(m_idMainThread, msgStatusLine, (WPARAM)message, 1);	//1 toggles free after use

	free(comment);

	::SendMessage(m_idMainThread, msgRefresh, NULL, 2);

	if(0 == gCount)
		::SendMessage(m_idMainThread, msgStopAnimation, NULL, NULL);

	//return 0;
}
