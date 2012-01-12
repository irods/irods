// MetadataDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Inquisitor.h"
#include "MetadataDlg.h"
#include "irodsWinObj.h"
#include "irodsWinUtils.h"
#include "MainFrm.h"
#include "InquisitorDoc.h"
#include <algorithm>
#include "DispTextDlg.h"
#include "DialogWebDisp.h"


// CMetadataDlg dialog

IMPLEMENT_DYNAMIC(CMetadataDlg, CDialog)

CMetadataDlg::CMetadataDlg(CString & sObjName, int iObjType, CWnd* pParent /*=NULL*/)
	: CDialog(CMetadataDlg::IDD, pParent)
{
	m_iObjType = iObjType;
	m_sObjNameWithPath = sObjName;
	m_dlgInitialized = false;
}

CMetadataDlg::~CMetadataDlg()
{
}

void CMetadataDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_CD_METADLG, m_editCDNameWpath);
	DDX_Control(pDX, IDC_STATIC_CD_LABEL_METADLG, m_labelCD);
	DDX_Control(pDX, IDC_LIST_METADATA_DLG, m_listMetadata);
	DDX_Control(pDX, IDC_TAB_MEDATA_DLG, m_tabMetadata);
	DDX_Control(pDX, IDC_BUTTON_DELETE_METADATA_DLG, m_buttonMdlgDelete);
	DDX_Control(pDX, IDC_STATIC_MDLG_ATT, m_labelMdlgAtt);
	DDX_Control(pDX, IDC_STATIC_MDLG_VAL, m_labelMdlgVal);
	DDX_Control(pDX, IDC_STATIC_MDLG_UNIT, m_labelMdlgUnit);
	DDX_Control(pDX, IDC_EDIT_MDLG_ATT, m_editMdlgAtt);
	DDX_Control(pDX, IDC_EDIT_MDLG_VAL, m_editMdlgVal);
	DDX_Control(pDX, IDC_EDIT_MDLG_UNIT, m_editMdlgUnit);
	DDX_Control(pDX, IDC_STATIC_METADLG_MSG, m_labelMetadlgMsg);
	DDX_Control(pDX, IDCANCEL, m_buttonCancel);
	DDX_Control(pDX, IDC_BUTTON_VIEWHTML_METADAT_DLG, m_btnViewMetaInHTML);
}


BEGIN_MESSAGE_MAP(CMetadataDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CMetadataDlg::OnBnClickedOk)
	//ON_BN_CLICKED(IDC_BUTTON_ADD_METADLG, &CMetadataDlg::OnBnClickedButtonAddMetadlg)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_METADATA_DLG, &CMetadataDlg::OnBnClickedButtonDeleteMetadataDlg)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_MEDATA_DLG, &CMetadataDlg::OnTcnSelchangeTabMedataDlg)
	//ON_BN_CLICKED(IDC_BUTTON_METADLG_VIEW_LONGVAL, &CMetadataDlg::OnBnClickedButtonMetadlgViewLongval)
	ON_BN_CLICKED(IDCANCEL, &CMetadataDlg::OnBnClickedCancel)
//	ON_WM_WINDOWPOSCHANGED()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BUTTON_VIEWHTML_METADAT_DLG, &CMetadataDlg::OnBnClickedButtonViewhtmlMetadatDlg)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// CMetadataDlg message handlers

void CMetadataDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	//OnOK();
}

