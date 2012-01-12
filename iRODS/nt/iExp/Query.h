#if !defined(AFX_QUERY_H__80896A70_163C_4700_8D14_3AAB4E38034E__INCLUDED_)
#define AFX_QUERY_H__80896A70_163C_4700_8D14_3AAB4E38034E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Query.h : header file
//

#include "winiObjects.h"
#include <vector>

/////////////////////////////////////////////////////////////////////////////
// CQuery dialog

class CQuery : public CDialog
{
// Construction
public:
	CQuery(WINI::ISession* session, WINI::INode* source, CWnd* pParent = NULL);   // standard constructor
	void Fill(WINI::INode* node);
// Dialog Data
	//{{AFX_DATA(CQuery)
	enum { IDD = IDD_QUERY };
	CListBox	m_List;
	CComboBox	m_Value;
	CComboBox	m_Operation;
	CComboBox	m_Attribute;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQuery)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
	public:
		WINI::ISession* m_session;
		WINI::INode* m_source;
		WINI::IQueryNode* m_query;
protected:
	std::vector<WINI::IMetadataNode*> m_list;
	//WINI::IQueryOperator* m_qop;
	WINI::IQueryOperator* m_qOp;


	// Generated message map functions
	//{{AFX_MSG(CQuery)
	afx_msg void OnQueryAdd();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnQueryRemove();
	afx_msg void OnQueryAnd();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QUERY_H__80896A70_163C_4700_8D14_3AAB4E38034E__INCLUDED_)
