#if !defined(AFX_WORKERTHREAD_H__59D88CE2_5762_422B_81D8_D35C1CCF68E9__INCLUDED_)
#define AFX_WORKERTHREAD_H__59D88CE2_5762_422B_81D8_D35C1CCF68E9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WorkerThread.h : header file
//

#include "winiSession.h"
#include "winiObjects.h"
#include "LeftView.h"

/////////////////////////////////////////////////////////////////////////////
// CWorkerThread thread

class CWorkerThread : public CWinThread
{
	DECLARE_DYNCREATE(CWorkerThread)
protected:
	CWorkerThread();           // protected constructor used by dynamic creation
public:
	BOOL SetThread(HWND main_thread, WINI::SessionImpl* session);
	void SetThread2(HWND main_thread, WINI::SessionImpl* session);

	//WINI::StatusCode Copy();
	void Delete(HWND window, WINI::INode* node);
	void OnCopy(WPARAM wParam, LPARAM lParam);
	void OnDownload(WPARAM wParam, LPARAM lParam);
	void OnOpen(WPARAM wParam, LPARAM lParam);
	void OnSetComment(WPARAM wParam, LPARAM lParam);
	void OnUpload(WPARAM wParam, LPARAM lParam);
	void OnRename(WPARAM wParam, LPARAM lParam);
	void OnDelete(WPARAM wParam, LPARAM lParam);
	void OnReplicate(WPARAM wParam, LPARAM lParam);
	void OnNewNode(WPARAM wParam, LPARAM lParam);
	void OnSynchronize(WPARAM wParam, LPARAM lParam);
	LRESULT Execute(WINI::INode* node, const char* local_path);
	virtual ~CWorkerThread();
// Attributes
public:
	WINI::SessionImpl* m_session;
// Operations
public:
CWinThread* m_pPThread;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWorkerThread)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual BOOL OnIdle(LONG lCount);
	//}}AFX_VIRTUAL

// Implementation
protected:
//	virtual ~CWorkerThread();

	// Generated message map functions
	//{{AFX_MSG(CWorkerThread)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

private:
	HWND m_idMainThread;
	//CLeftView* m_pView;
	
	


	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WORKERTHREAD_H__59D88CE2_5762_422B_81D8_D35C1CCF68E9__INCLUDED_)
