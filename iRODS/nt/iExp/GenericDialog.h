#if !defined(AFX_GENERICDIALOG_H__5E8BA1D1_BF88_41DB_B286_787925166A31__INCLUDED_)
#define AFX_GENERICDIALOG_H__5E8BA1D1_BF88_41DB_B286_787925166A31__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GenericDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGenericDialog dialog

class CGenericDialog : public CDialog
{
private:
	CString m_dlgTitle;
// Construction
public:
	CGenericDialog( char* szTitle, CWnd* pParent = NULL);   // standard constructor
	
// Dialog Data
	//{{AFX_DATA(CGenericDialog)
	enum { IDD = IDD_GENERIC_DIALOG };
	CString	m_Edit;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGenericDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGenericDialog)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GENERICDIALOG_H__5E8BA1D1_BF88_41DB_B286_787925166A31__INCLUDED_)
