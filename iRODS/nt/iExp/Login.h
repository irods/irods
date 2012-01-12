#if !defined(AFX_LOGIN_H__6D9E96B8_E064_4011_9AB1_F766EA574DE1__INCLUDED_)
#define AFX_LOGIN_H__6D9E96B8_E064_4011_9AB1_F766EA574DE1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Login.h : header file
//
//#define CLOGIN_TIMER_ID (101+WM_USER)
/////////////////////////////////////////////////////////////////////////////
// CLogin dialog

class CLogin : public CDialog
{
// Construction
public:
	CLogin(CWnd* pParent = NULL);   // standard constructor
	~CLogin();
	bool SetDefaultResource(const char* resource);
	CString GetMyName();
	CString GetZone() { return m_szZone;};
	CString GetHost() { return m_szHost;};
	CString GetPassword() { return m_szPassword;};
	CString GetPort() { return m_szPort;};
	CString GetHome();
	CString GetDomainHome() { return "home";};
	CString GetDefaultResource() { return m_szDefaultResource;};
	void FillBoxes(int i, bool FillListBox);
// Dialog Data
	//{{AFX_DATA(CLogin)
	enum { IDD = IDD_LOGIN };
	CEdit	m_PasswordEditbox;
	CListCtrl	m_ListBox;
	CComboBox	m_PortCombobox;
	CComboBox	m_NameCombobox;
	CComboBox	m_HostCombobox;
	CComboBox	m_ZoneCombobox;
	//}}AFX_DATA

	CString m_szPort;
	CString m_szName;
	CString m_szHost;
	CString m_szZone;
	CString m_szPassword;
	CString m_szHome;
	CString m_szDefaultResource;
	int     m_nProfileId;

	CStringArray m_prevLogins;

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
	virtual void OnCancel();
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
public:
//	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedCheckPrelogins();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOGIN_H__6D9E96B8_E064_4011_9AB1_F766EA574DE1__INCLUDED_)
