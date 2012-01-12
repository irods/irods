#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "MyTabCtrl.h"

#define FILENAME_SEARCH 1
#define USERMETA_SEARCH 2
#define ADVANCED_SEARCH 3

// CMetaSearchDlg dialog

class CMetaSearchDlg : public CDialog
{
	DECLARE_DYNAMIC(CMetaSearchDlg)

public:
	CMetaSearchDlg(CString & TopCollForSEarch, CWnd* pParent = NULL);   // standard constructor
	virtual ~CMetaSearchDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG_META_SEARCH };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
private:
	CEdit m_editTopColl;
	CComboBox m_comboAtt1;
	CComboBox m_comboAtt2;
	CComboBox m_comboAtt3;
	CComboBox m_comboAtt4;
	CComboBox m_comboVal1;
	CComboBox m_comboVal2;
	CComboBox m_comboVal3;
	CComboBox m_comboVal4;
	CButton m_checkRecursiveSearch;
	CListBox m_listSearchResults;

	CComboBox *combo_atts[4];
	CComboBox *combo_vals[4];

	CString m_topSearchColl;

	bool IsWhiteBkGrdControl(CWnd* pWnd);

	//COLORREF dg_bkcolor;
	//CBrush *tabBackBrush;

public:
	afx_msg void OnBnClickedOk();
private:
	CEdit m_editVal1;
	CEdit m_editVal2;
	CEdit m_editVal3;
	CEdit m_editVal4;
	
	CEdit m_editAtt1;
	CEdit m_editAtt2;
	CEdit m_editAtt3;
	CEdit m_editAtt4;

	CEdit *edit_atts[4];
	CEdit *edit_vals[4];

	CButton m_buttonSearch;
	CButton m_buttonClose;

	// for resizing
	int m_minHeight, m_minWidth;
	int m_roffset;

	bool m_winNotInitialized;

	void SetOneOpCombo(CComboBox *cb);
	void ResizeWidth(CWnd *myWin);
	void MoveButton(CWnd *myWin);
	void SetControls4SearchMode();
	void SearchButtonOnUserMetadata();
	void SearchButtonOnFilename();
	void SearchButtonOnAdvanced();

	int m_serachMode;

public:
	afx_msg void OnSize(UINT nType, int cx, int cy);

private:
	CTabCtrl m_tabMetaSearch;
	//CMyTabCtrl m_tabMetaSearch;
	CStatic m_frame1;
	CStatic m_frame2;
	CStatic m_frame3;
	CStatic m_frame4;

	CStatic m_staticAtt2;
	CStatic m_staticAtt3;
	CStatic m_staticAtt4;
	CStatic m_staticAtt1;
	CStatic m_staticVal1;
	CStatic m_staticVal2;
	CStatic m_staticVal3;
	CStatic m_staticVal4;

	CStatic *m_static_frames[4];
	CStatic *m_static_atts[4];
	CStatic *m_static_vals[4];

	// serach result in the list
	CArray<CString, CString> m_ret_par_colls, m_ret_data_names;

public:
	afx_msg void OnTcnSelchangeTabMetasrh(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnMetasearchdlgDownload();
	afx_msg void OnMetasearchdlgMetadata();
	afx_msg void OnMetasearchdlgCopytoclipboard();
	afx_msg void OnMetasearchdlgDelete();
	afx_msg void OnBnClickedCancel();

	void SetTopCollection(CString & TopColl);
	afx_msg void OnLbnDblclkListMetasearchResults();
//	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
//	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
//	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
//	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
private:
	CStatic m_staticAdvancedSrh;
	CEdit m_editAdvancedSrh;
	CStatic m_staticResultCaption;

public:
	virtual void OnCancel();
};
