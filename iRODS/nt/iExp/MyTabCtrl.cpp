// MyTabCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "Inquisitor.h"
#include "MyTabCtrl.h"


// CMyTabCtrl

IMPLEMENT_DYNAMIC(CMyTabCtrl, CTabCtrl)

CMyTabCtrl::CMyTabCtrl()
{

}

CMyTabCtrl::~CMyTabCtrl()
{
}


BEGIN_MESSAGE_MAP(CMyTabCtrl, CTabCtrl)
//	ON_WM_PAINT()
//	ON_WM_ERASEBKGND()
	ON_WM_CREATE()
//	ON_WM_DRAWITEM()
	ON_WM_PAINT()
END_MESSAGE_MAP()



// CMyTabCtrl message handlers



//void CMyTabCtrl::OnPaint()
//{
//	CPaintDC dc(this); // device context for painting
//	// TODO: Add your message handler code here
//	// Do not call CTabCtrl::OnPaint() for painting messages
//
//	CTabCtrl::OnPaint();
//}

//BOOL CMyTabCtrl::OnEraseBkgnd(CDC* pDC)
//{
//	// TODO: Add your message handler code here and/or call default
//	/*
//	DWORD dw = GetSysColor(COLOR_WINDOW);
//
//	CRect rect;    GetClientRect(&rect);    
//	CBrush myBrush(RGB(GetRValue(dw), GetGValue(dw), GetBValue(dw)));    // dialog background color    
//	CBrush *pOld = pDC->SelectObject(&myBrush);    
//	BOOL bRes = pDC->PatBlt(0, 0, rect.Width(), rect.Height(), PATCOPY);    
//	pDC->SelectObject(pOld);    // restore old brush    
//	return bRes;
//	*/
//
//	return CTabCtrl::OnEraseBkgnd(pDC);
//}

int CMyTabCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CTabCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  Add your specialized creation code here
	this->ModifyStyle(0, TCS_OWNERDRAWFIXED);

	return 0;
}

//void CMyTabCtrl::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
//{
//	// TODO: Add your message handler code here and/or call default
//
//	//CTabCtrl::OnDrawItem(nIDCtl, lpDrawItemStruct);
//	int nTabIndex = lpDrawItemStruct->itemID;
//
//	BOOL bSelected = (nTabIndex == GetCurSel());
// 
//    char label[64];
//    TC_ITEM tci;
//    tci.mask = TCIF_TEXT|TCIF_IMAGE;
//    tci.pszText = label;     
//    tci.cchTextMax = 63;               
//	if (!GetItem(nTabIndex, &tci )) return;
//
//	CRect rect = lpDrawItemStruct->rcItem;
//	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
//	int nSavedDC = pDC->SaveDC();
//
//	pDC->SetBkMode(TRANSPARENT);
//	pDC->FillSolidRect(rect, GetSysColor(COLOR_BTNFACE));
//	//pDC->DrawFrameControl(rect, 
//	 // Draw image
//    CImageList* pImageList = GetImageList();
//    if (pImageList && tci.iImage >= 0) 
//	{
//             rect.left += pDC->GetTextExtent(_T(" ")).cx;            // Margin
// 
//             // Get height of image so we 
//             IMAGEINFO info;
//             pImageList->GetImageInfo(tci.iImage, &info);
//             CRect ImageRect(info.rcImage);
//             int nYpos = rect.top;
// 
//             pImageList->Draw(pDC, tci.iImage, CPoint(rect.left, nYpos), ILD_TRANSPARENT);
//             rect.left += ImageRect.Width();
//     }
// 
//	/*
//     if (bSelected) 
//	 {
//             pDC->SetTextColor(m_crSelColour);
//             pDC->SelectObject(&m_SelFont);
//             rect.top -= ::GetSystemMetrics(SM_CYEDGE);
//             pDC->DrawText(label, rect, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
//	 } 
//	 else 
//	 {
//             pDC->SetTextColor(m_crUnselColour);
//             pDC->SelectObject(&m_UnselFont);
//             pDC->DrawText(label, rect, DT_SINGLELINE|DT_BOTTOM|DT_CENTER);
//     }
//	 */
//  
//	 pDC->RestoreDC(nSavedDC);
//}

void CMyTabCtrl::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class
	CTabCtrl::PreSubclassWindow();
	ModifyStyle(0, TCS_OWNERDRAWFIXED);
}

