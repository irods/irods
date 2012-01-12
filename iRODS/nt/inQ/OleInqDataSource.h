#if !defined(AFX_OLEINQDATASOURCE_H__958B60F5_A21C_4324_B40C_A51EEBEB86C9__INCLUDED_)
#define AFX_OLEINQDATASOURCE_H__958B60F5_A21C_4324_B40C_A51EEBEB86C9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OleInqDataSource.h : header file
//



/////////////////////////////////////////////////////////////////////////////
// COleInqDataSource command target

class COleInqDataSource : public COleDataSource
{
	DECLARE_DYNCREATE(COleInqDataSource)

	COleInqDataSource();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COleInqDataSource)
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~COleInqDataSource();

	// Generated message map functions
	//{{AFX_MSG(COleInqDataSource)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OLEINQDATASOURCE_H__958B60F5_A21C_4324_B40C_A51EEBEB86C9__INCLUDED_)
