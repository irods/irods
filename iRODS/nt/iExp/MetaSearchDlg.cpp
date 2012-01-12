// MetaSearchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Inquisitor.h"
#include "MetaSearchDlg.h"
#include "MainFrm.h"
#include "InquisitorDoc.h"
#include "irodsWinUtils.h"
#include "MetadataDlg.h"
#include <string>


// CMetaSearchDlg dialog

IMPLEMENT_DYNAMIC(CMetaSearchDlg, CDialog)

CMetaSearchDlg::CMetaSearchDlg(CString & TopCollForSEarch, CWnd* pParent /*=NULL*/)
	: CDialog(CMetaSearchDlg::IDD, pParent)
{
	m_topSearchColl = TopCollForSEarch;
	m_winNotInitialized = true;
	m_serachMode = FILENAME_SEARCH;
}

CMetaSearchDlg::~CMetaSearchDlg()
{
}

void CMetaSearchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_METASRH_TOP_COLL, m_editTopColl);
	DDX_Control(pDX, IDC_COMBO_METASRH_ATT1, m_comboAtt1);
	DDX_Control(pDX, IDC_COMBO_METASRH_ATT2, m_comboAtt2);
	DDX_Control(pDX, IDC_COMBO_METASRH_ATT3, m_comboAtt3);
	DDX_Control(pDX, IDC_COMBO_METASRH_ATT4, m_comboAtt4);
	DDX_Control(pDX, IDC_COMBO_METASRH_VAL1, m_comboVal1);
	DDX_Control(pDX, IDC_COMBO_METASRH_VAL2, m_comboVal2);
	DDX_Control(pDX, IDC_COMBO_METASRH_VAL3, m_comboVal3);
	DDX_Control(pDX, IDC_COMBO_METASRH_VAL4, m_comboVal4);
	DDX_Control(pDX, IDC_CHECK_METASRH_RECURSIVE, m_checkRecursiveSearch);
	DDX_Control(pDX, IDC_LIST_METASEARCH_RESULTS, m_listSearchResults);
	DDX_Control(pDX, IDC_EDIT_METASRH_VAL1, m_editVal1);
	DDX_Control(pDX, IDC_EDIT_METASRH_VAL2, m_editVal2);
	DDX_Control(pDX, IDC_EDIT_METASRH_VAL3, m_editVal3);
	DDX_Control(pDX, IDC_EDIT_METASRH_VAL4, m_editVal4);

	DDX_Control(pDX, IDC_EDIT_METASRH_ATT1, m_editAtt1);
	DDX_Control(pDX, IDC_EDIT_METASRH_ATT2, m_editAtt2);
	DDX_Control(pDX, IDC_EDIT_METASRH_ATT3, m_editAtt3);
	DDX_Control(pDX, IDC_EDIT_METASRH_ATT4, m_editAtt4);
	DDX_Control(pDX, IDOK, m_buttonSearch);
	DDX_Control(pDX, IDCANCEL, m_buttonClose);
	DDX_Control(pDX, IDC_TAB_METASRH, m_tabMetaSearch);
	DDX_Control(pDX, IDC_STATIC_METASRH_F1, m_frame1);
	DDX_Control(pDX, IDC_STATIC_METASRH_F2, m_frame2);
	DDX_Control(pDX, IDC_STATIC_METASRH_F3, m_frame3);
	DDX_Control(pDX, IDC_STATIC_METASRH_F4, m_frame4);
	DDX_Control(pDX, IDC_STATIC_METASRH_ATT2, m_staticAtt2);
	DDX_Control(pDX, IDC_STATIC_METASRH_ATT3, m_staticAtt3);
	DDX_Control(pDX, IDC_STATIC_METASRH_ATT4, m_staticAtt4);
	DDX_Control(pDX, IDC_STATIC_METASRH_ATT1, m_staticAtt1);
	DDX_Control(pDX, IDC_STATIC_METASRH_VAL1, m_staticVal1);
	DDX_Control(pDX, IDC_STATIC_METASRH_VAL2, m_staticVal2);
	DDX_Control(pDX, IDC_STATIC_METASRH_VAL3, m_staticVal3);
	DDX_Control(pDX, IDC_STATIC_METASRH_VAL4, m_staticVal4);
	DDX_Control(pDX, IDC_STATIC_ADVANCED_SRH, m_staticAdvancedSrh);
	DDX_Control(pDX, IDC_EDIT_ADVANCED_SRH, m_editAdvancedSrh);
	DDX_Control(pDX, IDC_STATIC_METASRH_RESULT, m_staticResultCaption);
}