BOOL CMetadataDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc *pDoc = (CInquisitorDoc *)frame->GetActiveDocument();

	m_listMetadata.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	this->BeginWaitCursor();

	// TODO:  Add extra initialization here
	switch(m_iObjType)
	{
		case irodsWinObj::IRODS_COLLECTION_TYPE:
			m_labelCD.SetWindowText("Collection:");
			m_editCDNameWpath.SetWindowText(m_sObjNameWithPath);
			break;
		case irodsWinObj::IRODS_DATAOBJ_TYPE:
			m_labelCD.SetWindowText("Dataset:");
			m_editCDNameWpath.SetWindowText(m_sObjNameWithPath);
			break;
		default:
			break;
	}
	
	int t = irodsWinGetObjUserDefinedMetadata(pDoc->conn, m_sObjNameWithPath, m_iObjType,
					m_metaAtts, m_metaVals, m_metaUnits);

	if(t < 0)
	{
		CString msgHead = CString("Get Metadata() error for ") + m_sObjNameWithPath;
		pDoc->disp_err_msgbox(msgHead, t, this);
		this->EndWaitCursor();
	}

	// display 
	m_listMetadata.InsertColumn(METADATA_ATT, "Attribute", LVCFMT_LEFT, 80);
	m_listMetadata.InsertColumn(METADATA_VAL, "Value", LVCFMT_LEFT, 140);
	m_listMetadata.InsertColumn(METADATA_UNIT, "Unit", LVCFMT_LEFT, 60);

	for(int i=0; i<m_metaAtts.GetSize();i++)
	{
		//int pos = list.InsertItem(n, tDataobjToAdd->name, 1);
		int pos = m_listMetadata.InsertItem(i, m_metaAtts[i], 0);
		m_listMetadata.SetItemText(pos, METADATA_ATT, m_metaAtts[i]);
		m_listMetadata.SetItemText(pos, METADATA_VAL, m_metaVals[i]);
		m_listMetadata.SetItemText(pos, METADATA_UNIT, m_metaUnits[i]);
	}

	// create tabs
	m_tabMetadata.InsertItem(0, "Current Metadata");
	m_tabMetadata.InsertItem(1, "Add Metadata");
	m_tabMetadata.InsertItem(2, "Modify Metadata");
	m_tabMetadata.InsertItem(3, "View selected Metadata");

	// handling showing of controls for current tab selection
	m_labelMdlgAtt.ShowWindow(SW_HIDE);
	m_labelMdlgVal.ShowWindow(SW_HIDE);
	m_labelMdlgUnit.ShowWindow(SW_HIDE);
	m_editMdlgAtt.ShowWindow(SW_HIDE);
	m_editMdlgVal.ShowWindow(SW_HIDE);
	m_editMdlgUnit.ShowWindow(SW_HIDE);

	m_operationMode = METADATA_DELETE_OP;

	// get the geometry size of controls
	RECT lrect;
	int dlg_w, dlg_h;
	GetClientRect(&lrect);
	dlg_w = lrect.right;
	dlg_h = lrect.bottom;

	// collection text 
	m_editCDNameWpath.GetWindowRect(&lrect);
	ScreenToClient(&lrect);
	editCDNameWpath_rightoffset = dlg_w - lrect.right;
	editCDNameWpath_leftoffset = lrect.left;
	editCDNameWpath_topoffset = lrect.top;

	// delete button
	m_buttonMdlgDelete.GetWindowRect(&lrect);
	ScreenToClient(&lrect);
	buttonMdlgDelete_rightoffset = dlg_w - lrect.right;
	buttonMdlgDelete_topoffset = lrect.top;
	int button_delete_bottom = lrect.bottom;
	int buton_delete_left = lrect.left;

	// cancel button
	m_buttonCancel.GetWindowRect(&lrect);
	ScreenToClient(&lrect);
	buttonCancel_rightoffset = dlg_w - lrect.right;
	buttonCancel_bottomoffset = dlg_h - lrect.bottom;

	// tab
	m_tabMetadata.GetWindowRect(&lrect);
	ScreenToClient(&lrect);
	tabMetadata_rightoffset = dlg_w - lrect.right;
	tabMetadata_bottomoffset = dlg_h - lrect.bottom;

	// metadata list
	m_listMetadata.GetWindowRect(&lrect);
	ScreenToClient(&lrect);
	listMetadata_rightoffset =  dlg_w - lrect.right;
	listMetadata_bottomoffset = dlg_h - lrect.bottom;

	// status label
	m_labelMetadlgMsg.GetWindowRect(&lrect);
	ScreenToClient(&lrect);
	labelMetadlgMsg_leftoffset = lrect.left;
	labelMetadlgMsg_bottomoffset = dlg_h - lrect.bottom;

	// att edit
	m_editMdlgAtt.GetWindowRect(&lrect);
	ScreenToClient(&lrect);
	editMdlgAtt_rightoffset = dlg_w - lrect.right;
	editMdlgAtt_leftoffset = lrect.left;
	editMdlgAtt_topoffset = lrect.top;

	// att UNIT edit
	m_editMdlgUnit.GetWindowRect(&lrect);
	ScreenToClient(&lrect);
	editMdlgUnit_rightoffset = dlg_w - lrect.right;
	editMdlgUnit_leftoffset = lrect.left;
	int editMdlgUnit_topoffset = lrect.top;
	editMdlgUnit_bottomoffset = dlg_h - lrect.bottom;

	// att value edit
	m_editMdlgVal.GetWindowRect(&lrect);
	ScreenToClient(&lrect);
	editMdlgVal_rightoffset = dlg_w - lrect.right;
	editMdlgVal_leftoffset = lrect.left;
	editMdlgVal_topoffset = lrect.top;
	vgap_between_editVal_editUnit = editMdlgUnit_topoffset - lrect.bottom;

	// get the distance between buttons of "delete" and "view html"
	CButton *viewhtml_btn = (CButton *)this->GetDlgItem(IDC_BUTTON_VIEWHTML_METADAT_DLG);
	int ty;
	if(viewhtml_btn != NULL)
	{
		//GetWindowRect(&lrect);
		viewhtml_btn->GetWindowRect(&lrect);
		ty = lrect.top;
		m_buttonMdlgDelete.GetWindowRect(&lrect);
		offset_delete_viewhtml_buttons = ty - lrect.bottom;
	}

	m_labelMetadlgMsg.SetWindowText(" ");
	m_dlgInitialized = true;

	this->EndWaitCursor();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CMetadataDlg::CleanMetaData()
{
	m_metaAtts.RemoveAll();
	m_metaVals.RemoveAll();
	m_metaUnits.RemoveAll();
}

