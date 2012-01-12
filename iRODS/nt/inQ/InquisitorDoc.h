// InquisitorDoc.h : interface of the CInquisitorDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_INQUISITORDOC_H__736A9F8C_911C_46B5_BBF2_446BF4CA7230__INCLUDED_)
#define AFX_INQUISITORDOC_H__736A9F8C_911C_46B5_BBF2_446BF4CA7230__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "winiSession.h"
#include "winiObjects.h"
#include "WorkerThread.h"
#include "Login.h"
#include <stack>
//#include <shldisp.h>

class CInquisitorDoc : public COleDocument
{
protected: // create from serialization only
	CInquisitorDoc();
	DECLARE_DYNCREATE(CInquisitorDoc)

public:
	void OnFailedConnection();
	WINI::SessionImpl MyImpl;
	WINI::StatusCode NukeDir(const char* szpath);
	void Copy(WINI::INode* node, int type, bool cut, char* name, char* path, int replication_index);
	void Copy(WINI::INode* target, WINI::INode* source, bool cut);
	char* GetTempPath();
	void SetSynchronizationFlags(bool purge, bool primary_only);
	void Replicate(WINI::INode* node);
	WINI::StatusCode ChangePassword(const char* password);
	bool isConnected();
	WINI::StatusCode DragOutDownload(LPSTGMEDIUM lpStgMedium);
	void SetAccess(WINI::INode* node);
	bool Disconnect();
	WINI::IStringNode* GetAccessConstraint();
	WINI::StatusCode SetAccess(WINI::INode* target, WINI::IUserNode* owner, const char* permission, bool recursive = false);
	WINI::StatusCode ModifyAccess(WINI::IAccessNode* target, const char* permission, bool recursive = false);
	BOOL OnCompletedConnection();
	WINI::INode* CInquisitorDoc::GetDomains();
	WINI::StatusCode SetMetadataValue(const char* attribute, const char* value);
	WINI::StatusCode SetMetadataValue(WINI::IMetadataNode* node, const char* value);
	WINI::INode* GetRoot();
		WINI::INode* GetLocalRoot();
	WINI::INode* GetHome();
	WINI::INode* GetHomeZone();
	BOOL MyNewConnection();
	WINI::INode* GetMCAT();
	void SetComment(WINI::INode* node, char* comment);
	WINI::StatusCode OpenTree(WINI::INode* node, int levels, bool clear, unsigned int mask = WINI_ALL);
	WINI::ISetNode* GetResources(WINI::IZoneNode* node);
	WINI::StatusCode AddNewMetadata(const char* attribute, const char* value);
	WINI::StatusCode SetDefaultResource(WINI::INode* resource);
	WINI::INode* GetDefaultResource() { return m_defaultResource;};
	void OnReturn();
	void Synchronize(WINI::INode* node);
	void Rename(WINI::INode* node, char* name);
	void Delete(WINI::INode* node);
	bool SuspendWorkThread();
	bool ResumeWorkThread();
	void Download(WINI::INode* node, const char* local_path, bool Execute);
	void Download(WINI::INode* node, bool Execute);
	void Upload(WINI::INode* node, const char* local_path);
	void Execute(const char* local_path);
	void SetCurrentNode(WINI::INode* node);
	WINI::INode* GetCurrentNode();

	void CreateNode(WINI::INode* parent, const char* name);

	void OnBack();
	void OnForward();



//WINI::IZoneNode* GetCurrentZone();

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInquisitorDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual void OnCloseDocument();
	//}}AFX_VIRTUAL

public:
	virtual ~CInquisitorDoc();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CInquisitorDoc)
	afx_msg void OnQuery();
	afx_msg void OnUpdateBack(CCmdUI* pCmdUI);
	afx_msg void OnUpdateForward(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileNew(CCmdUI* pCmdUI);
	afx_msg void OnRefresh();
	afx_msg void OnUpdateRefresh(CCmdUI* pCmdUI);
	afx_msg void OnGetpath();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

	CWinThread* m_pCThread;
	HANDLE m_hCThread;

	//WINI::SessionImpl* MyImpl2;
	CLogin m_Login;
	WINI::INode* m_current_node;
	WINI::INode* m_defaultResource;
	CWorkerThread* m_worker_thread;
	bool m_bIsDemo;
	WINI::INode* m_History[10];
	int m_HistoryLast, m_HistoryFirst, m_HistoryCurrent;
	unsigned int m_synchFlag;

	WINI::IZoneNode* m_zone;

	bool m_eventConnecting;

};

class CharlieSource : public COleDataSource, public IAsyncOperation
{
public:
	~CharlieSource();
	BOOL OnRenderData(LPFORMATETC lpFormatEtc, LPSTGMEDIUM lpStgMedium);
	CInquisitorDoc* m_doc;

	//IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	//IAsyncOperation
	HRESULT STDMETHODCALLTYPE SetAsyncMode(BOOL fDoOpAsync);
	HRESULT STDMETHODCALLTYPE GetAsyncMode(BOOL *pfIsOpAsync);
	HRESULT STDMETHODCALLTYPE StartOperation(IBindCtx *pbcReserved);
	HRESULT STDMETHODCALLTYPE InOperation(BOOL *pfInAsyncOp);
	HRESULT STDMETHODCALLTYPE EndOperation(HRESULT hResult, IBindCtx *pbcReserved, DWORD dwEffects);


private:
	BOOL RenderFileData(LPFORMATETC lpFormatEtc, LPSTGMEDIUM lpStgMedium);
	BOOL RenderInqData(LPFORMATETC lpFormatEtc, LPSTGMEDIUM lpStgMedium);
	WINI::StatusCode DownloadForCopy(LPSTGMEDIUM lpStgMedium);
	std::vector<WINI::INode*> m_NodeDragList;
	int mysentinel;

	BOOL m_AsyncModeOn;
	BOOL m_AsyncTransferInProgress;
	ULONG m_cRef;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INQUISITORDOC_H__736A9F8C_911C_46B5_BBF2_446BF4CA7230__INCLUDED_)