BEGIN_MESSAGE_MAP(CMetaSearchDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CMetaSearchDlg::OnBnClickedOk)
	ON_WM_SIZE()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_METASRH, &CMetaSearchDlg::OnTcnSelchangeTabMetasrh)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_METASEARCHDLG_DOWNLOAD, &CMetaSearchDlg::OnMetasearchdlgDownload)
	ON_COMMAND(ID_METASEARCHDLG_METADATA, &CMetaSearchDlg::OnMetasearchdlgMetadata)
	ON_COMMAND(ID_METASEARCHDLG_COPYTOCLIPBOARD, &CMetaSearchDlg::OnMetasearchdlgCopytoclipboard)
	ON_COMMAND(ID_METASEARCHDLG_DELETE, &CMetaSearchDlg::OnMetasearchdlgDelete)
	ON_BN_CLICKED(IDCANCEL, &CMetaSearchDlg::OnBnClickedCancel)
	ON_LBN_DBLCLK(IDC_LIST_METASEARCH_RESULTS, &CMetaSearchDlg::OnLbnDblclkListMetasearchResults)
//	ON_WM_DRAWITEM()
//ON_WM_CTLCOLOR()
//ON_WM_DRAWITEM()
//ON_WM_DRAWITEM()
//ON_WM_ERASEBKGND()
ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// CMetaSearchDlg message handlers

void CMetaSearchDlg::SetOneOpCombo(CComboBox *cb)
{
	CRect rcCombo;
    CRect rcDropped;
	int h;

	h = cb->GetItemHeight(-1);
	cb->GetClientRect(&rcCombo);
	cb->GetDroppedControlRect(&rcDropped);
	cb->GetParent()->ScreenToClient(&rcDropped);
	rcDropped.bottom = rcDropped.top + rcCombo.Height() + 7*h;
	cb->MoveWindow(&rcDropped);
}

