#if !defined(AFX_CHARLIECEDIT_H__97E98DA1_C2BF_49A0_A46E_058F28F53FC9__INCLUDED_)
#define AFX_CHARLIECEDIT_H__97E98DA1_C2BF_49A0_A46E_058F28F53FC9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CharlieCEdit.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCEdit window

class CCEdit : public CEdit
{
// Construction
public:
	CCEdit(int iItem, int iSubItem, CString sInitText);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCEdit)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCEdit();

	// Generated message map functions
protected:
	//{{AFX_MSG(CCEdit)
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnNcDestroy();
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
private:
	int m_iItem;
	int m_iSubItem;
	CString m_sInitText;
	BOOL    m_bESC;	 	// To indicate whether ESC key was pressed
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHARLIECEDIT_H__97E98DA1_C2BF_49A0_A46E_058F28F53FC9__INCLUDED_)






