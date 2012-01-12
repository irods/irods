#pragma once


// CMyTabCtrl

class CMyTabCtrl : public CTabCtrl
{
	DECLARE_DYNAMIC(CMyTabCtrl)

public:
	CMyTabCtrl();
	virtual ~CMyTabCtrl();

protected:
	DECLARE_MESSAGE_MAP()
public:
//	afx_msg void OnPaint();
//	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
//	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
protected:
	virtual void PreSubclassWindow();

	COLORREF m_crSelColour, m_crUnselColour;
	CFont    m_SelFont,	m_UnselFont;

public:
	afx_msg void OnPaint();

private:
	void SetColours(COLORREF bSelColour, COLORREF bUnselColour);
	void SetFonts(CFont* pSelFont, CFont* pUnselFont);
	void SetFonts(int nSelWeight,   BOOL bSelItalic,   BOOL bSelUnderline,
                          int nUnselWeight, BOOL bUnselItalic, BOOL bUnselUnderline);

};