BOOL CMetaSearchDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here

	// This is for XP and up. force dialog to have same white background as the tab
	// need the linking lib, UxTheme.lib.
	//EnableThemeDialogTexture(m_hWnd, ETDT_ENABLETAB);

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc *pDoc = (CInquisitorDoc *)frame->GetActiveDocument();
	this->m_editTopColl.SetWindowText(pDoc->cur_selelected_coll_in_tree->fullPath);

	//std::string srh_ops[] = {">", "<", "<=", ">=", "=", "contains", "like"};
	std::string srh_ops[] = {"=", "contains", "like"};
	int num_srh_ops = 3;
	CString sel_str = "contains";

	combo_atts[0] = &m_comboAtt1;
	combo_atts[1] = &m_comboAtt2;
	combo_atts[2] = &m_comboAtt3;
	combo_atts[3] = &m_comboAtt4;

	combo_vals[0] = &m_comboVal1;
	combo_vals[1] = &m_comboVal2;
	combo_vals[2] = &m_comboVal3;
	combo_vals[3] = &m_comboVal4;

	m_static_atts[0] = &m_staticAtt1;
	m_static_atts[1] = &m_staticAtt2;
	m_static_atts[2] = &m_staticAtt3;
	m_static_atts[3] = &m_staticAtt4;

	m_static_vals[0] = &m_staticVal1;
	m_static_vals[1] = &m_staticVal2;
	m_static_vals[2] = &m_staticVal3;
	m_static_vals[3] = &m_staticVal4;

	m_static_frames[0] = &m_frame1;
	m_static_frames[1] = &m_frame2;
	m_static_frames[2] = &m_frame3;
	m_static_frames[3] = &m_frame4;

	int i, j;

	for(i=0;i<4;i++)
	{
		SetOneOpCombo(combo_atts[i]);
		SetOneOpCombo(combo_vals[i]);
	}

	for(i=0;i<4;i++)
	{
		for(j=0;j<num_srh_ops;j++)
		{
			combo_atts[i]->AddString((char *)srh_ops[j].c_str());
			combo_vals[i]->AddString((char *)srh_ops[j].c_str());
		}
	}

	for(i=0;i<4;i++)
	{
		combo_atts[i]->SelectString(0, sel_str);
		combo_vals[i]->SelectString(0, sel_str);
	}
	
	edit_atts[0] = &m_editAtt1;
	edit_atts[1] = &m_editAtt2;
	edit_atts[2] = &m_editAtt3;
	edit_atts[3] = &m_editAtt4;

	edit_vals[0] = &m_editVal1;
	edit_vals[1] = &m_editVal2;
	edit_vals[2] = &m_editVal3;
	edit_vals[3] = &m_editVal4;
	
	if(m_topSearchColl.GetLength() > 0)
	{
		m_editTopColl.SetWindowText(m_topSearchColl);
	}
	m_checkRecursiveSearch.SetCheck(BST_CHECKED);

	//get the geometry data of dialog
	RECT lrect;
	GetClientRect(&lrect);
	m_minHeight = lrect.bottom;
	m_minWidth = lrect.right;

	m_listSearchResults.GetWindowRect(&lrect);
	ScreenToClient(&lrect);
	m_roffset = m_minWidth - lrect.right;

	//m_tabMetaSearch.SubclassDlgItem(IDC_TAB_METASRH, this);
	m_tabMetaSearch.InsertItem(0, "Filename");
	m_tabMetaSearch.InsertItem(1, "User Defined Metadata");
	m_tabMetaSearch.InsertItem(2, "Advanced");
	m_tabMetaSearch.Invalidate();
	
	SetControls4SearchMode();
	m_winNotInitialized = false;

	//dg_bkcolor = this->GetDC()->GetBkColor();
	//tabBackBrush = new CBrush(dg_bkcolor);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CMetaSearchDlg::SetControls4SearchMode()
{
	int i;
	int TabSearchMode = m_tabMetaSearch.GetCurSel();
	if(TabSearchMode == 0)
		m_serachMode = FILENAME_SEARCH;
	else if(TabSearchMode == 1)
		m_serachMode = USERMETA_SEARCH;
	else 
		m_serachMode = ADVANCED_SEARCH;

	switch(m_serachMode)
	{
	case FILENAME_SEARCH:
		for(i=0;i<4;i++)
		{
			m_static_frames[i]->ShowWindow(SW_HIDE);
			m_static_vals[i]->ShowWindow(SW_HIDE);
			combo_vals[i]->ShowWindow(SW_HIDE);
			edit_vals[i]->ShowWindow(SW_HIDE);
		}

		for(i=1;i<4;i++)
		{
			m_static_atts[i]->ShowWindow(SW_HIDE);
			combo_atts[i]->ShowWindow(SW_HIDE);
			edit_atts[i]->ShowWindow(SW_HIDE);
		}
		m_static_atts[0]->ShowWindow(SW_SHOW);
		combo_atts[0]->ShowWindow(SW_SHOW);
		edit_atts[0]->ShowWindow(SW_SHOW);

		m_staticAtt1.SetWindowText("File Name:");

		m_staticAdvancedSrh.ShowWindow(SW_HIDE);
		m_editAdvancedSrh.ShowWindow(SW_HIDE);

		m_staticResultCaption.SetWindowText("Search Results for collection:");
		m_editTopColl.ShowWindow(SW_SHOW);
		m_checkRecursiveSearch.ShowWindow(SW_SHOW);
		
		break;
	case USERMETA_SEARCH:
		for(i=0;i<4;i++)
		{
			m_static_frames[i]->ShowWindow(SW_SHOW);
			m_static_vals[i]->ShowWindow(SW_SHOW);
			combo_vals[i]->ShowWindow(SW_SHOW);
			edit_vals[i]->ShowWindow(SW_SHOW);

			m_static_frames[i]->BringWindowToTop();
		}

		for(i=0;i<4;i++)
		{
			m_static_atts[i]->ShowWindow(SW_SHOW);
			combo_atts[i]->ShowWindow(SW_SHOW);
			edit_atts[i]->ShowWindow(SW_SHOW);
		}


		m_staticAtt1.SetWindowText("Attribute Name:");

		m_staticAdvancedSrh.ShowWindow(SW_HIDE);
		m_editAdvancedSrh.ShowWindow(SW_HIDE);

		m_staticResultCaption.SetWindowText("Search Results for collection:");
		m_editTopColl.ShowWindow(SW_SHOW);
		m_checkRecursiveSearch.ShowWindow(SW_SHOW);

		break;

	case ADVANCED_SEARCH:

		m_staticAdvancedSrh.ShowWindow(SW_SHOW);
		m_editAdvancedSrh.ShowWindow(SW_SHOW);

		for(i=0;i<4;i++)
		{
			m_static_frames[i]->ShowWindow(SW_HIDE);
			m_static_vals[i]->ShowWindow(SW_HIDE);
			combo_vals[i]->ShowWindow(SW_HIDE);
			edit_vals[i]->ShowWindow(SW_HIDE);
		}

		for(i=0;i<4;i++)
		{
			m_static_atts[i]->ShowWindow(SW_HIDE);
			combo_atts[i]->ShowWindow(SW_HIDE);
			edit_atts[i]->ShowWindow(SW_HIDE);
		}

		m_staticAdvancedSrh.ShowWindow(SW_SHOW);
		m_staticAdvancedSrh.BringWindowToTop();
		m_editAdvancedSrh.ShowWindow(SW_SHOW);
		m_editAdvancedSrh.BringWindowToTop();

		m_staticResultCaption.SetWindowText("Search Results:");
		m_editTopColl.ShowWindow(SW_HIDE);
		m_checkRecursiveSearch.ShowWindow(SW_HIDE);

		break;
	}
}

