// MetaView.cpp : implementation file
//

#include "stdafx.h"
#include "inquisitor.h"
#include "MetaView.h"
#include "InquisitorDoc.h"
#include "winiObjects.h"
#include "CCEdit.h"

//#include "winbase.h"
#define REPORT_ATTR 0
#define REPORT_VALU 1
#define REPORT_WIDTH_ATTR 200
#define REPORT_WIDTH_VALU 200

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//extern CMutex gMutex;
//extern CCriticalSection gCS;
extern CMutex gCS;
extern int gCount;

extern CImageList gIconListLarge;
extern CImageList gIconListSmall;


/////////////////////////////////////////////////////////////////////////////
// CMetaView

IMPLEMENT_DYNCREATE(CMetaView, CListView)

CMetaView::CMetaView()
{
	m_bInitialized = false;
	m_bEditing = false;
}

CMetaView::~CMetaView()
{
}


BEGIN_MESSAGE_MAP(CMetaView, CListView)
	//{{AFX_MSG_MAP(CMetaView)
	ON_NOTIFY_REFLECT(NM_SETFOCUS, OnSetfocus)
	ON_UPDATE_COMMAND_UI(ID_NEW_COLLECTION, OnUpdateNewCollection)
	ON_UPDATE_COMMAND_UI(ID_NEW_CONTAINER, OnUpdateNewContainer)
	//ON_UPDATE_COMMAND_UI(ID_QUERY, OnUpdateQuery)
	ON_UPDATE_COMMAND_UI(ID_UPLOAD, OnUpdateUpload)
	ON_UPDATE_COMMAND_UI(ID_UPLOADFOLDER, OnUpdateUploadFolder)
	ON_UPDATE_COMMAND_UI(ID_DOWNLOAD, OnUpdateDownload)
	ON_UPDATE_COMMAND_UI(ID_REPLICATE, OnUpdateReplicate)
	ON_UPDATE_COMMAND_UI(ID_SYNCHRONIZE, OnUpdateSynchronize)
	ON_UPDATE_COMMAND_UI(ID_ACCESS_CTRL, OnUpdateAccessCtrl)
	//ON_COMMAND(ID_DELETE, OnDelete)
	ON_COMMAND(ID_METADATA, OnMetadata)
	//ON_UPDATE_COMMAND_UI(ID_METADATA, OnUpdateMetadata)
	ON_UPDATE_COMMAND_UI(ID_DELETE, OnUpdateDelete)
	ON_COMMAND(ID_RENAME, OnRename)
	ON_UPDATE_COMMAND_UI(ID_RENAME, OnUpdateRename)
	ON_UPDATE_COMMAND_UI(ID_COMMENT, OnUpdateComment)
	ON_WM_LBUTTONDOWN()
	ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, OnEndlabeledit)
	ON_COMMAND(ID_NEW_META, OnNewMeta)
	ON_UPDATE_COMMAND_UI(ID_NEW_META, OnUpdateNewMeta)
	ON_NOTIFY_REFLECT(NM_RCLICK, OnRclick)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMetaView drawing

void CMetaView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CListCtrl& refCtrl = GetListCtrl();
	refCtrl.InsertItem(0,"Item!");
}

/////////////////////////////////////////////////////////////////////////////
// CMetaView diagnostics

#ifdef _DEBUG
void CMetaView::AssertValid() const
{
	CListView::AssertValid();
}

void CMetaView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMetaView message handlers

BOOL CMetaView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if(!CListView::PreCreateWindow(cs))
		return FALSE;

	cs.style |=  LVS_EDITLABELS;

	return TRUE;
}

//this function is called once per connection, but may be called several times during the
//lifetime of the program because single document interfaces resuse the document object.
//this operation needs to be done after the window's been created, but only once during
//the lifetime of the process - hence the variable.
void CMetaView::OnInitialUpdate() 
{
	if(!m_bInitialized)
	{
		CListCtrl& list = GetListCtrl();

		list.SetImageList(&gIconListSmall,LVSIL_SMALL);
		list.SetImageList(&gIconListLarge,LVSIL_NORMAL);

		list.InsertColumn(REPORT_ATTR,"Attribute", LVCFMT_LEFT, REPORT_WIDTH_ATTR);
		list.InsertColumn(REPORT_VALU,"Value", LVCFMT_LEFT, REPORT_WIDTH_VALU);

		list.ModifyStyle(0, LVS_REPORT);

		m_bInitialized = true;
	}

	//OnUpdate(this, NULL, NULL);

}

