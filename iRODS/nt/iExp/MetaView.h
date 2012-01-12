#if !defined(AFX_METAVIEW_H__AA581DFF_B3A2_4364_BF43_3C9DB0781F63__INCLUDED_)
#define AFX_METAVIEW_H__AA581DFF_B3A2_4364_BF43_3C9DB0781F63__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MetaView.h : header file
//

#include "InquisitorDoc.h"

/////////////////////////////////////////////////////////////////////////////
// CMetaView view

class CMetaView : public CListView
{
public:
	void Refresh() { }; //OnUpdate(NULL, NULL, NULL);};
	void NewMeta() { OnNewMeta();};

protected:
	CMetaView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CMetaView)

// Attributes
public:
	int HitTestEx(CPoint &point, int *col) const;
	CEdit* EditSubLabel( int nItem, int nCol );
// Operations
public:
#ifndef _DEBUG  // debug version in InquisitorView.cpp
inline CInquisitorDoc* GetDocument()
   { return (CInquisitorDoc*)m_pDocument; }
#else
	CInquisitorDoc* GetDocument();
#endif
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMetaView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CMetaView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	std::vector<int> m_nRedirector;
	bool m_bInitialized;
	bool m_bEditing;
	// Generated message map functions
private:
	void GetImage(const int& node_type, const char* sub_type, bool isOpen, int &image, int& selected_image);

protected:
	//{{AFX_MSG(CMetaView)
	afx_msg void OnSetfocus(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnUpdateNewCollection(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNewContainer(CCmdUI* pCmdUI);
	//afx_msg void OnUpdateQuery(CCmdUI* pCmdUI);
	afx_msg void OnUpdateUpload(CCmdUI* pCmdUI);
	afx_msg void OnUpdateUploadFolder(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDownload(CCmdUI* pCmdUI);
	afx_msg void OnUpdateReplicate(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSynchronize(CCmdUI* pCmdUI);
	afx_msg void OnUpdateAccessCtrl(CCmdUI* pCmdUI);
	//afx_msg void OnDelete();
	afx_msg void OnMetadata();
	//afx_msg void OnUpdateMetadata(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDelete(CCmdUI* pCmdUI);
	afx_msg void OnRename();
	afx_msg void OnUpdateRename(CCmdUI* pCmdUI);
	afx_msg void OnUpdateComment(CCmdUI* pCmdUI);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNewMeta();
	afx_msg void OnUpdateNewMeta(CCmdUI* pCmdUI);
	afx_msg void OnRclick(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_METAVIEW_H__AA581DFF_B3A2_4364_BF43_3C9DB0781F63__INCLUDED_)