void CMetaSearchDlg::OnBnClickedOk()
{
	//int MySearchMode = m_tabMetaSearch.GetCurSel();
	if(m_serachMode == FILENAME_SEARCH)  // FILENAME_SEARCH
	{
		SearchButtonOnFilename();
	}
	else if(m_serachMode == USERMETA_SEARCH) // USERMETA_SEARCH
	{
		SearchButtonOnUserMetadata();
	}
	else if(m_serachMode == ADVANCED_SEARCH)
	{
		SearchButtonOnAdvanced();
	}
}

void CMetaSearchDlg::SearchButtonOnAdvanced()
{
	CString quest_where_str;
	m_editAdvancedSrh.GetWindowText(quest_where_str);
	if(quest_where_str.GetLength() <= 0)
	{
		return;
	}

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc *pDoc = (CInquisitorDoc *)frame->GetActiveDocument();

	m_listSearchResults.ResetContent();
	m_ret_par_colls.RemoveAll();
	m_ret_data_names.RemoveAll();

	// BBBB
	CString msgHead;
	BeginWaitCursor();
	int t = irodsWinQueryFilenameAdvanced(pDoc->conn, quest_where_str, pDoc->cur_selected_node_zone, 0, m_ret_par_colls, m_ret_data_names);
	if(t < 0)
	{
		msgHead = CString("Advanced Filename Search error ");
		pDoc->disp_err_msgbox(msgHead, t, this);
		EndWaitCursor();
		return;
	}

	if((t == 0)&&(m_ret_par_colls.GetSize() <= 0))
	{
		::MessageBox(this->m_hWnd, "No result was found.", "Dialog", MB_OK);
		EndWaitCursor();
		return;
	}

	int i;
	CString tmpstr;
	for(i=0;i<m_ret_par_colls.GetSize();i++)
	{
		tmpstr = m_ret_par_colls[i] + CString("/") + m_ret_data_names[i];
		m_listSearchResults.InsertString(-1, tmpstr);
	}

	EndWaitCursor();
}

