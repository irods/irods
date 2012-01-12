#if !defined(AFX_PATHBAR_H__F0D09E38_E623_47BE_83B8_A89CB2449521__INCLUDED_)
#define AFX_PATHBAR_H__F0D09E38_E623_47BE_83B8_A89CB2449521__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PathBar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// PathBar dialog

class PathBar : public CDialog
{
// Construction
public:
	PathBar(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(PathBar)
	enum { IDD = IDD_DIALOGBAR4 };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(PathBar)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(PathBar)
	afx_msg void OnPathgo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PATHBAR_H__F0D09E38_E623_47BE_83B8_A89CB2449521__INCLUDED_)
