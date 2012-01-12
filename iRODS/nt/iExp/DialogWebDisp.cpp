// DialogWebDisp.cpp : implementation file
//

#include "stdafx.h"
#include "Inquisitor.h"
#include "DialogWebDisp.h"


// CDialogWebDisp dialog

IMPLEMENT_DYNAMIC(CDialogWebDisp, CDialog)

CDialogWebDisp::CDialogWebDisp(CString & HtmlStr, CWnd* pParent /*=NULL*/)
	: CDialog(CDialogWebDisp::IDD, pParent)
{
	m_pageLoaded = false;
	m_htmlStr = HtmlStr;
	m_dlgInitialized = false;
}

CDialogWebDisp::~CDialogWebDisp()
{
}

void CDialogWebDisp::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WEBDISP_EXPLORER, m_webdispExplorer);
	DDX_Control(pDX, IDOK, m_okButton);
}


BEGIN_MESSAGE_MAP(CDialogWebDisp, CDialog)
	ON_WM_SIZE()
	ON_WM_CREATE()
END_MESSAGE_MAP()


// CDialogWebDisp message handlers

BOOL CDialogWebDisp::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	RECT wrect;
	GetClientRect(&wrect);

	RECT lrect;
	m_webdispExplorer.GetWindowRect(&lrect);
	ScreenToClient(&lrect);

	m_topOffset = lrect.top - wrect.top;
	m_leftOffset = lrect.left - wrect.left;
	m_rightOffset = wrect.right - lrect.right;

	RECT okrect;
	m_okButton.GetWindowRect(&okrect);
	ScreenToClient(&okrect);

	m_bottomOffset = okrect.top - lrect.bottom;

	CString blankurl("about:blank");
	m_webdispExplorer.Navigate(blankurl, NULL, NULL, NULL, NULL);

	m_dlgInitialized = true;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_EVENTSINK_MAP(CDialogWebDisp, CDialog)
	ON_EVENT(CDialogWebDisp, IDC_WEBDISP_EXPLORER, 259, CDialogWebDisp::DocumentCompleteWebdispExplorer, VTS_DISPATCH VTS_PVARIANT)
END_EVENTSINK_MAP()

void CDialogWebDisp::DocumentCompleteWebdispExplorer(LPDISPATCH pDisp, VARIANT* URL)
{
	// TODO: Add your message handler code here
	if(m_pageLoaded)
		return;

	LoadHtmlString();
}

void CDialogWebDisp::LoadHtmlString()
{
	HRESULT hr;
    IUnknown* pUnkBrowser = NULL;
    IUnknown* pUnkDisp = NULL;
    IStream* pStream = NULL;
    HGLOBAL hHTMLText;

    // Is this the DocumentComplete event for the top frame window?
    // Check COM identity: compare IUnknown interface pointers.
    size_t cchLength = m_htmlStr.GetLength();
	hHTMLText = GlobalAlloc( GPTR, cchLength+1 );
	if(!hHTMLText)
		return;

	_tcscpy((TCHAR*)hHTMLText, m_htmlStr);
	hr = CreateStreamOnHGlobal( hHTMLText, TRUE, &pStream );
	if(!SUCCEEDED(hr))
	{
		GlobalFree( hHTMLText );
		return;
	}

	IDispatch* pHtmlDoc = m_webdispExplorer.get_Document();
	IPersistStreamInit* pPersistStreamInit = NULL;
	hr = pHtmlDoc->QueryInterface( IID_IPersistStreamInit,  (void**)&pPersistStreamInit );
	if(!SUCCEEDED(hr))
	{
		GlobalFree( hHTMLText );
		return;
	}

	hr = pPersistStreamInit->InitNew();
	if(!SUCCEEDED(hr))
	{
		pPersistStreamInit->Release();
		GlobalFree( hHTMLText );
		return;
	}

	hr = pPersistStreamInit->Load( pStream );

	pPersistStreamInit->Release();
	GlobalFree( hHTMLText );

	m_pageLoaded = true;
}
void CDialogWebDisp::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	if(!m_dlgInitialized)
		return;

	RECT lrect;
	int x, y, w, h;

	int dlg_w, dlg_h;
	GetClientRect(&lrect);
	dlg_w = lrect.right;
	dlg_h = lrect.bottom;

	m_okButton.GetWindowRect(&lrect);
	int ok_w = lrect.right - lrect.left;
	int ok_h = lrect.bottom -  lrect.top;
	x = (int)((dlg_w - ok_w)/2);
	y = dlg_h - ok_h -5;
	m_okButton.MoveWindow(x, y, ok_w, ok_h);

	x = 0;
	y = 0;
	w = dlg_w;
	h = dlg_h - ok_h - 10;
	m_webdispExplorer.MoveWindow(x, y, w, h);
}

int CDialogWebDisp::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  Add your specialized creation code here
	SetWindowLong(this->m_hWnd,GWL_STYLE,
                GetWindowLong(this->m_hWnd, GWL_STYLE) | WS_MAXIMIZEBOX);


	return 0;
}