void CMetaSearchDlg::SearchButtonOnFilename()
{
	CString query_str;
	m_editAtt1.GetWindowText(query_str);
	if(query_str.GetLength() <= 0)
	{
		::MessageBox(this->m_hWnd, "The query input is invalid!", "Dialog", MB_OK);
		return;
	}

	CString op_str;
	m_comboAtt1.GetWindowText(op_str);

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc *pDoc = (CInquisitorDoc *)frame->GetActiveDocument();

	CString tmpstr;
	m_editTopColl.GetWindowTextA(tmpstr);
	if(tmpstr.GetLength() == 0)
	{
		::MessageBox(this->m_hWnd, "Please enter a collection name to start a search.", "Dialog", MB_OK);
		return;
	}
	if(tmpstr.Compare(m_topSearchColl) != 0)
		m_topSearchColl = tmpstr;

	BeginWaitCursor();

	m_listSearchResults.ResetContent();
	m_ret_par_colls.RemoveAll();
	m_ret_data_names.RemoveAll();

	int recursive_srh = 0;
	if(m_checkRecursiveSearch.GetCheck() == BST_CHECKED)
		recursive_srh = 1;

	int t = irodsWinQueryFilename(pDoc->conn, m_topSearchColl, recursive_srh, 
		        op_str, query_str,
				m_ret_par_colls, m_ret_data_names);

	CString msgHead;
	if(t < 0)
	{
		if(m_topSearchColl.GetLength() > 0)
			msgHead = CString("File Name Query error for ") + m_topSearchColl;
		else
			msgHead = CString("Metadata Query error ");
		pDoc->disp_err_msgbox(msgHead, t, this);
		EndWaitCursor();
		return;
	}

	if(t == 0)
	{
		::MessageBox(this->m_hWnd, "No result was found.", "Dialog", MB_OK);
		EndWaitCursor();
	}

	int i;
	for(i=0;i<m_ret_par_colls.GetSize();i++)
	{
		tmpstr = m_ret_par_colls[i] + CString("/") + m_ret_data_names[i];
		m_listSearchResults.InsertString(-1, tmpstr);
	}

	EndWaitCursor();
}

void CMetaSearchDlg::SearchButtonOnUserMetadata()
{
	// TODO: Add your control notification handler code here
	CString tmpstr;
	m_editTopColl.GetWindowText(tmpstr);
	if(tmpstr.GetLength() == 0)
	{
		::MessageBox(this->m_hWnd, "Please enter a collection name to start a search.", "Dialog", MB_OK);
		return;
	}
	if(tmpstr.Compare(m_topSearchColl) != 0)
		m_topSearchColl = tmpstr;

	CArray<CString, CString> att_qname_ops;
	CArray<CString, CString> att_qnames;
	CArray<CString, CString> att_qval_ops;
	CArray<CString, CString> att_qvals;

	CString qatt, qval, att_op, val_op;
	int i;
	for(i=0;i<4;i++)
	{
		edit_atts[i]->GetWindowTextA(qatt);
		edit_vals[i]->GetWindowText(qval);
		if((qatt.GetLength() > 0) && (qval.GetLength() > 0))
		{
			combo_atts[i]->GetWindowTextA(att_op);
			combo_vals[i]->GetWindowTextA(val_op);

			att_qname_ops.Add(att_op);
			att_qnames.Add(qatt);
			att_qval_ops.Add(val_op);
			att_qvals.Add(qval);

		}
	}

	if(att_qname_ops.GetSize() <= 0)
	{
		::MessageBox(this->m_hWnd, "The query input(s) is/are invalid!", "Dialog", MB_OK);
		return;
	}

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc *pDoc = (CInquisitorDoc *)frame->GetActiveDocument();

	BeginWaitCursor();

	m_listSearchResults.ResetContent();
	m_ret_par_colls.RemoveAll();
	m_ret_data_names.RemoveAll();

	int recursive_srh = 0;
	if(m_checkRecursiveSearch.GetCheck() == BST_CHECKED)
		recursive_srh = 1;

	int t = irodsWinQueryMetadata(pDoc->conn, m_topSearchColl, recursive_srh,
				att_qname_ops, att_qnames, att_qval_ops, att_qvals,
				m_ret_par_colls, m_ret_data_names);
	
	CString msgHead;
	if(t < 0)
	{
		if(m_topSearchColl.GetLength() > 0)
			msgHead = CString("Metadata Query error for ") + m_topSearchColl;
		else
			msgHead = CString("Metadata Query error ");
		pDoc->disp_err_msgbox(msgHead, t, this);
		EndWaitCursor();
		return;
	}

	if(t == 0)
	{
		::MessageBox(this->m_hWnd, "No result was found.", "Dialog", MB_OK);
		EndWaitCursor();
	}

	for(i=0;i<m_ret_par_colls.GetSize();i++)
	{
		tmpstr = m_ret_par_colls[i] + CString("/") + m_ret_data_names[i];
		m_listSearchResults.InsertString(-1, tmpstr);
	}

	EndWaitCursor();
}

