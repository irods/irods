#pragma once
#include "afxwin.h"
#include "afxcmn.h"

#define METADATA_ATT  0
#define METADATA_VAL  1
#define METADATA_UNIT 2

// for metadat operation mode
#define METADATA_DELETE_OP  1
#define METADATA_ADD_OP     2
#define METADATA_MODIFY_OP  3
#define METADATA_VIEW_SELECTED_OP 4

// CMetadataDlg dialog

class CMetadataDlg : public CDialog
{
	DECLARE_DYNAMIC(CMetadataDlg)

public:
	CMetadataDlg(CString & sObjName, int iObjType, CWnd* pParent = NULL);   // standard constructor
	virtual ~CMetadataDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG_METADATA };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	virtual BOOL OnInitDialog();
	//CEdit m_editCDNameWpath;
	//CStatic m_labelCD;

public:
	int m_registeredDlgID;
private:
	bool m_dlgInitialized;
	int m_iObjType;
	CString m_sObjNameWithPath;
	CStringArray m_metaAtts, m_metaVals, m_metaUnits;

	void AddMetadata();
	void ModifyMetadata();
	void DeleteMetadata();
	void TabViewMetadataListAction();
	void TabAddModifyViewSelAction();
	void TabModifyMetadataAction();
	void ResetAttEditControls(bool readonly);

	int m_operationMode;
	// for modify only
	CString att_to_modify;
	CString val_to_modify;
	CString unit_to_modify;
	int selected_pos_to_modify;

	void CleanMetaData();
	//CListCtrl m_listMetadata;
public:
	//afx_msg void OnBnClickedButtonAddMetadlg();
	//afx_msg void OnBnClickedButtonDeleteMetadataDlg();

private:
	CEdit m_editCDNameWpath;
	CStatic m_labelCD;
	CTabCtrl m_tabMetadata;
	CListCtrl m_listMetadata;
	CButton m_buttonMdlgDelete;
	CStatic m_labelMdlgAtt;
	CStatic m_labelMdlgVal;
	CStatic m_labelMdlgUnit;
	CEdit m_editMdlgAtt;
	CEdit m_editMdlgVal;
	CEdit m_editMdlgUnit;

	CStatic m_labelMetadlgMsg;
	CButton m_buttonCancel;

private:
	int   editCDNameWpath_rightoffset, editCDNameWpath_leftoffset, editCDNameWpath_topoffset;
	int   tabMetadata_rightoffset, tabMetadata_bottomoffset;
	int   buttonMdlgDelete_rightoffset, buttonMdlgDelete_topoffset;
	int   buttonCancel_rightoffset, buttonCancel_bottomoffset;
	int   listMetadata_rightoffset, listMetadata_bottomoffset;
	int   labelMetadlgMsg_leftoffset, labelMetadlgMsg_bottomoffset;

	// for add and modify metadata
	//int   labelMdlgAtt_leftoffset, labelMdlgAtt_topoffset;
	int   editMdlgAtt_rightoffset, editMdlgAtt_leftoffset, editMdlgAtt_topoffset;
	int   editMdlgVal_rightoffset, editMdlgVal_leftoffset, editMdlgVal_topoffset;
	int   vgap_between_editVal_editUnit;
	int   editMdlgUnit_rightoffset, editMdlgUnit_leftoffset, editMdlgUnit_bottomoffset;
	int   offset_delete_viewhtml_buttons;

public:
	afx_msg void OnBnClickedButtonDeleteMetadataDlg();
	afx_msg void OnTcnSelchangeTabMedataDlg(NMHDR *pNMHDR, LRESULT *pResult);
	//afx_msg void OnBnClickedButtonMetadlgViewLongval();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	afx_msg void OnBnClickedButtonViewhtmlMetadatDlg();
private:
	CButton m_btnViewMetaInHTML;
public:
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
