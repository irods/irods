#if !defined(AFX_ADDRESSBAR_H__28B1A308_1BCF_4496_BF4D_E2B1D1FACF9C__INCLUDED_)
#define AFX_ADDRESSBAR_H__28B1A308_1BCF_4496_BF4D_E2B1D1FACF9C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AddressBar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// AddressBar form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class AddressBar : public CFormView
{
protected:
	AddressBar();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(AddressBar)

// Form Data
public:
	//{{AFX_DATA(AddressBar)
	enum { IDD = IDD_DIALOGBAR4 };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(AddressBar)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~AddressBar();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(AddressBar)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDRESSBAR_H__28B1A308_1BCF_4496_BF4D_E2B1D1FACF9C__INCLUDED_)