void CMetaSearchDlg::ResizeWidth(CWnd *myWin)
{
	RECT wrect;
	GetClientRect(&wrect);

	RECT lrect;
	myWin->GetWindowRect(&lrect);
	ScreenToClient(&lrect);
	int w = wrect.right - m_roffset - lrect.left;
	myWin->MoveWindow(lrect.left, lrect.top, w, lrect.bottom-lrect.top);
}

void CMetaSearchDlg::MoveButton(CWnd *myWin)
{
	RECT wrect;
	GetClientRect(&wrect);

	RECT lrect;
	myWin->GetWindowRect(&lrect);
	ScreenToClient(&lrect);
	int w = lrect.right - lrect.left; //wrect.right - m_roffset - lrect.left;
	int x = wrect.right - m_roffset - w;
	myWin->MoveWindow(x, lrect.top, w, lrect.bottom-lrect.top);
}

void CMetaSearchDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if(m_winNotInitialized)
		return;

	RECT wrect;
	GetClientRect(&wrect);

	bool change_height = false;
	if(wrect.bottom > m_minHeight)
		change_height = true;
	bool change_width = false;
	if(wrect.right > m_minWidth)
		change_width = true;

	RECT lrect;
	int w;
	//m_listSearchResults.GetWindowRect(&lrect);
	//ScreenToClient(&lrect);
	if(change_width)
	{
		MoveButton((CWnd *)&m_buttonSearch);
		MoveButton((CWnd *)&m_buttonClose);
		MoveButton((CWnd *)&m_checkRecursiveSearch);
		ResizeWidth((CWnd *)&m_editTopColl);
	}

	if((change_width)||(change_height))
	{
		RECT lrect;
		m_listSearchResults.GetWindowRect(&lrect);
		ScreenToClient(&lrect);
		int w = wrect.right - m_roffset - lrect.left;
		int h = wrect.bottom - m_roffset - lrect.top;
		m_listSearchResults.MoveWindow(lrect.left, lrect.top, w, h);
	}

	// TODO: Add your message handler code here
}

void CMetaSearchDlg::OnTcnSelchangeTabMetasrh(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: Add your control notification handler code here
	*pResult = 0;
	SetControls4SearchMode();
}

void CMetaSearchDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
	// TODO: Add your message handler code here
	CWnd *theWin = (CWnd *)&m_listSearchResults;
	if(theWin == pWnd)
	{
		int count = m_listSearchResults.GetSelCount();
		if(count == 0)
			return;

		CMenu menu;
		POINT pt;

		GetCursorPos(&pt);
		if(count == 1)
		{
			menu.CreatePopupMenu();
			menu.AppendMenu(MF_ENABLED, ID_METASEARCHDLG_DOWNLOAD, "Download ...");
			menu.AppendMenu(MF_ENABLED, ID_METASEARCHDLG_METADATA, "Metadata ...");
			menu.AppendMenu(MF_ENABLED, ID_METASEARCHDLG_COPYTOCLIPBOARD, "Copy iRODS Path to Clipboard");
			menu.AppendMenu(MF_ENABLED, ID_METASEARCHDLG_DELETE, "Delete");
			menu.TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y,this);
		}
		else if(count > 1)
		{
			menu.CreatePopupMenu();
			menu.AppendMenu(MF_ENABLED, ID_METASEARCHDLG_DOWNLOAD, "Download ...");
			menu.AppendMenu(MF_ENABLED, ID_METASEARCHDLG_COPYTOCLIPBOARD, "Copy iRODS Path to Clipboard");
			menu.AppendMenu(MF_ENABLED, ID_METASEARCHDLG_DELETE, "Delete");
			menu.TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y,this);
		}
	}
}