#if 0
void CMetadataDlg::OnBnClickedButtonAddMetadlg()  // this button is used for both add and modify
{
	// TODO: Add your control notification handler code here
	switch(m_operationMode)
	{
	case 1: // add metadata
		AddMetadata();
		break;
	case 2: // modify metadata
		ModifyMetadata();
		break;
	}
}
#endif

void CMetadataDlg::ModifyMetadata()
{
	CString newAtt, newVal, newUnit;
	m_editMdlgAtt.GetWindowText(newAtt);
	m_editMdlgVal.GetWindowText(newVal);
	m_editMdlgUnit.GetWindowText(newUnit);

	if((newAtt==att_to_modify)&&(newVal==val_to_modify)&&(newUnit==unit_to_modify))
	{
		::MessageBox(this->m_hWnd, "Noting has been modified!", "Modify Metadata", MB_OK);
		return;
	}

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc *pDoc = (CInquisitorDoc *)frame->GetActiveDocument();

	this->BeginWaitCursor();
	// remove the old metadat first and then insert new metadata
	int t = irodsWinDeleteMetadata(pDoc->conn, m_sObjNameWithPath, m_iObjType, 
					att_to_modify, val_to_modify, unit_to_modify);
	if(t < 0)
	{
		this->EndWaitCursor();
		CString msgHead = CString("Modify Metadata() error for ") + m_sObjNameWithPath;
		pDoc->disp_err_msgbox(msgHead, t, this);
		return;
	}

	// add new metadata
	t = irodsWinAddMetadata(pDoc->conn, m_sObjNameWithPath, m_iObjType, 
					newAtt, newVal, newUnit);
	this->EndWaitCursor();
	if(t < 0)
	{
		
		CString msgHead = CString("Modify Metadata() error for ") + m_sObjNameWithPath;
		pDoc->disp_err_msgbox(msgHead, t, this);
		return;
	}
	int p = selected_pos_to_modify;
	m_listMetadata.SetItemText(p, METADATA_ATT, newAtt);
	m_listMetadata.SetItemText(p, METADATA_VAL, newVal);
	m_listMetadata.SetItemText(p, METADATA_UNIT, newUnit);

	m_metaAtts[p] = newAtt;
	m_metaVals[p] = newVal;
	m_metaUnits[p] = newUnit;

	m_labelMetadlgMsg.SetWindowText("The metadata was successfully modified.");
}

