// OleInqDataSource.cpp : implementation file
//

#include "stdafx.h"
#include "inquisitor.h"
#include "OleInqDataSource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COleInqDataSource

IMPLEMENT_DYNCREATE(COleInqDataSource, COleDataSource)

COleInqDataSource::COleInqDataSource()
{
}

COleInqDataSource::~COleInqDataSource()
{
}


BEGIN_MESSAGE_MAP(COleInqDataSource, COleDataSource)
	//{{AFX_MSG_MAP(COleInqDataSource)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COleInqDataSource message handlers