void CMetaSearchDlg::OnMetasearchdlgDownload()
{
	// TODO: Add your command handler code here
	if(m_listSearchResults.GetSelCount() < 0)
		return;

	// get a local folder
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc *pDoc = (CInquisitorDoc *)frame->GetActiveDocument();

	CString selectedLocalDir = "";
	pDoc->SelectLocalFolder(selectedLocalDir);
	if(selectedLocalDir.GetLength() == 0)
		return;
	
	int forceful_flag = 1;
	int recursive_flag = 1;
	this->BeginWaitCursor();

	// AAAA
	int n = m_listSearchResults.GetSelCount();
	CArray<int,int> listbox_sels;
	listbox_sels.SetSize(n);
	m_listSearchResults.GetSelItems(n, listbox_sels.GetData());
	int i, j, t;
	CString irod_obj_fullpath;
	for(i=0;i<n;i++)
	{
		j = listbox_sels[n-1-i];
		irod_obj_fullpath = m_ret_par_colls[j] + CString("/") + m_ret_data_names[j];
		t = irodsWinDownloadOneObject(pDoc->conn, irod_obj_fullpath, selectedLocalDir, 
									recursive_flag, forceful_flag);
		if(t < 0)
		{
			CString msgHead = CString("Downlod Operation error ");
			pDoc->disp_err_msgbox(msgHead, t, this);
			this->EndWaitCursor();
			return;
		}
	}
	::MessageBox(this->m_hWnd, "The selected file(s) has/have been downloaded.", "Dialog", MB_OK);
	
	this->EndWaitCursor();
}

void CMetaSearchDlg::OnMetasearchdlgMetadata()
{
	// TODO: Add your command handler code here
	int t = m_listSearchResults.GetSelCount();
	if(t != 1)
		return;
	t = m_listSearchResults.GetCurSel();

	CString obj_name_wpath = m_ret_par_colls[t] + CString("/") + m_ret_data_names[t];
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	frame->CreateMetadataDlg(obj_name_wpath, irodsWinObj::IRODS_DATAOBJ_TYPE);
}

void CMetaSearchDlg::OnMetasearchdlgCopytoclipboard()
{
	// TODO: Add your command handler code here
	int n = m_listSearchResults.GetSelCount();
	CArray<int,int> listbox_sels;
	listbox_sels.SetSize(n);
	m_listSearchResults.GetSelItems(n, listbox_sels.GetData());

	CString path_to_copy = "";
	CString irod_obj_fullpath;
	int i,j;
	for(i=0;i<n;i++)
	{
		j = listbox_sels[i];
		irod_obj_fullpath = m_ret_par_colls[j] + CString("/") + m_ret_data_names[j];
		if(i < (n-1))
		{
			path_to_copy += irod_obj_fullpath + CString("\r\n");
		}
		else
		{
			path_to_copy += irod_obj_fullpath;
		}
	}

	if(!OpenClipboard())
	{
		::MessageBox(this->m_hWnd, "Cannot open the Clipboard", "Dialog", MB_OK);
		return;
	}

	if( !EmptyClipboard())
	{
		::MessageBox(this->m_hWnd, "Cannot empty the Clipboard", "Dialog", MB_OK);
		return;
	}

	HANDLE hClipboardData = GlobalAlloc(GPTR, path_to_copy.GetLength()+1);
	LPTSTR lpszBuffer = (LPTSTR)GlobalLock(hClipboardData);
	_tcscpy(lpszBuffer, path_to_copy);
	GlobalUnlock(hClipboardData);
	SetClipboardData(CF_TEXT, hClipboardData);
	CloseClipboard();
}