void CMetadataDlg::AddMetadata()
{
	CString newMetaAtt, newMetaVal, newMetaUnit;
	m_editMdlgAtt.GetWindowTextA(newMetaAtt);
	m_editMdlgVal.GetWindowTextA(newMetaVal);
	m_editMdlgUnit.GetWindowTextA(newMetaUnit);

	newMetaAtt.Trim();
	newMetaVal.Trim();
	newMetaUnit.Trim();
	if((newMetaAtt.GetLength()==0)||(newMetaVal.GetLength() ==0))
	{
		(void)::MessageBox(this->m_hWnd, "Neither Attribute nor Value can be null!", "No Inout Warning", MB_OK|MB_ICONWARNING);
		return;
	}

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc *pDoc = (CInquisitorDoc *)frame->GetActiveDocument();

	this->BeginWaitCursor();
	int t = irodsWinAddMetadata(pDoc->conn, m_sObjNameWithPath, m_iObjType, 
					newMetaAtt, newMetaVal, newMetaUnit);
	this->EndWaitCursor();

	if(t < 0)
	{
		CString msgHead = CString("Add Metadata() error for ") + m_sObjNameWithPath;
		pDoc->disp_err_msgbox(msgHead, t, this);
		return;
	}

	int n = m_listMetadata.GetItemCount();
	int pos = m_listMetadata.InsertItem(n, newMetaAtt, 0);
	m_listMetadata.SetItemText(pos, METADATA_ATT, newMetaAtt);
	m_listMetadata.SetItemText(pos, METADATA_VAL, newMetaVal);
	m_listMetadata.SetItemText(pos, METADATA_UNIT, newMetaUnit);

	m_metaAtts.Add(newMetaAtt);
	m_metaVals.Add(newMetaVal);
	m_metaUnits.Add(newMetaUnit);

	m_labelMetadlgMsg.SetWindowText("The metadata was successfully created.");
}

void CMetadataDlg::DeleteMetadata()
{
	// TODO: Add your control notification handler code here
	if(m_listMetadata.GetSelectedCount() <= 0)
		return;

	if(::MessageBox(this->m_hWnd, "This will delete the selected metadata. Are you sure?", "Delete Data Confirmation", MB_YESNO|MB_ICONQUESTION) != IDYES)
	{
		return;
	}

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc *pDoc = (CInquisitorDoc *)frame->GetActiveDocument();

	std::vector<int> listbox_sels;
	POSITION POS = m_listMetadata.GetFirstSelectedItemPosition();
	if(POS == NULL)
		return;
	while(POS)
	{
		int t = m_listMetadata.GetNextSelectedItem(POS); 
		listbox_sels.push_back(t);
	}
	sort(listbox_sels.begin(), listbox_sels.end());

	this->BeginWaitCursor();
	int n = listbox_sels.size();
	for(int i=0;i<n;i++)
	{
		int j = listbox_sels[n-i-1];
		int t = irodsWinDeleteMetadata(pDoc->conn, m_sObjNameWithPath, m_iObjType, 
					m_metaAtts[j], m_metaVals[j], m_metaUnits[j]);
		if(t < 0)
		{
			this->EndWaitCursor();
			CString msgHead = CString("Delete Metadata() error for ") + m_sObjNameWithPath;
			pDoc->disp_err_msgbox(msgHead, t, this);
			return;
		}

		//ddelete data
		m_metaAtts.RemoveAt(j);
		m_metaVals.RemoveAt(j);
		m_metaUnits.RemoveAt(j);
		m_listMetadata.DeleteItem(j);
	}
	m_labelMetadlgMsg.SetWindowText("The metadata was succcessfully deleted.");
}

// this button is actually used for 'delete', 'add', and 'modify'.
void CMetadataDlg::OnBnClickedButtonDeleteMetadataDlg()
{
	switch(m_operationMode)
	{
	case METADATA_DELETE_OP:
		DeleteMetadata();
		break;
	case METADATA_ADD_OP:
		AddMetadata();
		break;
	case METADATA_MODIFY_OP:
		ModifyMetadata();
		break;
	}	
}

void CMetadataDlg::TabViewMetadataListAction()
{
	m_listMetadata.ShowWindow(SW_SHOW);
	
	m_labelMdlgAtt.ShowWindow(SW_HIDE);
	m_labelMdlgVal.ShowWindow(SW_HIDE);
	m_labelMdlgUnit.ShowWindow(SW_HIDE);
	m_editMdlgAtt.ShowWindow(SW_HIDE);
	m_editMdlgVal.ShowWindow(SW_HIDE);
	m_editMdlgUnit.ShowWindow(SW_HIDE);
	
	m_listMetadata.SetFocus();
	m_operationMode = METADATA_DELETE_OP;
}

