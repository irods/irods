#if !defined(AFX_METADATADIALOG_H__80B8FDFC_894A_4D6D_93B2_3CF874B580FA__INCLUDED_)
#define AFX_METADATADIALOG_H__80B8FDFC_894A_4D6D_93B2_3CF874B580FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MetadataDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMetadataDialog dialog

class CMetadataDialog : public CDialog
{
// Construction
public:
	CMetadataDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMetadataDialog)
	enum { IDD = IDD_METADATA };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMetadataDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMetadataDialog)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_METADATADIALOG_H__80B8FDFC_894A_4D6D_93B2_3CF874B580FA__INCLUDED_)
