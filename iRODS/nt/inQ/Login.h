#if !defined(AFX_LOGIN_H__6D9E96B8_E064_4011_9AB1_F766EA574DE1__INCLUDED_)
#define AFX_LOGIN_H__6D9E96B8_E064_4011_9AB1_F766EA574DE1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Login.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLogin dialog

class CLogin : public CDialog
{
// Construction
public:
	CLogin(CWnd* pParent = NULL);   // standard constructor
	~CLogin();
	bool SetDefaultResource(const char* resource);
	const char* GetMyName();
	const char* GetZone() { return m_szZone;};
	const char* GetHost() { return m_szHost;};
	const char* GetPassword() { return m_szPassword;};
	const char* GetPort() { return m_szPort;};
	const char* GetHome();
	const char* GetDomainHome() { return "home";};
	const char* GetDefaultResource() { return m_szDefaultResource;};
	void FillBoxes(int i, bool FillListBox);
// Dialog Data
	//{{AFX_DATA(CLogin)
	enum { IDD = IDD_LOGIN };
	CEdit	m_Password;
	CListCtrl	m_ListBox;
	CComboBox	m_Port;
	CComboBox	m_Name;
	CComboBox	m_Host;
	CComboBox	m_Zone;
	//}}AFX_DATA

	char* m_szPort;
	char* m_szName;
	char* m_szHost;
	char* m_szZone;
	char* m_szPassword;
	char* m_szHome;
	char* m_szDefaultResource;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLogin)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	int m_profile_number;
	// Generated message map functions
	//{{AFX_MSG(CLogin)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnLoginNew();
	afx_msg void OnLoginRemove();
	afx_msg void OnClickList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLoginDemo();
	afx_msg void OnEditchangeLoginZone();
	afx_msg void OnEditchangeLoginHost();
	afx_msg void OnEditchangeLoginName();
	afx_msg void OnEditchangeLoginPort();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOGIN_H__6D9E96B8_E064_4011_9AB1_F766EA574DE1__INCLUDED_)