void CMetadataDlg::TabAddModifyViewSelAction()
{
	m_listMetadata.ShowWindow(SW_HIDE);

	m_labelMdlgAtt.ShowWindow(SW_SHOW);
	m_labelMdlgVal.ShowWindow(SW_SHOW);
	m_labelMdlgUnit.ShowWindow(SW_SHOW);
	m_editMdlgAtt.ShowWindow(SW_SHOW);
	m_editMdlgVal.ShowWindow(SW_SHOW);
	m_editMdlgUnit.ShowWindow(SW_SHOW);

	m_labelMdlgAtt.BringWindowToTop();
	m_labelMdlgVal.BringWindowToTop();
	m_labelMdlgUnit.BringWindowToTop();
	m_editMdlgAtt.BringWindowToTop();
	m_editMdlgVal.BringWindowToTop();
	m_editMdlgUnit.BringWindowToTop();
}

void CMetadataDlg::TabModifyMetadataAction()
{
	POSITION POS = m_listMetadata.GetFirstSelectedItemPosition();
	int p = m_listMetadata.GetNextSelectedItem(POS);
	if(p < 0)
		return;
	att_to_modify = m_metaAtts.GetAt(p);
	val_to_modify = m_metaVals.GetAt(p);
	unit_to_modify = m_metaUnits.GetAt(p);
	selected_pos_to_modify = p;

	m_editMdlgAtt.SetWindowText(att_to_modify);
	m_editMdlgVal.SetWindowText(val_to_modify);
	m_editMdlgUnit.SetWindowText(unit_to_modify);
}

void CMetadataDlg::ResetAttEditControls(bool readonly)
{
	m_editMdlgAtt.SetReadOnly(readonly);
	m_editMdlgVal.SetReadOnly(readonly);
	m_editMdlgUnit.SetReadOnly(readonly);
}

void CMetadataDlg::OnTcnSelchangeTabMedataDlg(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: Add your control notification handler code here
	int t = m_tabMetadata.GetCurSel();

	if(((t==2)||(t==3))&&(m_listMetadata.GetSelectedCount() != 1))
	{
		{
			::MessageBox(this->m_hWnd, "Please select only one item for this operation.", "modify",MB_OK);
			TabViewMetadataListAction();
			m_tabMetadata.SetCurSel(0);
			m_buttonMdlgDelete.SetWindowTextA("Delete");
			m_buttonMdlgDelete.ShowWindow(SW_SHOW);
			m_btnViewMetaInHTML.ShowWindow(SW_SHOW);
			*pResult = 0;
			return;
		}
	}

	switch(t)
	{
	case 0:  // metadata view tab
		TabViewMetadataListAction();
		m_operationMode = METADATA_DELETE_OP;
		m_buttonMdlgDelete.SetWindowTextA("Delete");
		m_buttonMdlgDelete.ShowWindow(SW_SHOW);
		m_btnViewMetaInHTML.ShowWindow(SW_SHOW);
		break;
	case 1:  // 1->add metadata tab
		TabAddModifyViewSelAction();
		m_editMdlgAtt.SetWindowText("");
		m_editMdlgVal.SetWindowText("");
		m_editMdlgUnit.SetWindowText("");
		ResetAttEditControls(false);
		m_buttonMdlgDelete.SetWindowTextA("Add");
		m_buttonMdlgDelete.ShowWindow(SW_SHOW);
		m_btnViewMetaInHTML.ShowWindow(SW_HIDE);
		m_operationMode = METADATA_ADD_OP;
		break;
	case 2:  // 2 -> modify selected metadata tab.
		TabAddModifyViewSelAction();
		TabModifyMetadataAction();
		ResetAttEditControls(false);
		m_buttonMdlgDelete.SetWindowTextA("Modify");
		m_buttonMdlgDelete.ShowWindow(SW_SHOW);
		m_btnViewMetaInHTML.ShowWindow(SW_HIDE);
		m_operationMode = METADATA_MODIFY_OP;
		break;
	case 3:  // view selected metadat
		TabAddModifyViewSelAction();
		TabModifyMetadataAction();
		ResetAttEditControls(true);
		m_buttonMdlgDelete.ShowWindow(SW_HIDE);
		m_btnViewMetaInHTML.ShowWindow(SW_HIDE);
		m_operationMode = METADATA_VIEW_SELECTED_OP;
		break;
	default:
		break;
	}

	m_labelMetadlgMsg.SetWindowText(" ");
	*pResult = 0;
}