void CMyTabCtrl::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: Add your message handler code here
	// Do not call CTabCtrl::OnPaint() for painting messages

	CRect rect;
	GetClientRect(&rect);
	rect.top += ::GetSystemMetrics(SM_CYEDGE);
	//CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	//int nSavedDC = pDC->SaveDC();

	CFont *myfont = this->GetFont();

	//dc.SetBkMode(TRANSPARENT);
	//dc.FillSolidRect(rect, GetSysColor(COLOR_BTNFACE));
	
	//CBrush br(RGB(255,0,0));
	//dc.SelectObject(&br);
	dc.SetBkMode(TRANSPARENT);
	//dc.FillRect(rect, &br);
	dc.FillSolidRect(rect, GetSysColor(COLOR_BTNFACE));

	dc.SelectObject(myfont);
	int i;
	char szTabText[100];
	TCITEM tabItem;
	memset(szTabText, '\0', sizeof(szTabText));
	CString itemstr;
	for(i=0;i<GetItemCount();i++)
	{
		GetItemRect(i,&rect);
		rect.top += ::GetSystemMetrics(SM_CYEDGE);

		memset(szTabText, '\0', sizeof(szTabText));
		tabItem.mask = TCIF_TEXT;
		tabItem.pszText = szTabText;
		tabItem.cchTextMax = sizeof(szTabText) - 1;
		this->GetItem(i, &tabItem);
		itemstr = tabItem.pszText;
		/*
		if(tabItem.mask == TCIF_TEXT)
		{
			itemstr.Format("%d", i);
			dc.DrawText(itemstr, rect, DT_SINGLELINE|DT_BOTTOM|DT_CENTER);
		}
		*/
		if(GetCurSel() == i)
		{
			dc.SetTextColor(m_crSelColour);
			dc.SelectObject(&m_SelFont);
			rect.top -= ::GetSystemMetrics(SM_CYEDGE);
			dc.DrawText(itemstr, rect, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
		}
		else
		{
			dc.SetTextColor(m_crUnselColour);
			dc.SelectObject(&m_UnselFont);
			dc.DrawText(itemstr, rect, DT_SINGLELINE|DT_BOTTOM|DT_CENTER);
		}
	}

	//dc.DrawFrameControl(rect, 
	//pDC->SetBkMode(TRANSPARENT);
	//pDC->FillSolidRect(rect, GetSysColor(COLOR_BTNFACE));
}

void CMyTabCtrl::SetColours(COLORREF bSelColour, COLORREF bUnselColour)
{
	m_crSelColour = bSelColour;
	m_crUnselColour = bUnselColour;
	Invalidate();
}

void CMyTabCtrl::SetFonts(CFont* pSelFont, CFont* pUnselFont)
{
	ASSERT(pSelFont && pUnselFont);

	LOGFONT lFont;
	int nSelHeight, nUnselHeight;

	m_SelFont.DeleteObject();
	m_UnselFont.DeleteObject();

	pSelFont->GetLogFont(&lFont);
	m_SelFont.CreateFontIndirect(&lFont);
	nSelHeight = lFont.lfHeight;

	pUnselFont->GetLogFont(&lFont);
	m_UnselFont.CreateFontIndirect(&lFont);
	nUnselHeight = lFont.lfHeight;

	SetFont( (nSelHeight > nUnselHeight)? &m_SelFont : &m_UnselFont);
}

void CMyTabCtrl::SetFonts(int nSelWeight,   BOOL bSelItalic,   BOOL bSelUnderline,
                          int nUnselWeight, BOOL bUnselItalic, BOOL bUnselUnderline)
{
	// Free any memory currently used by the fonts.
	m_SelFont.DeleteObject();
	m_UnselFont.DeleteObject();

	// Get the current font
	LOGFONT lFont;
	CFont *pFont = GetFont();
	if (pFont)
		pFont->GetLogFont(&lFont);
	else {
		NONCLIENTMETRICS ncm;
		ncm.cbSize = sizeof(NONCLIENTMETRICS);
		VERIFY(SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
		                            sizeof(NONCLIENTMETRICS), &ncm, 0));
		lFont = ncm.lfMessageFont;
	}

	// Create the "Selected" font
	lFont.lfWeight = nSelWeight;
	lFont.lfItalic = bSelItalic;
	lFont.lfUnderline = bSelUnderline;
	m_SelFont.CreateFontIndirect(&lFont);

	// Create the "Unselected" font
	lFont.lfWeight = nUnselWeight;
	lFont.lfItalic = bUnselItalic;
	lFont.lfUnderline = bUnselUnderline;
	m_UnselFont.CreateFontIndirect(&lFont);

	SetFont( (nSelWeight > nUnselWeight)? &m_SelFont : &m_UnselFont);
}