#if _DEBUG
CInquisitorDoc* CMetaView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CInquisitorDoc)));
	return (CInquisitorDoc*)m_pDocument;
}
#endif

#if 0
void CMetaView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	CListCtrl& list = GetListCtrl();

	return;

	list.DeleteAllItems();
	m_nRedirector.clear();

	if(3 == lHint)	//code to clear screen
		return;

	CInquisitorDoc* pDoc = GetDocument();

	WINI::INode* node = pDoc->GetCurrentNode();

	if(NULL == node)
		return;

	int count = node->CountChildren();

	m_nRedirector.resize(count);

	WINI::INode* child;

	int type;

	DWORD dwStyle = GetWindowLong(list.m_hWnd, GWL_STYLE);

	int image, selected_image;

	for(int i = 0; i < node->CountChildren(); i++)
	{
		child = node->GetChild(i);
		type = child->GetType();

		if(WINI_METADATA != type)
			continue;

		WINI::IMetadataNode* metadata = (WINI::IMetadataNode*)child;

		const char * blah = metadata->GetName();

		GetImage(type, 0, child->isOpen(), image, selected_image);

	
		int position;

   		if(dwStyle & LVS_REPORT && !(dwStyle & LVS_SMALLICON))
		{
			position = list.InsertItem(i, metadata->GetAttribute(), image);
			m_nRedirector[position] = i;
			list.SetItemText(position, 1, metadata->GetValue());

		}else
		{
			position = list.InsertItem(i, metadata->GetName(), image);
			m_nRedirector[position] = i;
		}
	}
}
#endif

void CMetaView::GetImage(const int& node_type, const char* sub_type, bool isOpen, int &image, int& selected_image)
{
	switch(node_type) 
	{ 
	case WINI_DATASET: 
		image = 1; 
		break; 
	case WINI_COLLECTION: 
		image = 2; 
		break; 
	case WINI_METADATA: 
		image = 3; 
		break; 
	case WINI_ACCESS: 
		image = 12; 
		break; 
	case WINI_RESOURCE: 
		if(0 == strcmp("unix file system", sub_type)) 
			image = 14; 
		else if(0 == strcmp("hpss file system", sub_type)) 
			image = 15; 
		else if(0 == strcmp("oracle dblobj database", sub_type)) 
			image = 13; 
		else 
			image = 7; 
		break; 
	case WINI_QUERY: 
		image = 4; 
		break; 
	case WINI_DOMAIN: 
		image = 10; 
		break; 
	case WINI_USER: 
		image = 9; 
		break; 
	case WINI_ZONE: 
		image = 16; 
		break; 
	default: 
		image = 0; 
		break; 
	} 
	 
	selected_image = image + 42; 

	if(isOpen) 
		image += 21;
}

void CMetaView::OnSetfocus(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
}

void CMetaView::OnUpdateNewCollection(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(FALSE);
}

void CMetaView::OnUpdateNewContainer(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(FALSE);
}

