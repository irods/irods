#if !defined(AFX_ACCESSCTRLUSERADD_H__EE2D3FAB_6033_4FCF_A9E6_0CCA3062693B__INCLUDED_)
#define AFX_ACCESSCTRLUSERADD_H__EE2D3FAB_6033_4FCF_A9E6_0CCA3062693B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AccessCtrlUserAdd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAccessCtrlUserAdd dialog

class CAccessCtrlUserAdd : public CDialog
{
// Construction
public:
	CAccessCtrlUserAdd(CWnd* pParent = NULL);   // standard constructor
	CAccessCtrlUserAdd(WINI::ICatalogNode* mcat, WINI::IStringNode* constraints, CWnd* pParent = NULL);   // standard constructor

	void Update();
	WINI::IUserNode* GetUser() { return m_newUser;};
	const char* GetPermission() { return m_szConstraint;};
	CWnd* m_pParent;
// Dialog Data
	//{{AFX_DATA(CAccessCtrlUserAdd)
	enum { IDD = IDD_ACCESS_CTRL_USER_ADD };
	CListBox	m_Zone;
	CComboBox	m_CC;
	CListBox	m_User;
	CListBox	m_Domain;
	CListBox	m_ListDomains;
	CListBox	m_ListUsers;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAccessCtrlUserAdd)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAccessCtrlUserAdd)
	afx_msg void OnSelchangeUserAddDomain();
	afx_msg void OnSelchangeUserAddUser();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSelchangeListzone();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	WINI::ICatalogNode* m_mcat;
	WINI::IStringNode* m_constraints;
	WINI::IUserNode* m_newUser;
	char* m_szConstraint;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACCESSCTRLUSERADD_H__EE2D3FAB_6033_4FCF_A9E6_0CCA3062693B__INCLUDED_)
