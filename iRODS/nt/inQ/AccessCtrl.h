#if !defined(AFX_ACCESSCTRL_H__E1A33187_E678_43E1_B703_7B3C3436D28F__INCLUDED_)
#define AFX_ACCESSCTRL_H__E1A33187_E678_43E1_B703_7B3C3436D28F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AccessCtrl.h : header file
//
#include "InquisitorDoc.h"
/////////////////////////////////////////////////////////////////////////////
// CAccessCtrl dialog

class CAccessCtrl : public CDialog
{
// Construction
public:
	CAccessCtrl(CInquisitorDoc* doc, WINI::INode* node, CWnd* pParent = NULL);
	CWnd* m_pParent;
// Dialog Data
	//{{AFX_DATA(CAccessCtrl)
	enum { IDD = IDD_ACCESS_CTRL };
	CButton	m_OK;
	CButton	m_Recursive;
	CListBox	m_ListBox;
	CComboBox	m_AccessConstraint;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAccessCtrl)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
	void Update();
protected:

	// Generated message map functions
	//{{AFX_MSG(CAccessCtrl)
	afx_msg void OnAccessCtrlAdd();
	afx_msg void OnAccessCtrlRemove();
	virtual void OnOK();
	afx_msg void OnSelchangeAccessCtrlConstraint();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
		bool m_isUser;
		CInquisitorDoc* m_doc;
		WINI::IStringNode* m_constraints;
		WINI::INode* m_node;
		std::vector<int> m_nRedirector;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACCESSCTRL_H__E1A33187_E678_43E1_B703_7B3C3436D28F__INCLUDED_)