#if 0
void CMetaView::OnUpdateQuery(CCmdUI* pCmdUI) 
{
	CInquisitorDoc* pDoc = GetDocument();
	
	//WINI::INode* Node = pDoc->GetCurrentNode();

	if(NULL == Node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	switch(Node->GetType())
	{
	case WINI_COLLECTION:
	case WINI_QUERY:
		break;
	default:
		pCmdUI->Enable(FALSE);
	}	
}
#endif

void CMetaView::OnUpdateUpload(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(FALSE);
}

void CMetaView::OnUpdateUploadFolder(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(FALSE);
}

void CMetaView::OnUpdateDownload(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(FALSE);
}

void CMetaView::OnUpdateReplicate(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(FALSE);
}

void CMetaView::OnUpdateSynchronize(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(FALSE);
}

void CMetaView::OnUpdateAccessCtrl(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(FALSE);
}

#if 0
void CMetaView::OnDelete() 
{
	CListCtrl& list = GetListCtrl();

	int count = list.GetSelectedCount();

	gCount = count;

	POSITION POS;
	int pos;

	CInquisitorDoc* pDoc = GetDocument();

	if(IDNO == AfxMessageBox("Do you wish to delete the selected item(s)?", MB_YESNO))
	{
		return;
	}

	//WINI::INode* parent = pDoc->GetCurrentNode();
	WINI::INode* child;

	POS = list.GetFirstSelectedItemPosition();
	int type;

	while(POS)
	{
		pos = list.GetNextSelectedItem(POS);
		child = parent->GetChild(m_nRedirector[pos]);
		type = child->GetType();	//if it cans, it'll can here before it hits the thread
		ASSERT(WINI_METADATA == type);
		//pDoc->Delete(child);
	}
}
#endif

void CMetaView::OnMetadata() 
{
	// TODO: Add your command handler code here
	AfxMessageBox("Extended Metadata dialog not yet implemented.");
}

#if 0
void CMetaView::OnUpdateMetadata(CCmdUI* pCmdUI) 
{
	CInquisitorDoc* pDoc = GetDocument();

	WINI::INode* node = pDoc->GetCurrentNode();

	switch(node->GetType())
	{
	case WINI_DATASET:
	case WINI_COLLECTION:
		break;
	default:
		pCmdUI->Enable(FALSE);
		break;
	}	
}
#endif

void CMetaView::OnUpdateDelete(CCmdUI* pCmdUI) 
{
	CListCtrl& list = GetListCtrl();

	if(list.GetSelectedCount() == 0)
		pCmdUI->Enable(FALSE);
}

void CMetaView::OnRename() 
{
	// TODO: Add your command handler code here
}

void CMetaView::OnUpdateRename(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(FALSE);
}

void CMetaView::OnUpdateComment(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(FALSE);
}

void CMetaView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	int index;
	CListView::OnLButtonDown(nFlags, point);

	int colnum;
	if( ( index = HitTestEx( point, &colnum )) != -1 )
	{
		int blah;
		for (unsigned int i=0; i < GetListCtrl().GetSelectedCount();i++)
		{
			blah = GetListCtrl().GetNextItem(i, LVNI_SELECTED);
			GetListCtrl().SetItemState(blah, 0, LVIS_SELECTED | LVIS_FOCUSED);
		}

		if(colnum > 0)
		{
			if( GetWindowLong(m_hWnd, GWL_STYLE) & LVS_EDITLABELS )
				EditSubLabel( index, colnum );
		}
		else
		{
			GetListCtrl().SetItemState( index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		}
	}
}

void CMetaView::OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO *plvDispInfo = (LV_DISPINFO *)pNMHDR;
	LV_ITEM	*plvItem = &plvDispInfo->item;


	*pResult = 0;
	CInquisitorDoc* pDoc;
	WINI::IMetadataNode *meta;


	//pDoc functions call update so no need to set pResult to 1.
	//but we need to reset manually (remove the invalid new item) if it fails ...
	if(plvItem->pszText != NULL)
	{
		if(0 != strcmp(plvItem->pszText, ""))
		{
			pDoc = (CInquisitorDoc*)GetDocument();
			if(plvItem->iSubItem == 0)
			{
				//pDoc->AddNewMetadata(plvItem->pszText, "");
				EditSubLabel(plvItem->iItem, 1);
			}else
			{
				//meta = (WINI::IMetadataNode*)(pDoc->GetCurrentNode()->GetChild(m_nRedirector[plvItem->iItem]));

				//if(0 != strcmp(meta->GetValue(), plvItem->pszText))
				//	pDoc->SetMetadataValue(meta, plvItem->pszText);
			}
		}else
		{
			if(plvItem->iSubItem == 0)
				GetListCtrl().DeleteItem(plvItem->iItem);
		}
	}


}

int CMetaView::HitTestEx(CPoint &point, int *col) const
{
	CListCtrl& list = GetListCtrl();

	int colnum = 0;
	int row = list.HitTest( point, NULL );
	
	if( col ) *col = 0;

	// Make sure that the ListView is in LVS_REPORT
	if((GetWindowLong(m_hWnd, GWL_STYLE) & LVS_TYPEMASK) != LVS_REPORT )
		return row;

	// Get the top and bottom row visible
	row = list.GetTopIndex();
	int bottom = row + list.GetCountPerPage();
	if( bottom > list.GetItemCount() )
		bottom = list.GetItemCount();
	
	// Get the number of columns
	CHeaderCtrl* pHeader = (CHeaderCtrl*)list.GetDlgItem(0);
	int nColumnCount = pHeader->GetItemCount();

	// Loop through the visible rows
	for( ;row <=bottom;row++)
	{
		// Get bounding rect of item and check whether point falls in it.
		CRect rect;
		list.GetItemRect( row, &rect, LVIR_BOUNDS );
		if( rect.PtInRect(point) )
		{
			// Now find the column
			for( colnum = 0; colnum < nColumnCount; colnum++ )
			{
				int colwidth = list.GetColumnWidth(colnum);
				if( point.x >= rect.left 
					&& point.x <= (rect.left + colwidth ) )
				{
					if( col ) *col = colnum;
					return row;
				}
				rect.left += colwidth;
			}
		}
	}
	return -1;

}

