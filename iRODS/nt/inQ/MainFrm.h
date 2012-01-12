// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__8AE77304_0BDB_4F77_AE42_0B4A92E248BF__INCLUDED_)
#define AFX_MAINFRM_H__8AE77304_0BDB_4F77_AE42_0B4A92E248BF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "winiObjects.h"


class CInquisitorView;
class CLeftView;
class CMetaView;


class CMainFrame : public CFrameWnd
{
protected:
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)
	CSplitterWnd m_wndSplitter;
	CSplitterWnd m_wndSplitter2;
public:
	CStatusBar  m_wndStatusBar;
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL
	virtual ~CMainFrame();
	CInquisitorView* GetRightPane();
	CLeftView* GetLeftPane();
	CMetaView* GetMetaPane();
	LRESULT OnRefresh(WPARAM wParam, LPARAM lParam);
	LRESULT OnUpdateCopy(WPARAM wParam, LPARAM lParam);
	void SetStatusMessage(const char* message);
	void SetCountMessage(const char* message);
	void SetFocusMeta();
	void SetLoginMessage(const char* message);
	int Push(const char* name);
	void FillResourceList(WINI::INode* node);
	LRESULT OnStatusMessage1(WPARAM wParam, LPARAM lParam);
	LRESULT OnStatusMessage2(WPARAM wParam, LPARAM lParam);
	LRESULT OnRefillResourceList(WPARAM wParam, LPARAM lParam);
	LRESULT OnStatusLine(WPARAM wParam, LPARAM lParam);
	LRESULT OnDLProgress(WPARAM wParam, LPARAM lParam);
	LRESULT OnULProgress(WPARAM wParam, LPARAM lParam);
	LRESULT OnConnectedUpdate(WPARAM wParam, LPARAM lParam);
	void FooFighter();
	void ClearStack(bool resourcetoo);
	void SetStack(int i);
	LRESULT OnStopAnimation(WPARAM wParam, LPARAM lParam);
	LRESULT OnGoAnimation(WPARAM wParam, LPARAM lParam);
	WINI::StatusCode SetResource(WINI::INode* ResourceORContainer);
	LRESULT OnProgressRotate(WPARAM wParam, LPARAM lParam);
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	CToolBar	m_wndToolBar;
protected:
	CProgressCtrl m_wndProgress;
	CToolBar	m_wndToolBar2;
	CDialogBar	m_wndDialogBar;
	CDialogBar	m_wndDialogBar2;
	CDialogBar	m_wndDialogBar3;
	//CDialogBar	m_wndDialogBar4;
	CToolBar	m_wndViewsBar;
	CReBar		m_wndReBar;
	CAnimateCtrl m_wndAnimate;
	bool m_isPlaying;
	bool m_finishedInitializing;
	bool m_bProgressEnabled;
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg void OnTreeUp();
	afx_msg void OnTreeHome();
	afx_msg void OnSelChangeTreeStack();
	afx_msg void OnSelchangeResource();
	afx_msg void OnBack();
	afx_msg void OnForward();
	afx_msg void OnFooFighter();
	afx_msg void OnMetadata();
	afx_msg void OnQuery();
	afx_msg void OnAccessCtrl();
	afx_msg void OnChangePassword();
	afx_msg void OnUpdateChangePassword(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTreeUp(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFooFighter(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTreeHome(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewJumpviewbar(CCmdUI* pCmdUI);
	afx_msg void OnViewJumpviewbar();
	afx_msg void OnUpdateViewStyles(CCmdUI* pCmdUI);
	afx_msg void OnViewStyle(UINT nCommandID);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__8AE77304_0BDB_4F77_AE42_0B4A92E248BF__INCLUDED_)