#if 0
void CMetadataDlg::OnBnClickedButtonMetadlgViewLongval()
{
	// TODO: Add your control notification handler code here
	if(m_listMetadata.GetSelectedCount() != 1)
		return;

	if(m_metaAtts.GetSize() <= 0)
		return;

	POSITION POS = m_listMetadata.GetFirstSelectedItemPosition();
	int p = m_listMetadata.GetNextSelectedItem(POS);

	CString dlgTitle = "Metadata Value";
	CDispTextDlg tdlg(dlgTitle, m_metaVals[p], this);
	tdlg.DoModal();
}
#endif

void CMetadataDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	::SendMessage(frame->m_hWnd, METADATA_DLG_CLOSE_MSG, (WPARAM)m_registeredDlgID, NULL);
}

//void CMetadataDlg::OnWindowPosChanged(WINDOWPOS* lpwndpos)
//{
//	CDialog::OnWindowPosChanged(lpwndpos);
//	int i;
//	i = 1;
//	// TODO: Add your message handler code here
//}

void CMetadataDlg::OnSize(UINT nType, int cx, int cy)
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
	
	// collection name edit
	m_editCDNameWpath.GetWindowRect(&lrect);
	h = lrect.bottom - lrect.top;
	x = editCDNameWpath_leftoffset;
	y = editCDNameWpath_topoffset;
	w = dlg_w - editCDNameWpath_leftoffset - editCDNameWpath_rightoffset;
	m_editCDNameWpath.MoveWindow(x, y, w, h);

	// delete button
	m_buttonMdlgDelete.GetWindowRect(&lrect);
	w =  lrect.right - lrect.left;
	h = lrect.bottom - lrect.top;
	x = dlg_w - w - buttonMdlgDelete_rightoffset;
	y = buttonMdlgDelete_topoffset;
	m_buttonMdlgDelete.MoveWindow(x, y, w, h);

	int delete_button_bottom = y + h;

	// cancel button
	m_buttonCancel.GetWindowRect(&lrect);
	w =  lrect.right - lrect.left;
	h = lrect.bottom - lrect.top;
	x = dlg_w - w - buttonCancel_rightoffset;
	y = dlg_h - h - buttonCancel_bottomoffset;
	m_buttonCancel.MoveWindow(x, y, w, h);

	// tab control
	m_tabMetadata.GetWindowRect(&lrect);
	ScreenToClient(&lrect);
	x = lrect.left;
	y = lrect.top;
	w = dlg_w - x - tabMetadata_rightoffset;
	h = dlg_h - y - tabMetadata_bottomoffset;
	m_tabMetadata.MoveWindow(x, y, w, h);

	// metadata list
	m_listMetadata.GetWindowRect(&lrect);
	ScreenToClient(&lrect);
	x = lrect.left;
	y = lrect.top;
	w = dlg_w - x - listMetadata_rightoffset;
	h = dlg_h - y - listMetadata_bottomoffset;
	m_listMetadata.MoveWindow(x, y, w, h);

	// operation status label
	m_labelMetadlgMsg.GetWindowRect(&lrect);
	ScreenToClient(&lrect);
	w =  lrect.right - lrect.left;
	h = lrect.bottom - lrect.top;
	x = labelMetadlgMsg_leftoffset;
	y = dlg_h - h - labelMetadlgMsg_bottomoffset;
	m_labelMetadlgMsg.MoveWindow(x, y, w, h);

	// att label
	/*
	m_labelMdlgAtt.GetWindowRect(&lrect);
	ScreenToClient(&lrect);
	w =  lrect.right - lrect.left;
	h = lrect.bottom - lrect.top;
	x = labelMdlgAtt_leftoffset;
	y = labelMdlgAtt_topoffset;
	m_labelMdlgAtt.MoveWindow(x, y, w, h);
	*/

	// att name edit
	m_editMdlgAtt.GetWindowRect(&lrect);
	h = lrect.bottom - lrect.top;
	x = editMdlgAtt_leftoffset;
	y = editMdlgAtt_topoffset;
	w = dlg_w - editMdlgAtt_leftoffset - editMdlgAtt_rightoffset;
	m_editMdlgAtt.MoveWindow(x, y, w, h);

	// att unit edit
	m_editMdlgUnit.GetWindowRect(&lrect);
	h = lrect.bottom - lrect.top;
	x = editMdlgUnit_leftoffset;
	y = dlg_h - h - editMdlgUnit_bottomoffset;
	w = dlg_w - editMdlgUnit_leftoffset - editMdlgUnit_rightoffset;
	m_editMdlgUnit.MoveWindow(x, y, w, h);
	int editMdlgUnit_topoffset = y;

	// att value edit
	m_editMdlgVal.GetWindowRect(&lrect);
	h = editMdlgUnit_topoffset - editMdlgVal_topoffset - vgap_between_editVal_editUnit;
	if(h < 0)
		h = 10;
	x = editMdlgVal_leftoffset;
	y = editMdlgVal_topoffset;
	w = dlg_w - editMdlgVal_leftoffset - editMdlgVal_rightoffset;
	m_editMdlgVal.MoveWindow(x, y, w, h);

	// view html button
	CButton *viewhtml_btn = (CButton *)this->GetDlgItem(IDC_BUTTON_VIEWHTML_METADAT_DLG);
	if(viewhtml_btn != NULL)
	{
		viewhtml_btn->GetWindowRect(&lrect);
		h = lrect.bottom - lrect.top;
		w = lrect.right - lrect.left;
		y = delete_button_bottom + offset_delete_viewhtml_buttons;
		x = dlg_w - w - buttonMdlgDelete_rightoffset;
		viewhtml_btn->MoveWindow(x, y, w, h);
	}
}