CEdit* CMetaView::EditSubLabel( int nItem, int nCol )
{
	CListCtrl& list = GetListCtrl();

	// The returned pointer should not be saved

	// Make sure that the item is visible
	if( !list.EnsureVisible( nItem, TRUE ) ) return NULL;

	// Make sure that nCol is valid
	CHeaderCtrl* pHeader = (CHeaderCtrl*)list.GetDlgItem(0);
	int nColumnCount = pHeader->GetItemCount();
	if( nCol >= nColumnCount || list.GetColumnWidth(nCol) < 5 )
		return NULL;

	// Get the column offset
	int offset = 0;
	for( int i = 0; i < nCol; i++ )
		offset += list.GetColumnWidth( i );

	CRect rect;
	list.GetItemRect( nItem, &rect, LVIR_BOUNDS );

	// Now scroll if we need to expose the column
	CRect rcClient;
	list.GetClientRect( &rcClient );
	if( offset + rect.left < 0 || offset + rect.left > rcClient.right )
	{
		CSize size;
		size.cx = offset + rect.left;
		size.cy = 0;
		list.Scroll( size );
		rect.left -= size.cx;
	}

	// Get Column alignment
	LV_COLUMN lvcol;
	lvcol.mask = LVCF_FMT;
	list.GetColumn( nCol, &lvcol );
	DWORD dwStyle ;
	if((lvcol.fmt&LVCFMT_JUSTIFYMASK) == LVCFMT_LEFT)
		dwStyle = ES_LEFT;
	else if((lvcol.fmt&LVCFMT_JUSTIFYMASK) == LVCFMT_RIGHT)
		dwStyle = ES_RIGHT;
	else dwStyle = ES_CENTER;

	//rect.left += offset+4;
	//rect.right = rect.left + list.GetColumnWidth( nCol ) - 3 ;
	rect.left += offset;
	rect.right = rect.left + list.GetColumnWidth( nCol );
	

	if( rect.right > rcClient.right) rect.right = rcClient.right;

	dwStyle |= WS_BORDER|WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL;
	CEdit *pEdit = new CCEdit(nItem, nCol, list.GetItemText( nItem, nCol ));
	pEdit->Create( dwStyle, rect, this, IDC_BLAH);

	return pEdit;

}

void CMetaView::OnNewMeta() 
{

	CListCtrl& list = GetListCtrl();

	
	DWORD dwStyle = list.GetStyle() & LVS_TYPEMASK;

	if(!(dwStyle && LVS_REPORT))
		list.ModifyStyle(LVS_TYPEMASK, LVS_REPORT);
	
	int count = list.GetItemCount();

	int position = list.InsertItem(count, "", 3);

	CEdit* edit = EditSubLabel(position, 0);
}

void CMetaView::OnUpdateNewMeta(CCmdUI* pCmdUI) 
{
}

void CMetaView::OnRclick(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	CListCtrl& list = GetListCtrl();
	int count = list.GetSelectedCount();

//	POSITION POS = list.GetFirstSelectedItemPosition();
//	int i = list.GetNextSelectedItem(POS);
	//CInquisitorDoc* pDoc = GetDocument();
//	WINI::INode* node = pDoc->GetCurrentNode();
//	node = node->GetChild(m_nRedirector[i]);

	CMenu menu;
	POINT pt;

	GetCursorPos(&pt);
	menu.CreatePopupMenu();

	if(0 == count)
		menu.AppendMenu(MF_ENABLED, ID_NEW_META, "Add Metadata");
	else
		menu.AppendMenu(MF_ENABLED, ID_DELETE, "Delete");

	menu.TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y,this);
}