void CMetaSearchDlg::OnMetasearchdlgDelete()
{
	// TODO: Add your command handler code here
	if(m_listSearchResults.GetSelCount() < 0)
		return;

	// get a local folder
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc *pDoc = (CInquisitorDoc *)frame->GetActiveDocument();

	if(::MessageBox(this->m_hWnd, "This will delete selected dataset(s) and all related replica(s). Are you sure?", "Dialog", MB_YESNO) != IDYES)
		return;

	
	int repnum = -1;
	this->BeginWaitCursor();

	int n = m_listSearchResults.GetSelCount();
	CArray<int,int> listbox_sels;
	listbox_sels.SetSize(n);
	m_listSearchResults.GetSelItems(n, listbox_sels.GetData());
	int i, j, t;
	CString irod_obj_fullpath;
	for(i=0;i<n;i++)
	{
		j = listbox_sels[n-1-i];
		irod_obj_fullpath = m_ret_par_colls[j] + CString("/") + m_ret_data_names[j];
		t = irodsWinDeleteOneDataobj(pDoc->conn, m_ret_data_names[j], irod_obj_fullpath, 
					repnum, pDoc->delete_obj_permanently);
		if(t < 0)
		{
			CString msgHead = CString("Delete Operation error ");
			pDoc->disp_err_msgbox(msgHead, t, this);
			this->EndWaitCursor();
			return;
		}
		m_ret_par_colls.RemoveAt(j);
		m_ret_data_names.RemoveAt(j);
		m_listSearchResults.DeleteString(j);
	}
	::MessageBox(this->m_hWnd, "The selected file(s) has/have been deleted.", "Dialog", MB_OK);
	
	this->EndWaitCursor();
}

void CMetaSearchDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	//OnCancel();
	this->ShowWindow(SW_HIDE);
}

void CMetaSearchDlg::SetTopCollection(CString & TopColl)
{
	// check if the original search results are there
	CString tmpstr;
	m_editTopColl.GetWindowTextA(tmpstr);

	if(TopColl.Compare(tmpstr) != 0)
	{
		m_listSearchResults.ResetContent();
		m_ret_par_colls.RemoveAll();
		m_ret_data_names.RemoveAll();
		
		m_topSearchColl = TopColl;
		m_editTopColl.SetWindowText(m_topSearchColl);
	}
}

void CMetaSearchDlg::OnLbnDblclkListMetasearchResults()
{
	// TODO: Add your control notification handler code here
	if(m_listSearchResults.GetSelCount() != 1)
		return;

	int i = m_listSearchResults.GetCurSel();
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc *pDoc = (CInquisitorDoc *)frame->GetActiveDocument();
	pDoc->disp_file_in_local_machine(m_ret_par_colls[i], m_ret_data_names[i], this);
}

bool CMetaSearchDlg::IsWhiteBkGrdControl(CWnd* pWnd)
{
	if((pWnd->m_hWnd == m_staticAtt1.m_hWnd) || (pWnd->m_hWnd == m_staticAtt2.m_hWnd) ||
	    (pWnd->m_hWnd == m_staticAtt3.m_hWnd) || (pWnd->m_hWnd == m_staticAtt4.m_hWnd))
		return true;

	if((pWnd->m_hWnd == m_staticVal1.m_hWnd) || (pWnd->m_hWnd == m_staticVal2.m_hWnd) ||
	    (pWnd->m_hWnd == m_staticVal3.m_hWnd) || (pWnd->m_hWnd == m_staticVal4.m_hWnd))
		return true;

	return false;
}

HBRUSH CMetaSearchDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	//pDC->SetBkMode(TRANSPARENT);

	CBrush m_brBgColor;   
    m_brBgColor.CreateSolidBrush(RGB(255, 255, 255));   

	// TODO:  Change any attributes of the DC here
	if(IsWhiteBkGrdControl(pWnd))
	{
		pDC->SetBkColor(RGB(255,255,255));
		pDC->SetTextColor(RGB(0,0,0));
		hbr = m_brBgColor;
	}

	// TODO:  Return a different brush if the default is not desired
	return hbr;
}

void CMetaSearchDlg::OnCancel()
{
	// TODO: Add your specialized code here and/or call the base class
	int i;
	for(i=0;i<4;i++)
	{
		m_static_frames[i] = NULL;
		m_static_atts[i] = NULL;
		m_static_vals[i] = NULL;
	}
	for(i=0;i<4;i++)
	{
		edit_atts[i] = NULL;
		edit_vals[i] = NULL;
	}
	CDialog::OnCancel();
}