void CMetadataDlg::OnBnClickedButtonViewhtmlMetadatDlg()
{
	// TODO: Add your control notification handler code here
	if(m_metaAtts.GetSize() <= 0)
	{
		//AfxMessageBox("Currently there is no metadata to be viewed.");
		return;
	}

	CString html_info = CString("<html><body bgcolor=#CCCC99><h1>Metadata for the ");
	switch(m_iObjType)
	{
	case irodsWinObj::IRODS_COLLECTION_TYPE:
		html_info += CString("collection:</h1>\n");
		break;
	case irodsWinObj::IRODS_DATAOBJ_TYPE:
		html_info += CString("file:</h1>\n");
		break;
	default:
		return;
	}
	
	html_info +=CString("<p>") + m_sObjNameWithPath + CString("</p><br>\n");
	html_info += CString("<p>\n");
	html_info += CString("<table border=2 cellpadding=2 frame=box>\n");
	html_info += CString("<tr bgcolor=\"#000066\"><td><font color=\"white\">Attribute Name</font></td>") +
				CString("<td><font color=\"white\">Attribue Value</font></td>") +
 				CString("<td><font color=\"white\">Attribue Unit</td></font></tr>\n");
	for(int i=0; i<m_metaAtts.GetSize();i++)
	{
		html_info += CString("<tr><td>") + m_metaAtts[i] + CString("</td> <td>") + m_metaVals[i] +
 				CString("</td> <td>")+ m_metaUnits[i] + CString("</td></tr>\n");
	}
	html_info += CString("</p>\n");
	html_info += CString("</table></body></html>");

	CDialogWebDisp dlg(html_info, this);
	dlg.DoModal();
}

HBRUSH CMetadataDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  Change any attributes of the DC here
	CBrush m_brBgColor;   
    m_brBgColor.CreateSolidBrush(RGB(255, 255, 255));  

	if((pWnd->m_hWnd == m_labelMdlgAtt.m_hWnd) || (pWnd->m_hWnd ==m_labelMdlgVal.m_hWnd) ||
		(pWnd->m_hWnd == m_labelMdlgUnit.m_hWnd))
	{
		pDC->SetBkColor(RGB(255,255,255));
		pDC->SetTextColor(RGB(0,0,0));
		hbr = m_brBgColor;
	}

	// TODO:  Return a different brush if the default is not desired
	return hbr;
}
