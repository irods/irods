// AddressBar.cpp : implementation file
//

#include "stdafx.h"
#include "inquisitor.h"
#include "AddressBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// AddressBar

IMPLEMENT_DYNCREATE(AddressBar, CFormView)

AddressBar::AddressBar()
	: CFormView(AddressBar::IDD)
{
	//{{AFX_DATA_INIT(AddressBar)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

AddressBar::~AddressBar()
{
}

void AddressBar::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(AddressBar)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(AddressBar, CFormView)
	//{{AFX_MSG_MAP(AddressBar)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// AddressBar diagnostics

#ifdef _DEBUG
void AddressBar::AssertValid() const
{
	CFormView::AssertValid();
}

void AddressBar::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// AddressBar message handlers
