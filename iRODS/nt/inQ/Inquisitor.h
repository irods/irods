// Inquisitor.h : main header file for the INQUISITOR application
//

#if !defined(AFX_INQUISITOR_H__37BD70D5_1A10_4C19_BD3F_083D06461F28__INCLUDED_)
#define AFX_INQUISITOR_H__37BD70D5_1A10_4C19_BD3F_083D06461F28__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CInquisitorApp:
// See Inquisitor.cpp for the implementation of this class
//


class CInquisitorApp : public CWinApp
{
public:
	CInquisitorApp();
	char m_szTempPath[MAX_PATH];
	//charlieccharliec

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInquisitorApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual void DoWaitCursor(int nCode);
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CInquisitorApp)
	afx_msg void OnAppAbout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INQUISITOR_H__37BD70D5_1A10_4C19_BD3F_083D06461F28__INCLUDED_)
