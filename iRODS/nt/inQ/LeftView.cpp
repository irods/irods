   #include "stdafx.h"
#include "Inquisitor.h"
#include "InquisitorDoc.h"
#include "LeftView.h"
#include "MainFrm.h"
#include "winiObjects.h"
#include "GenericDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CMutex gCS;
extern int gCount;
extern bool g_bIsDragging;
extern bool g_bDeleteOriginalPostCopy;
extern UINT gInqClipboardFormat;
int image_reverse_map[];
extern CImageList gIconListSmall;

extern std::vector<WINI::INode*> gNodeDragList;
extern HANDLE ghDragList;
extern CharlieSource* gDS;
extern bool gCaptureOn;
extern unsigned int gOn;

IMPLEMENT_DYNCREATE(CLeftView, CTreeView)

BEGIN_MESSAGE_MAP(CLeftView, CTreeView)
	//{{AFX_MSG_MAP(CLeftView)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	ON_COMMAND(ID_DELETE, OnDelete)
	ON_WM_CREATE()
	ON_COMMAND(ID_DOWNLOAD, OnDownload)
	ON_COMMAND(ID_ACCESS_CTRL, OnAccessCtrl)
	ON_COMMAND(ID_UPLOAD, OnUpload)
	ON_COMMAND(ID_UPLOADFOLDER, OnUploadFolder)
	ON_COMMAND(ID_NEW_COLLECTION, OnNewCollection)
	ON_UPDATE_COMMAND_UI(ID_VIEW_LARGEICON, OnUpdateViewLargeicon)
	ON_UPDATE_COMMAND_UI(ID_VIEW_LIST, OnUpdateViewList)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SMALLICON, OnUpdateViewSmallicon)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DETAILS, OnUpdateViewDetails)
	ON_UPDATE_COMMAND_UI(ID_UPLOAD, OnUpdateUpload)
	ON_UPDATE_COMMAND_UI(ID_UPLOADFOLDER, OnUpdateUploadFolder)
	ON_UPDATE_COMMAND_UI(ID_DOWNLOAD, OnUpdateDownload)
	ON_UPDATE_COMMAND_UI(ID_NEW_COLLECTION, OnUpdateNewCollection)
	ON_UPDATE_COMMAND_UI(ID_ACCESS_CTRL, OnUpdateAccessCtrl)
	ON_COMMAND(ID_NEW_CONTAINER, OnNewContainer)
	ON_UPDATE_COMMAND_UI(ID_REPLICATE, OnUpdateReplicate)
	ON_COMMAND(ID_REPLICATE, OnReplicate)
	ON_COMMAND(ID_METADATA, OnMetadata)
	ON_UPDATE_COMMAND_UI(ID_METADATA, OnUpdateMetadata)
	ON_UPDATE_COMMAND_UI(ID_SYNCHRONIZE, OnUpdateSynchronize)
	ON_UPDATE_COMMAND_UI(ID_NEW_CONTAINER, OnUpdateNewContainer)
	ON_UPDATE_COMMAND_UI(ID_QUERY, OnUpdateQuery)
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnBegindrag)
	ON_UPDATE_COMMAND_UI(ID_DELETE, OnUpdateDelete)
	ON_COMMAND(ID_RENAME, OnRename)
	ON_UPDATE_COMMAND_UI(ID_RENAME, OnUpdateRename)
	ON_COMMAND(ID_COMMENT, OnComment)
	ON_UPDATE_COMMAND_UI(ID_COMMENT, OnUpdateComment)
	ON_NOTIFY_REFLECT(TVN_BEGINLABELEDIT, OnBeginlabeledit)
	ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT, OnEndlabeledit)
	ON_COMMAND(ID_NEW_META, OnNewMeta)
	ON_UPDATE_COMMAND_UI(ID_NEW_META, OnUpdateNewMeta)
	//}}AFX_MSG_MAP


END_MESSAGE_MAP()
//ON_REGISTERED_MESSAGE(msgMyMsg, OnDelete)

CLeftView::CLeftView()
{
	m_bDontChange = false;
	m_bPossibleDrag = false;
	m_ReverseMap.InitHashTable(181);
	m_RZoneToCZoneMap.InitHashTable(17);
	m_CZoneToRZoneMap.InitHashTable(17);
}

CLeftView::~CLeftView()
{
}

BOOL CLeftView::PreCreateWindow(CREATESTRUCT& cs)
{
	if(!CTreeView::PreCreateWindow(cs))
		return FALSE;

	cs.style |= TVS_HASLINES | TVS_HASBUTTONS | TVS_SHOWSELALWAYS | TVS_EDITLABELS;

	return TRUE;
}

void CLeftView::OnDraw(CDC* pDC)
{
	CInquisitorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
}

void CLeftView::Nuke(CTreeCtrl& tree, WINI::INode* node)
{
	int count = node->CountChildren();
	WINI::INode* child;

	m_ReverseMap.RemoveKey(node);
	int i;
	for(i = 0; i < count; i++)
	{
		child = node->GetChild(i);
		Nuke(tree, child);
	}
}

HTREEITEM CLeftView::Repopulate(WINI::INode* srbNode)
{
	HTREEITEM hNode;
	HTREEITEM hChild;
	HTREEITEM hNext;

	m_ReverseMap.Lookup(srbNode, hNode);

	CTreeCtrl& tree = GetTreeCtrl();

	Nuke(tree, srbNode);				//clean map
	m_ReverseMap.SetAt(srbNode, hNode);	//Nuke wipes out the reverse mapping for this node as well, which we don't want

	//delete all children, but not parent itself
	//this prevents an OnSelect from deleting the parent
	hNext = tree.GetChildItem(hNode);

	m_bDontChange = true;	//this is to cover times when we need to repopulate the whole tree
	//but we obviously don't want to opentree on each value. (this is done 'cause deleting an item
	//which is currently selected can cause an automatic onselchange to the next element

	CString mystring;
	
	while(hNext)
	{
		hChild = hNext;
		mystring = tree.GetItemText(hChild);
//CHARLIECHARLIEC		there is a problem here where the local root is incorporated into the same root as the zones so when we nuke the root local domain then we end up nuking all the zones as well....
		hNext = tree.GetNextSiblingItem(hChild);
		tree.DeleteItem(hChild);
	}

	m_bDontChange = false;

	Populate(hNode, srbNode);

	return hNode;
}

void CLeftView::Populate(HTREEITEM node, WINI::INode* srbNode)
{
	int count = srbNode->CountChildren();

	CTreeCtrl& tree = GetTreeCtrl();

	WINI::INode* ptr;
	int type, image, selected_image;
	const char* sub_type = NULL;

	for(int i = 0; i < count; i++)
	{
		ptr = srbNode->GetChild(i);
		type = ptr->GetType();

		if(WINI_RESOURCE == type)
			sub_type = ((WINI::IResourceNode*)ptr)->GetResourceType();


		GetImage(ptr, image, selected_image);

		HTREEITEM child = tree.InsertItem(ptr->GetName(),image, selected_image, node);

		tree.SetItemData(child, (DWORD)ptr);
		m_ReverseMap.SetAt(ptr, child);

		Populate(child, ptr);
	}
}

void CLeftView::OnInitialUpdate()
{
	//This implementation keeps default from calling OnUpdate function.
	int foo;
	foo = 1;
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	frame->FooFighter();
}

LRESULT CLeftView::ConnectedUpdate(WPARAM wParam, LPARAM lParam)
{
	WINI::StatusCode status((int)wParam);
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CInquisitorDoc* pDoc = GetDocument();

	static char buf[1024];

	if(!status.isOk())
	{
		//take care of this if(SRB_ERROR_NO_HOME_COLLECTION != status)
		if(0 != status)
		{
			sprintf(buf, "Connection failed: %s", status.GetError());
			frame->SetStatusMessage(buf);
			frame->OnStopAnimation(NULL, NULL);
			pDoc->OnFailedConnection();
			return FALSE;
		}
	}

	pDoc->OnCompletedConnection();

	CTreeCtrl& tree = GetTreeCtrl();

	//clear tree (SDI document reuses document and views)
	m_ReverseMap.RemoveAll();
	tree.DeleteAllItems();
	tree.SetImageList(&gIconListSmall,TVSIL_NORMAL);

	HTREEITEM h1, h2;

	WINI::INode* catalog = pDoc->GetMCAT();
	int count = catalog->CountChildren();

	for(int i = 0; i < count; i++)
	{
		WINI::INode* node = catalog->GetChild(i);
		ASSERT(node->GetType() == WINI_ZONE);
		h1 = tree.InsertItem(node->GetName(), 38, 59);
		tree.SetItemData(h1, (DWORD_PTR)node);
		m_ReverseMap.SetAt(node, h1);
		Populate(h1, node);
	}


	WINI::INode* myhome = pDoc->GetHome();

	if(myhome)
	{
		m_ReverseMap.Lookup(myhome, h2);
		tree.Select(h2, TVGN_CARET);
	};

	pDoc->UpdateAllViews(this);

	frame->PostMessage(msgStopAnimation, NULL, NULL);
	frame->PostMessage(msgStatusLine, NULL, 0);
	frame->PostMessage(msgRefillResourceList, NULL, NULL);

	return TRUE;
}

#if 0
	HTREEITEM crazy1;
	HTREEITEM crazy2;
	int i;
	for(i = 0; i < mcat->CountChildren(); i++)
	{
		WINI::INode* cnode = mcat->GetChild(i);

		if(cnode->GetType() == WINI_ZONE)
		{
		if(cnode == zone)
		{
			crazy1 = tree.InsertItem(cnode->GetName(),38, 59, m_collection_root);
			crazy2 = tree.InsertItem(cnode->GetName(),38, 59, m_resource_root);
		}
		else
		{
			crazy1 = tree.InsertItem(cnode->GetName(),16, 58, m_collection_root);
			crazy2 = tree.InsertItem(cnode->GetName(),16, 58, m_resource_root);
		}

		m_RZoneToCZoneMap.SetAt(crazy2, crazy1);
		m_CZoneToRZoneMap.SetAt(crazy1, crazy2);

		if(cnode == zone)
		{
			if(root)
			{
			tree.SetItemData(crazy1, (DWORD)root);
			m_ReverseMap.SetAt(root, crazy1);
			Populate(crazy1, root);
			tree.SetItemData(crazy2, (DWORD)resources);
			m_ReverseMap.SetAt(resources, crazy2);
			Populate(crazy2, resources);
			}else
			{
			tree.SetItemData(crazy1, (DWORD)cnode);
			m_ReverseMap.SetAt(cnode, crazy1);
			tree.SetItemData(crazy2, (DWORD)cnode);
			m_ReverseMap.SetAt(cnode, crazy2);
			}
		}else
		{
			tree.SetItemData(crazy1, (DWORD)cnode);
			m_ReverseMap.SetAt(cnode, crazy1);
			tree.SetItemData(crazy2, (DWORD)cnode);
			m_ReverseMap.SetAt(cnode, crazy2);
		}
		}
	}
#endif


#ifdef _DEBUG
void CLeftView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CLeftView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}

CInquisitorDoc* CLeftView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CInquisitorDoc)));
	return (CInquisitorDoc*)m_pDocument;
}
#endif //_DEBUG
#if 0
void CLeftView::GoGoGadgetResources()
{
	CInquisitorDoc* pDoc = GetDocument();

	CTreeCtrl& tree = GetTreeCtrl();

	WINI::INode* root = pDoc->GetRoot();
	WINI::INode* zone = pDoc->GetZone();
	WINI::INode* home = pDoc->GetHome();
	WINI::INode* mcat = zone->GetParent();

	HTREEITEM crazy;
	int i;
	for(i = 0; i < mcat->CountChildren(); i++)
	{
		WINI::INode* cnode = mcat->GetChild(i);

		if(cnode == zone)
			crazy = tree.InsertItem(cnode->GetName(),38, 59, m_resource_root);
		else
			crazy = tree.InsertItem(cnode->GetName(),16, 58, m_resource_root);

		if(cnode == zone)
		{
			tree.SetItemData(crazy, (DWORD)root);
			m_ReverseMap.SetAt(root, crazy);
			Populate(crazy, root);
		}else
		{
			tree.SetItemData(crazy, (DWORD)cnode);
			m_ReverseMap.SetAt(cnode, crazy);
		}
	}

}
#endif

void CLeftView::GetImage(WINI::INode* node, int &image, int& selected_image)
{
	const char* szptr;
	WINI::IResourceNode* resource;
	int value = node->GetType();
	

	if(WINI_SET == value)
	{
		value = ((WINI::ISetNode*)node)->GetSetType();
		if(WINI_RESOURCE == value)
			szptr = "";
	}else if(value == WINI_RESOURCE)
	{
		resource = (WINI::IResourceNode*)node;
		szptr = resource->GetResourceType();
	}

	switch(value) 
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
		if(0 == strcmp("unix file system", szptr))
			image = 14; 
		else if(0 == strcmp("hpss file system", szptr))
			image = 15; 
		else if(0 == strcmp("oracle dblobj database", szptr))
			image = 13; 
		else 
			image = 7; 
		break; 
	case WINI_QUERY: 
		image = 5; 
		break; 
	case WINI_DOMAIN: 
		image = 10; 
		break; 
	case WINI_USER: 
		image = 9; 
		break; 
	case WINI_SET:
		ASSERT(value != WINI_SET);
		break; 
	case WINI_ZONE: 
		image = 16; 
		break; 
	default: 
		image = 0; 
		break; 
	} 
	 
	selected_image = image + 42; 

	if(node->isOpen()) 
		image += 21;
}

WINI::INode* CLeftView::ProcessZone(WINI::INode* ptr, HTREEITEM hSelNode)
{
	int image, selected_image, type = ptr->GetType();
	const char* sub_type;
	WINI::StatusCode status;
	HTREEITEM hSelParent, hSelMirror;
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	CTreeCtrl& tree = GetTreeCtrl();
	CInquisitorDoc* pDoc = GetDocument();
	WINI::INode *mir;

	if(!ptr->isOpen(WINI_ALL ^ WINI_DOMAIN))	
	{
		((CMainFrame*)GetParentFrame())->OnGoAnimation(NULL, NULL);
	
		status = pDoc->OpenTree(ptr, 1, true, WINI_ALL ^ WINI_DOMAIN);

		((CMainFrame*)GetParentFrame())->OnStopAnimation(NULL, NULL);

		hSelParent = tree.GetParentItem(hSelNode);

		frame->PostMessage(msgRefillResourceList, (DWORD)ptr, NULL);

		if(-1023 == status)
		{
			if(m_collection_root == hSelParent)
			{
				tree.SetItemImage(hSelNode, 39,60);
				m_CZoneToRZoneMap.Lookup(hSelNode, hSelMirror);
				tree.SetItemImage(hSelMirror, 39,60);
			}else
			{
				ASSERT(m_resource_root == hSelParent);
				tree.SetItemImage(hSelNode, 39,60);
				m_RZoneToCZoneMap.Lookup(hSelNode, hSelMirror);
				tree.SetItemImage(hSelMirror, 39,60);
			}		
		}
		else if(-7001 == status)
		{
			if(m_collection_root == hSelParent)
			{
				tree.SetItemImage(hSelNode, 40,61);
				m_CZoneToRZoneMap.Lookup(hSelNode, hSelMirror);
				tree.SetItemImage(hSelMirror, 40,61);
			}else
			{
				ASSERT(m_resource_root == hSelParent);
				tree.SetItemImage(hSelNode, 40,61);
				m_RZoneToCZoneMap.Lookup(hSelNode, hSelMirror);
				tree.SetItemImage(hSelMirror, 40,61);
				ptr = ptr->GetChild(1);
			}	
		}
		else if(-2772 == status)
		{
			//your user/domain is not registered with this domain
			//do what?!?
		}else
		{
			//there's some case of down zones where teh resources come back but it's a null collection or something. we should investigate

			int blork = ptr->GetChild(0)->CountChildren();

			if(m_collection_root == hSelParent)// don't know why we needed to check for 0 && blork == 0)
			{
				mir = ptr->GetChild(1);
				ptr = ptr->GetChild(0);
				m_CZoneToRZoneMap.Lookup(hSelNode, hSelMirror);
			}else if(m_resource_root == hSelParent)
			{
				mir = ptr->GetChild(0);
				ptr = ptr->GetChild(1);
				m_RZoneToCZoneMap.Lookup(hSelNode, hSelMirror);
			}else
			{
				ASSERT(0 == 1);
			}


			tree.SetItemImage(hSelNode, 37, 58);
			tree.SetItemImage(hSelMirror, 37, 58);
			tree.SetItemData(hSelNode, (DWORD)ptr);
			tree.SetItemData(hSelMirror, (DWORD)mir);
			m_ReverseMap.SetAt(ptr, hSelNode);
			m_ReverseMap.SetAt(mir, hSelMirror);
			if(ptr)
				Populate(hSelNode, ptr);
			if(mir)
				Populate(hSelMirror, mir);
		}
	}

	return ptr;
}

WINI::INode* CLeftView::ProcessNonZone(WINI::INode* ptr, HTREEITEM hSelNode)
{
		WINI::INode* test;
		test = ptr->GetParent();
		CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
		CInquisitorDoc* pDoc = GetDocument();
		CTreeCtrl& tree = GetTreeCtrl();
	

		int type = ptr->GetType();
		
		if(test->GetType() == WINI_SET)
			type = test->GetType();

		if(ptr->GetParent()->GetType() == WINI_ZONE)
				frame->PostMessage(msgRefillResourceList, (DWORD)(ptr->GetParent()), NULL);

		int image, selected_image;
		const char* sub_type = NULL;




		if(WINI_RESOURCE == type)
			sub_type = ((WINI::IResourceNode*)ptr)->GetResourceType();

		if(!ptr->isOpen(WINI_ALL ^ WINI_ACCESS ^ WINI_METADATA))
		{
			//open the tree one level and then change the icon to show it's open
			((CMainFrame*)GetParentFrame())->OnGoAnimation(NULL, NULL);
			//AfxGetMainWnd()->PostMessage(msgGoAnimation, NULL, NULL);
			pDoc->OpenTree(ptr, 1, true);
			((CMainFrame*)GetParentFrame())->OnStopAnimation(NULL, NULL);
			//AfxGetMainWnd()->PostMessage(msgStopAnimation, NULL, NULL);
			GetImage(ptr, image, selected_image);
			tree.SetItemImage(hSelNode, image, selected_image);

			Populate(hSelNode, ptr);
		}

		return ptr;



}

void CLeftView::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	//OnSelChanged is called whenever the selection is
	//changed - whether internally or by the user.
	//m_bDontChange is toggled when we internally
	//change the selection.
	if(m_bDontChange)
		return;

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	*pResult = 0;
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hSelParent, hSelNode = tree.GetSelectedItem();

	//If somehow the user has managed to
	//deselect everything (does our internals ever
	//reach here?) simply return.
	if(hSelNode == NULL)
		return;

	//If an item is selected, find the SRB tree node
	//associated with it.
	CInquisitorDoc* pDoc = GetDocument();
	WINI::INode *mir, *ptr;
	
	DWORD_PTR tt = tree.GetItemData(hSelNode);
	ptr = (WINI::INode*)tt;


	WINI::StatusCode status;
	if(NULL != ptr)
	{
	if(!ptr->isOpen(WINI_ALL))
	{
		((CMainFrame*)GetParentFrame())->OnGoAnimation(NULL, NULL);
		status = pDoc->OpenTree(ptr, 1, true, WINI_ALL ^ WINI_DOMAIN);
		((CMainFrame*)GetParentFrame())->OnStopAnimation(NULL, NULL);
		//hSelParent = tree.GetParentItem(hSelNode);
		Populate(hSelNode, ptr);
		//frame->PostMessage(msgRefillResourceList, (DWORD)ptr, NULL);
	}
	int sel;
	int boo;
	this->GetImage(ptr, sel, boo);
	tree.SetItemImage(hSelNode, sel, boo);
	}


	pDoc->SetCurrentNode(ptr);
	pDoc->UpdateAllViews(this);
	UpdateDialogBars(ptr);
	return;

	WINI::INode* current = pDoc->GetCurrentNode();




////////////////
	int image, selected_image, type = ptr->GetType();
	const char* sub_type;

	HTREEITEM  hSelMirror;





	if(ptr == current && !ptr->isOpen(WINI_ALL))	
	{
		((CMainFrame*)GetParentFrame())->OnGoAnimation(NULL, NULL);
	
		status = pDoc->OpenTree(ptr, 1, true, WINI_ALL ^ WINI_DOMAIN);

		((CMainFrame*)GetParentFrame())->OnStopAnimation(NULL, NULL);

		hSelParent = tree.GetParentItem(hSelNode);

		frame->PostMessage(msgRefillResourceList, (DWORD)ptr, NULL);

		if(-1023 == status)
		{
			if(m_collection_root == hSelParent)
			{
				tree.SetItemImage(hSelNode, 39,60);
				m_CZoneToRZoneMap.Lookup(hSelNode, hSelMirror);
				tree.SetItemImage(hSelMirror, 39,60);
			}else
			{
				ASSERT(m_resource_root == hSelParent);
				tree.SetItemImage(hSelNode, 39,60);
				m_RZoneToCZoneMap.Lookup(hSelNode, hSelMirror);
				tree.SetItemImage(hSelMirror, 39,60);
			}		
		}
		else if(-7001 == status)
		{
			if(m_collection_root == hSelParent)
			{
				tree.SetItemImage(hSelNode, 40,61);
				m_CZoneToRZoneMap.Lookup(hSelNode, hSelMirror);
				tree.SetItemImage(hSelMirror, 40,61);
			}else
			{
				ASSERT(m_resource_root == hSelParent);
				tree.SetItemImage(hSelNode, 40,61);
				m_RZoneToCZoneMap.Lookup(hSelNode, hSelMirror);
				tree.SetItemImage(hSelMirror, 40,61);
				ptr = ptr->GetChild(1);
			}	
		}
		else if(-2772 == status)
		{
			//your user/domain is not registered with this domain
			//do what?!?
		}else
		{
			//there's some case of down zones where teh resources come back but it's a null collection or something. we should investigate

			int blork = ptr->GetChild(0)->CountChildren();

			if(m_collection_root == hSelParent)// don't know why we needed to check for 0 && blork == 0)
			{
				mir = ptr->GetChild(1);
				ptr = ptr->GetChild(0);
				m_CZoneToRZoneMap.Lookup(hSelNode, hSelMirror);
			}else if(m_resource_root == hSelParent)
			{
				mir = ptr->GetChild(0);
				ptr = ptr->GetChild(1);
				m_RZoneToCZoneMap.Lookup(hSelNode, hSelMirror);
			}else
			{
				ASSERT(0 == 1);
			}


			tree.SetItemImage(hSelNode, 37, 58);
			tree.SetItemImage(hSelMirror, 37, 58);
			tree.SetItemData(hSelNode, (DWORD)ptr);
			tree.SetItemData(hSelMirror, (DWORD)mir);
			m_ReverseMap.SetAt(ptr, hSelNode);
			m_ReverseMap.SetAt(mir, hSelMirror);
			if(ptr)
				Populate(hSelNode, ptr);
			if(mir)
				Populate(hSelMirror, mir);
		}
	}



//////////////////













#if 0
	if(NULL == ptr)
	{
		if(hSelNode == m_resource_root)	//this should only have to happen once
		{
			ptr = pDoc->GetResources(NULL);
			tree.SetItemData(m_resource_root, (DWORD)ptr);
			m_ReverseMap.SetAt(ptr, m_resource_root);
			pDoc->SetCurrentNode(ptr);
			pDoc->UpdateAllViews(this);
			UpdateDialogBars(ptr);
			return;
		}else if(hSelNode == m_query_root)
		{
			//do nothing right now
			return;
		}else if(hSelNode == m_collection_root)
		{
			return;
			//AfxMessageBox("change me!");
		}else
		{
			return;
		}
	}

	//initial updates, etc, delete tree. in which case, selchanged is called unnecessarily
	//if the node has not actually changed then there's no reason to update.
	//except when the node has not been opened
	if(ptr == pDoc->GetCurrentNode())
	{
		if(ptr->isOpen(WINI_ALL ^ WINI_ACCESS ^ WINI_METADATA))
		{
			return;
			UpdateDialogBars(ptr);
		}
	}

	int image, selected_image, type = ptr->GetType();
	const char* sub_type;

	WINI::StatusCode status;

	//Since zones are handled somewhat differently
	//than other nodes, there must be special processing done
	//for them - a kludge.

	if(ptr->GetType() == WINI_ZONE)
		ptr = ProcessZone(ptr,hSelNode);
	else
		ptr = ProcessNonZone(ptr,hSelNode);

#endif

	pDoc->SetCurrentNode(ptr);
	pDoc->UpdateAllViews(this);
	UpdateDialogBars(ptr);
}



/*
this function takes an inode and goes through the process of comparing the open/closed ness
of a node and the icon in the tree. It traverses up the tree until the root node, looking for
a closed icon and an open node. Because a closed node was opened during a query, it will have
n children, including, but not limited to our parameter. Thus, it's important for the subtree
to be located and updated.

Probably we can quit once we find an open tree and an open node but we'll leave this out for
now
*/
void CLeftView::VerifyTreeTraversingUp(WINI::INode* node)
{
	CTreeCtrl& tree = GetTreeCtrl();

	HTREEITEM hItem;

	//if the node is already in the map then it's not a new item.
	if(m_ReverseMap.Lookup(node, hItem))
			return;

	WINI::INode* parent = node->GetParent();

	int image, selected_image, type;

	while(NULL != parent)
	{
		if(m_ReverseMap.Lookup(parent, hItem))
		{
			//the parent node is displayed on the tree.
			//if the icon's closed we've found our point of update (obviously the node is open)
			//if the icon's open we can return (quit) because the tree is guaranteed to be
			//open from here on up - so no need for update.
			tree.GetItemImage(hItem, image, selected_image);
			type = parent->GetType();
			if(image == image_reverse_map[type])
			{
				//repopulate here
				//we know the tree here is closed but the node is open.
				//hence we must change the icon of this handle from closed to open
				//and populate it with the children it didn't know it had. =)
				image += 21;	//magic number for getting the selected version of the image.
				tree.SetItemImage(hItem, image, selected_image);
				Populate(hItem, parent);
			}else
			{
				return;
			}
		}else
		{
			//it's not there in the map - we need to keep looking until we find a node
			//attached to the tree.
			//parent = node->GetParent();
			parent = parent->GetParent();
		}
	}
}

void CLeftView::OnBack()
{
#if 1
	CInquisitorDoc* pDoc = GetDocument();
	pDoc->OnBack();

	pDoc->UpdateAllViews(NULL);
#endif

}

void CLeftView::OnForward()
{
	CInquisitorDoc* pDoc = GetDocument();
	pDoc->OnForward();

	pDoc->UpdateAllViews(NULL);

}

void CLeftView::OnAccessCtrl()
{
	CInquisitorDoc* pDoc = GetDocument();
	WINI::INode* child_node = pDoc->GetCurrentNode();
	pDoc->SetAccess(child_node);
	pDoc->UpdateAllViews(NULL, 2);
}

void CLeftView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CInquisitorDoc* pDoc = GetDocument();
	WINI::INode* child_node = pDoc->GetCurrentNode();

	CTreeCtrl& tree = GetTreeCtrl();

	//this handles case where a new connection is made
	if(NULL == child_node)
	{
		if(3 == lHint)
		{
			VERIFY(tree.DeleteAllItems());
			m_ReverseMap.RemoveAll();
			((CMainFrame*)AfxGetMainWnd())->ClearStack(true);
		}

		return;
	}

	static int l = 0;

	WINI::INode *blah; 

	HTREEITEM hChild;
	HTREEITEM hBlah;

	WINI::INode* pppp;
	int i, count;
	switch(lHint)
	{
	case 1:
		//this updates the collections tree
		//starts at 1 because 0th element is query's own metadata!
		count = child_node->CountChildren();
		for(i = 1; i < count; i++)
		{
			blah = child_node->GetChild(i);
			VerifyTreeTraversingUp(blah);
		}

		//add a handle on the tree for the actual query node
		pppp = child_node->GetParent();

		ASSERT(NULL != pppp);

		if(WINI_QUERY == pppp->GetType())
			m_ReverseMap.Lookup(pppp, hBlah);
		else
			hBlah = m_query_root;

		int type, image, selected_image;

		type = child_node->GetType();

		const char* szSub_type;

		if(WINI_SET == type)
			type = ((WINI::ISetNode*)child_node)->GetSetType();

		if(WINI_RESOURCE == type)
			szSub_type = ((WINI::IResourceNode*)child_node)->GetResourceType();

		GetImage(child_node, image, selected_image);
		hChild = tree.InsertItem(child_node->GetName(), image, selected_image, hBlah);
		tree.SetItemData(hChild, (DWORD)child_node);
		m_ReverseMap.SetAt(child_node, hChild);
	break;
	case 2:	//REFRESH FOR LVIEW REQUESTED
		if(WINI_QUERY != child_node->GetType())
			Repopulate(child_node);
		break;
	//case 3 is used above
	case 4: //REFRESH EVERYTHING
		tree.SetItemState(Repopulate(pDoc->GetRoot()), TVIS_EXPANDED, TVIS_EXPANDED);
		//need a list of Queries so that we can Repopulate them as well!
		break;
	case 5: //REFRESH FROM PARENT OF CHILD ON DOWN
		ASSERT(child_node->GetParent() != NULL);
		Repopulate(child_node->GetParent());
		break;
	case 6: //REFRESH EVERYTHING (FROM LOCAL TREE)
		tree.SetItemState(Repopulate(pDoc->GetLocalRoot()), TVIS_EXPANDED, TVIS_EXPANDED);
		//need a list of Queries so that we can Repopulate them as well!
		break;
	}

	m_ReverseMap.Lookup(child_node, hChild);

	tree.Select(hChild, TVGN_CARET);
}

void CLeftView::ShowHome()
{
	CInquisitorDoc* pDoc = GetDocument();

	WINI::INode* home = pDoc->GetHome();
	pDoc->SetCurrentNode(home);
	pDoc->UpdateAllViews(NULL);
	UpdateDialogBars(home);
}

void CLeftView::ShowParent()
{
	CInquisitorDoc* pDoc = GetDocument();

	WINI::INode* node = pDoc->GetCurrentNode();

	WINI::INode* parent = node->GetParent();

	if(parent)
	{
		pDoc->SetCurrentNode(parent);
		pDoc->UpdateAllViews(NULL);
		UpdateDialogBars(parent);
	}
}

void CLeftView::ShowName(const char* name)
{
	CInquisitorDoc* pDoc = GetDocument();

	WINI::INode* node = pDoc->GetCurrentNode();

	WINI::INode* ptr = node;

	while((ptr != NULL)&&(0 != strcmp(name, ptr->GetName())))
	{
		ptr = ptr->GetParent();
	}

	if(ptr)
	{
		pDoc->SetCurrentNode(ptr);
		pDoc->UpdateAllViews(NULL);
		UpdateDialogBars(ptr);
	}
}

const char* CLeftView::FindMetaValue(const char* attribute)
{
	CInquisitorDoc* pDoc = GetDocument();

	WINI::INode* node = pDoc->GetCurrentNode();

	int count = node->CountChildren();

	WINI::INode* child;
	WINI::IMetadataNode* childmeta;
	int i;
	for(i = 0; i < count; i++)
	{
		child = node->GetChild(i);

		if(WINI_METADATA == child->GetType())
		{
			childmeta = (WINI::IMetadataNode*) child;
			if(0 == strcmp(childmeta->GetAttribute(), attribute))
			{
				return childmeta->GetValue();
			}
		}
	}

	return NULL;
}


void CLeftView::UpdateDialogBars(WINI::INode* child_node)
{
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();

	frame->ClearStack(false);

	WINI::INode* ptr = child_node;

	int i = 0;
	char* szptr;

	int type = ptr->GetType();

	while(type != WINI_SET && type != WINI_ZONE)
	{
		szptr = (char*)ptr->GetName();
		frame->Push(szptr);
		i++;
		ptr = ptr->GetParent();
		if(ptr)
			type = ptr->GetType();
		else
			type = WINI_SET;	//hack makes WINI_SET test for NULL as well.
	}

	frame->SetStack(i-1);
}

void CLeftView::OnRename() 
{


	CTreeCtrl& tree = GetTreeCtrl();

	HTREEITEM selection = tree.GetSelectedItem();
	WINI::INode* node = (WINI::INode*)tree.GetItemData(selection);

	CInquisitorDoc* pDoc = GetDocument();

	if(!pDoc->SuspendWorkThread())
	{
		MessageBox("Couldn't Suspend Thread");

		return;
	}

	char* new_name;
	CGenericDialog myDlg("L");

	switch(node->GetType())
	{
	case WINI_DATASET:
	case WINI_COLLECTION:
		if(IDOK == myDlg.DoModal())
		{
			new_name = myDlg.m_Edit.LockBuffer();
			gCount = 1;
			pDoc->Rename(node, new_name);
			myDlg.m_Edit.UnlockBuffer();
		}
		break;
	default:
		AfxMessageBox("Unsupported type");
	}



	if(!pDoc->ResumeWorkThread())
	{
		MessageBox("Couldn't resume Thread");
	}
}

void CLeftView::OnDelete() 
{


	CTreeCtrl& tree = GetTreeCtrl();

	HTREEITEM selection = tree.GetSelectedItem();
	WINI::INode* node = (WINI::INode*)tree.GetItemData(selection);

	CInquisitorDoc* pDoc = GetDocument();

	if(!pDoc->SuspendWorkThread())
	{
		MessageBox("Couldn't Suspend Thread");

		return;
	}

	if(IDNO == AfxMessageBox("Do you wish to delete the selected item(s)?", MB_YESNO))
	{

		return;
	}

	switch(node->GetType())
	{
	case WINI_DATASET:
	case WINI_COLLECTION:
	case WINI_METADATA:
		gCount = 1;
		pDoc->Delete(node);
		//pDoc->UpdateAllViews(NULL);
		break;
	default:
		AfxMessageBox("Unsupported type");
	}



	if(!pDoc->ResumeWorkThread())
	{
		MessageBox("Couldn't resume Thread");
	}
}

int CLeftView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if(!m_dropTarget.Register(this))
		MessageBox("couldn't register LView");

	if (CTreeView::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

void CLeftView::OnDownload() 
{


	CInquisitorDoc* pDoc = GetDocument();

	if(!pDoc->SuspendWorkThread())
	{
		MessageBox("Couldn't Suspend Thread");

		return;
	}

	char buffer[MAX_PATH];
	LPITEMIDLIST itemids;
	char* folder_path;
	LPMALLOC shallocator;

	BROWSEINFO bwinfo;
	FillMemory((void*)&bwinfo,sizeof(bwinfo),0);
	bwinfo.hwndOwner=NULL;
	bwinfo.pidlRoot=NULL;
	bwinfo.pszDisplayName=(char*)buffer;
	bwinfo.lpszTitle="Place downloads in folder:";
	bwinfo.ulFlags=0;
	itemids = SHBrowseForFolder(&bwinfo); 
	if(NULL == itemids)
		goto end;
	folder_path = new char[MAX_PATH];
	SHGetPathFromIDList(itemids,(char*) folder_path);
	SHGetMalloc(&shallocator);
	shallocator->Free((void*) itemids);
	shallocator->Release();

	gCount = 1;

	pDoc->Download(pDoc->GetCurrentNode(), folder_path, false);	//later we'll add something to chose true or false

end:


	if(!pDoc->ResumeWorkThread())
	{
		MessageBox("Couldn't Resume Thread");
	}

}
#if 0
LRESULT CLeftView::OnRefresh(WPARAM wParam, LPARAM lParam)
{
	//this should actually refresh the INode tree instead of just
	//updating all the views

	CInquisitorDoc* pDoc = GetDocument();

	pDoc->UpdateAllViews(NULL, lParam);

	return 0;
}
#endif
#if 0
WINI::StatusCode CLeftView::AddMetadataAttribute(const char* attribute)
{
	CInquisitorDoc* pDoc = GetDocument();

	return pDoc->AddMetadataAttribute(attribute);
}


WINI::StatusCode CLeftView::SetMetadataValue(const char* attribute, const char* value)
{
	CInquisitorDoc* pDoc = GetDocument();

	return pDoc->SetMetadataValue(attribute, value);
}
#endif

void CLeftView::OnUpload() 
{


	CInquisitorDoc* pDoc = GetDocument();

	if(!pDoc->SuspendWorkThread())
	{
		MessageBox("Couldn't suspend work thread");

		return;
	}

	WINI::INode* node = pDoc->GetCurrentNode();

	POSITION POS;
	int st;
	CString FileName;


	//CFileDialog FileDlg(TRUE, NULL, NULL,OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_SHOWHELP | OFN_NODEREFERENCELINKS , NULL, NULL);

	CFileDialog FileDlg(TRUE, NULL, NULL,  OFN_NOVALIDATE | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST, NULL, NULL);

	//FileDlg.m_ofn.Flags &= ~(OFN_EXPLORER | OFN_SHOWHELP);

	FileDlg.m_ofn.lpstrTitle = "Upload";

	st = FileDlg.DoModal();

	if(st != IDOK)
	{
		if(IDCANCEL != st)
		{
			MessageBox("Error Occured");

		}

		return;
	}

	POS = FileDlg.GetStartPosition();
	gCount = 0;
	while(POS != NULL)
	{
		FileName = FileDlg.GetNextPathName(POS);
		gCount++;
		//definitely we cannot resume the thread until we're done!
		//otherwise the call will have a different (and wrong) gCount each time!
		pDoc->Upload(node, FileName.GetBuffer(MAX_PATH));
	}



	if(!pDoc->ResumeWorkThread())
	{
		MessageBox("Couldn't Resume Thread");
	}

}

#if 0
	CInquisitorDoc* pDoc = GetDocument();

	if(!pDoc->SuspendWorkThread())
	{
		MessageBox("Couldn't suspend work thread");

		return;
	}

	WINI::INode* node = pDoc->GetCurrentNode();

	POSITION POS;
	int st;
	CString FileName;


	CFileDialog FileDlg(TRUE, NULL, NULL,  OFN_NOVALIDATE | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST, NULL, NULL);


	FileDlg.m_ofn.lpstrTitle = "Upload";

	st = FileDlg.DoModal();

	if(st != IDOK)
	{
		if(IDCANCEL != st)
		{
			MessageBox("Error Occured");

		}

		return;
	}

	POS = FileDlg.GetStartPosition();
	gCount = 0;
	while(POS != NULL)
	{
		FileName = FileDlg.GetNextPathName(POS);
		gCount++;
		//definitely we cannot resume the thread until we're done!
		//otherwise the call will have a different (and wrong) gCount each time!
		pDoc->Upload(node, FileName.GetBuffer(MAX_PATH));
	}



	if(!pDoc->ResumeWorkThread())
	{
		MessageBox("Couldn't Resume Thread");
	}
#endif

void CLeftView::OnUploadFolder() 
{
	BROWSEINFO bwinfo;
	char buffer[MAX_PATH];
	LPITEMIDLIST itemids;
	char* folder_path;
	LPMALLOC shallocator;
	CInquisitorDoc* pDoc;
	WINI::INode* node;
	POSITION POS;
	int i;

	gCount = 1;

	FillMemory((void*)&bwinfo,sizeof(bwinfo),0);
	bwinfo.hwndOwner=NULL;
	bwinfo.pidlRoot=NULL;
	bwinfo.pszDisplayName=(char*) buffer;
	bwinfo.lpszTitle="Place downloads in folder:";
	bwinfo.ulFlags=0;
	itemids = SHBrowseForFolder(&bwinfo);
	if(itemids == NULL)
	{
		return;
	}

	folder_path = new char[MAX_PATH];
	SHGetPathFromIDList(itemids,(char*) folder_path);

	shallocator;
	SHGetMalloc(&shallocator);
	shallocator->Free((void*) itemids);
	shallocator->Release();

	pDoc = GetDocument();

	node = pDoc->GetCurrentNode();

	pDoc->Upload(node, folder_path);
}

void CLeftView::OnNewCollection()
{
#if 1


	CInquisitorDoc* pDoc = GetDocument();

	if(!pDoc->SuspendWorkThread())
	{
		MessageBox("Couldn't suspend work thread");
	
		return;
	}

	WINI::INode* current_node = pDoc->GetCurrentNode();

	if(WINI_COLLECTION != current_node->GetType())
	{
		AfxMessageBox("Collections can only be created within another collection.");
	
		return;
	}

	CGenericDialog myDlg("New Collection");

	int a = myDlg.DoModal();

	if(a == IDCANCEL)
	{

		return;
	}

	if(a != IDOK)
		AfxMessageBox("Dialog Error!");


	char* szName = myDlg.m_Edit.LockBuffer();

	//add new child to current collection

	gCount = 1;

	pDoc->CreateNode(current_node, szName);

	myDlg.m_Edit.UnlockBuffer();



	if(!pDoc->ResumeWorkThread())
	{
		MessageBox("Couldn't resume thread");
	}
#endif
}

void CLeftView::OnNewContainer() 
{


	CInquisitorDoc* pDoc = GetDocument();

	if(!pDoc->SuspendWorkThread())
	{
		MessageBox("Couldn't suspend work thread");

		return;
	}

	WINI::INode* current_node = pDoc->GetCurrentNode();

	if(WINI_RESOURCE != current_node->GetType())
	{
		AfxMessageBox("Containers can only be created within a resource.");

		return;
	}

	CGenericDialog myDlg("New Container");

	int a = myDlg.DoModal();

	if(a == IDCANCEL)
	{
	
		return;
	}

	if(a != IDOK)
		AfxMessageBox("Dialog Error!");


	char* szName = myDlg.m_Edit.LockBuffer();

	//add new child to current collection
	gCount = 1;

	pDoc->CreateNode(current_node, szName);

	myDlg.m_Edit.UnlockBuffer();



	if(!pDoc->ResumeWorkThread())
	{
		MessageBox("Couldn't resume thread");
	}
}

void CLeftView::OnReplicate() 
{
#if 1
	CInquisitorDoc* pDoc = GetDocument();
	WINI::INode* current_node = pDoc->GetCurrentNode();

	if(WINI_DATASET == current_node->GetType())
	{
		gCount = 1;
		pDoc->Replicate(current_node);
	}

	if(WINI_COLLECTION == current_node->GetType())
	{
		gCount = 1;
		pDoc->Replicate(current_node);
	}
#endif
#if 0
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	int i;
	for(i = 0; i < 60; i++)
	{
		frame->PostMessage(msgProgressRotate, NULL, NULL);
	}
#endif

}

void CLeftView::OnMetadata() 
{
}

//
// DRAG AND DROP / CLIPBOARD SECTION
//
//extern gDragDropList;
void CLeftView::OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	FORMATETC fe = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	FORMATETC fe2= {gInqClipboardFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

	CharlieSource* pDataSource = new CharlieSource;
	pDataSource->m_doc = GetDocument();

	//pDataSource->CacheData(gInqClipboardFormat, &fe2);
	STGMEDIUM stgmedium;

	stgmedium.tymed = TYMED_HGLOBAL;
	stgmedium.hGlobal = NULL;

	pDataSource->CacheData(gInqClipboardFormat, &stgmedium);
	pDataSource->DelayRenderData(CF_HDROP, &fe);

	CTreeCtrl& tree = GetTreeCtrl();

	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);
	HTREEITEM blah = tree.HitTest(pt);
	ASSERT(blah != NULL);
	WINI::INode* node = (WINI::INode*)tree.GetItemData(blah);
	ASSERT(node != NULL);

	gNodeDragList.clear();
	gOn = 0;
	gNodeDragList.push_back(node);

	g_bIsDragging = true;
	DROPEFFECT dropEffect = pDataSource->DoDragDrop(DROPEFFECT_COPY | DROPEFFECT_MOVE);

	*pResult = 0;
}

DROPEFFECT CLeftView::OnDragEnter( COleDataObject* pDataObject, DWORD dwKeyState, CPoint point )
{
	return OnDragOver(pDataObject, dwKeyState, point);
}

DROPEFFECT CLeftView::OnDragOver( COleDataObject* pDataObject, DWORD dwKeyState, CPoint point )
{
	CTreeCtrl& tree = GetTreeCtrl();

	HTREEITEM blah = tree.HitTest(point);

	if(NULL == blah)
		return DROPEFFECT_NONE;

	WINI::INode* node = (WINI::INode*)tree.GetItemData(blah);

	if(NULL == node)
	{
		ASSERT(1);
		return DROPEFFECT_NONE;
	}

	if(WINI_COLLECTION != node->GetType())
		return DROPEFFECT_NONE;

	m_bDontChange = true;

	if(pDataObject->IsDataAvailable(gInqClipboardFormat))
	{
	//	ASSERT(g_bIsDragging);	//if bool is true, then inQ is the program that's the source of the drag
		tree.SelectDropTarget(blah);
		return DROPEFFECT_MOVE;
	}

	if(pDataObject->IsDataAvailable(CF_HDROP))
	{
		tree.SelectDropTarget(blah);	//assume all cfhdrops originate from outside of inq
		return DROPEFFECT_COPY;
	}

	return DROPEFFECT_NONE;	//dragging something into inq that's not a file or inqtype
}

//later we should consider dropEffect, not simply imply copy or move
BOOL CLeftView::OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
{
	g_bIsDragging = false;

	m_bDontChange = false;

	CTreeCtrl& tree = GetTreeCtrl();

	HTREEITEM selected_item = tree.HitTest(point);

	WINI::INode* selected_node;
	
	selected_node = (WINI::INode*)tree.GetItemData(selected_item);

	CInquisitorDoc* pDoc = GetDocument();

	STGMEDIUM item;
	HDROP DropHandle;
	UINT quantity;
	char buf[MAX_PATH];

	BOOL nRet = FALSE;

	int count;
	WINI::INode* dragged_node;

	if(pDataObject->IsDataAvailable(gInqClipboardFormat))
	{
		count = gNodeDragList.size();
		gCount = count;
		ASSERT(DROPEFFECT_MOVE == dropEffect);
	
		for(int i = 0; i < count; i++)
		{
			dragged_node = gNodeDragList.at(i);

			if(NULL == dragged_node)
				continue;

			pDoc->Copy(selected_node, dragged_node, true);
		}

		gNodeDragList.clear();
		gOn = 0;
	}
	else if(pDataObject->IsDataAvailable(CF_HDROP))
	{
		ASSERT(DROPEFFECT_COPY);
		tree.SelectItem(selected_item);

		if(pDataObject->GetData(CF_HDROP, &item))
		{
			DropHandle = (HDROP)item.hGlobal;
			quantity = DragQueryFile(DropHandle, 0xFFFFFFFF, NULL, 0);
			gCount = quantity;
			for(UINT i = 0; i < quantity; i++)
			{
				DragQueryFile(DropHandle, i, buf, MAX_PATH);
				pDoc->Upload(selected_node, buf);
			}

			nRet = TRUE;
		}

	}else
	{
		ASSERT(1);
	}

	return nRet;
}

void CLeftView::OnDragLeave()
{
	POINT pt;
	GetCursorPos(&pt);
	CRect rect;
	CWnd* wnd = AfxGetMainWnd();
	wnd->GetClientRect(&rect);

//	if(!rect.PtInRect(pt))			what if it comes back inside after it leaves?
//		g_bIsDragging = false;		let DataSource::OnRender handle it

	CTreeCtrl& tree = GetTreeCtrl();

	HTREEITEM hitem = tree.GetDropHilightItem();

	if(hitem != NULL)
		tree.SelectDropTarget(NULL);

	m_bDontChange = false;		//if it wasn't already
}

void CLeftView::OnMouseMove(UINT nFlags, CPoint point) 
{
#if 0
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);
	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM blah = tree.HitTest(pt);
	if(blah == m_collection_root)
	{
		::SendMessage((AfxGetMainWnd())->m_hWnd,	WM_LBUTTONDBLCLK, NULL, 0);
	}
#endif
	CTreeView::OnMouseMove(nFlags, point);
}

void CLeftView::OnEditCopy()
{
	gDS = new CharlieSource;
	gDS->m_doc = GetDocument();

	FORMATETC fe = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	FORMATETC fe2= {gInqClipboardFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

	CTreeCtrl& tree = GetTreeCtrl();

	HTREEITEM hItem = tree.GetSelectedItem();

	WINI::INode* node = (WINI::INode*)tree.GetItemData(hItem);

	int type = node->GetType();

	ASSERT(WINI_COLLECTION == type || WINI_DATASET == type);

	//5 nulls, 1 four letter type, and 1 one letter Y or N
	//add n bytes for size of an int. (replication index) + n bytes for another int
	//(terminator is no longer a null after the final null terminated string)
	//(terminator is now a null string after a final sizeof(int) sized block (yes it's waste).
	int blah = 10 + strlen(node->GetName()) + strlen(node->GetPath()) + 2 * sizeof(int);

	HANDLE handle = ::GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, blah * sizeof(char));

	void* ptr = ::GlobalLock(handle);
	int* repl = (int*)ptr;
	char* szptr = (char*)ptr + sizeof(int);

	if(WINI_COLLECTION == type)
	{
		*repl = 0;	//no replication value for a collection
		sprintf(szptr, "COLL");
	}
	else
	{
		*repl = atoi(((WINI::IDatasetNode*)node)->GetReplicantNumber());
		sprintf(szptr, "DATA");
	}

	szptr += 5;

	sprintf(szptr, "N");
	szptr += 2;

	sprintf(szptr, "%s", node->GetName());
	szptr += strlen(node->GetName()) + 1;

	sprintf(szptr, "%s", node->GetPath());
	szptr += strlen(node->GetPath()) + 1;

	//final 4 bytes for integer and last 1 byte for null string is already there( memory initialized with zeroes).

	::GlobalUnlock(handle);

	gDS->CacheGlobalData(gInqClipboardFormat, handle);
	ghDragList = handle;
	gDS->DelayRenderData(CF_HDROP, &fe);
	gDS->SetClipboard();
}

void CLeftView::OnEditCut() 
{
	gDS = new CharlieSource;
	gDS->m_doc = GetDocument();

	FORMATETC fe = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	FORMATETC fe2= {gInqClipboardFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

	CTreeCtrl& tree = GetTreeCtrl();

	HTREEITEM hItem = tree.GetSelectedItem();

	WINI::INode* node = (WINI::INode*)tree.GetItemData(hItem);

	int type = node->GetType();

	ASSERT(WINI_COLLECTION == type || WINI_DATASET == type);

	//5 nulls, 1 four letter type, and 1 one letter Y or N
	//add n bytes for size of an int. (replication index) + n bytes for another int
	//(terminator is no longer a null after the final null terminated string)
	//(terminator is now a null string after a final sizeof(int) sized block (yes it's waste).
	int blah = 10 + strlen(node->GetName()) + strlen(node->GetPath()) + 2 * sizeof(int);

	HANDLE handle = ::GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, blah * sizeof(char));

	void* ptr = ::GlobalLock(handle);
	int* repl = (int*)ptr;
	char* szptr = (char*)ptr + sizeof(int);

	if(WINI_COLLECTION == type)
	{
		*repl = 0;	//no replication value for a collection
		sprintf(szptr, "COLL");
	}
	else
	{
		*repl = atoi(((WINI::IDatasetNode*)node)->GetReplicantNumber());
		sprintf(szptr, "DATA");
	}

	szptr += 5;

	sprintf(szptr, "Y");
	szptr += 2;

	sprintf(szptr, "%s", node->GetName());
	szptr += strlen(node->GetName()) + 1;

	sprintf(szptr, "%s", node->GetPath());
	szptr += strlen(node->GetPath()) + 1;

	//final 4 bytes for integer and last 1 byte for null string is already there( memory initialized with zeroes).

	::GlobalUnlock(handle);

	gDS->CacheGlobalData(gInqClipboardFormat, handle);
	ghDragList = handle;
	gDS->DelayRenderData(CF_HDROP, &fe);
	gDS->SetClipboard();
}

void CLeftView::OnEditPaste() 
{
	FORMATETC fe = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	FORMATETC fe2= {gInqClipboardFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

	COleDataObject odo;

	odo.AttachClipboard();

	CInquisitorDoc* pDoc = GetDocument();

	if(odo.IsDataAvailable(gInqClipboardFormat, &fe2))
	{
		CInquisitorDoc* pDoc = GetDocument();

		CTreeCtrl& tree = GetTreeCtrl();

		HTREEITEM selection = tree.GetSelectedItem();

		ASSERT(selection != NULL);

		WINI::INode* node = (WINI::INode*)tree.GetItemData(selection);

		ASSERT(node != NULL);

		HGLOBAL handle = odo.GetGlobalData(gInqClipboardFormat, &fe2);

		ASSERT(handle != NULL);

		void* ptr = ::GlobalLock(handle);
		int* repl = (int*)ptr;
		char* szptr = (char*)ptr + sizeof(int);

		int len;

		int type;
		bool cut;
		char* path;
		char* name;

		gCount = 0;

		do
		{
			szptr += strlen(szptr) + 1;
			ASSERT(szptr != NULL);
			szptr += strlen(szptr) + 1;
			ASSERT(szptr != NULL);
			szptr += strlen(szptr) + 1;
			ASSERT(szptr != NULL);
			szptr += strlen(szptr) + 1;
			szptr = szptr + sizeof(int);
			gCount++;
		} while(szptr[0] != '\0');

		szptr = (char*)ptr + sizeof(int);

		do
		{
			if('C' == szptr[0])
				type = WINI_COLLECTION;
			else
				type = WINI_DATASET;

			len = strlen(szptr);
			szptr += len + 1;
			ASSERT(szptr != NULL);

			if('Y' == szptr[0])
				cut = true;
			else
				cut = false;

			len = strlen(szptr);
			szptr += len + 1;
			ASSERT(szptr != NULL);

			name = szptr;

			len = strlen(szptr);
			szptr += len + 1;
			ASSERT(szptr != NULL);

			path = szptr;

			pDoc->Copy(node, type, cut, name, path, *repl);	//doc's copy func should copy these string values 'cause we'll clear the global memory

			len = strlen(szptr);
			szptr += len + 1;

			ptr = szptr;
			repl = (int*)ptr;
			szptr = (char*)ptr + sizeof(int);

		} while(szptr[0] != '\0');

		::GlobalUnlock(handle);
		::GlobalFree(handle);
	}
	else if(odo.IsDataAvailable(CF_HDROP, &fe))
	{
		CInquisitorDoc* pDoc = GetDocument();

		CTreeCtrl& tree = GetTreeCtrl();

		HTREEITEM selection = tree.GetSelectedItem();

		ASSERT(selection != NULL);

		WINI::INode* node = (WINI::INode*)tree.GetItemData(selection);

		ASSERT(node != NULL);

		HDROP handle = (HDROP)odo.GetGlobalData(CF_HDROP, &fe);

		ASSERT(handle != NULL);

		int count = ::DragQueryFile(handle, (UINT) -1, NULL, 0);

		if(count)
		{
			gCount = count;
			TCHAR szFile[MAX_PATH];
			for(int i = 0; i < count; i++)
			{
				::DragQueryFile(handle, i, szFile, sizeof(szFile)/sizeof(TCHAR));
				pDoc->Upload(node, szFile);
			}
		}

		::GlobalUnlock(handle);
		::GlobalFree(handle);

	}else
	{
		ASSERT(1);
	}

}

//
// UPDATE SECTION
//

void CLeftView::OnUpdateReplicate(CCmdUI* pCmdUI) 
{
#if 1
	WINI::INode* current_node = GetDocument()->GetCurrentNode();
	if(NULL == current_node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}
	if(WINI_DATASET != current_node->GetType() && WINI_COLLECTION != current_node->GetType())
		pCmdUI->Enable(FALSE);		
#endif
}

void CLeftView::OnUpdateMetadata(CCmdUI* pCmdUI) 
{
	#if 0
	WINI::INode* current_node = GetDocument()->GetCurrentNode();

	if(NULL == current_node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	switch(current_node->GetType())
	{
	case WINI_DATASET:
	case WINI_COLLECTION:
		break;
	default:
		pCmdUI->Enable(FALSE);
	}		
	#endif
}

void CLeftView::OnUpdateSynchronize(CCmdUI* pCmdUI) 
{
	#if 0
	WINI::INode* current_node = GetDocument()->GetCurrentNode();

	if(NULL == current_node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	if(WINI_CONTAINER != current_node->GetType())
		pCmdUI->Enable(FALSE);	
	#endif
}

void CLeftView::OnUpdateNewContainer(CCmdUI* pCmdUI) 
{
	#if 0
	WINI::INode* Node = GetDocument()->GetCurrentNode();

	if(NULL == Node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	if(Node->GetType() != WINI_RESOURCE)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	if(NULL == Node->GetParent())
	{
		pCmdUI->Enable(FALSE);
		return;
	}
	#endif
}

void CLeftView::OnUpdateQuery(CCmdUI* pCmdUI) 
{
#if 0
	WINI::INode* Node = GetDocument()->GetCurrentNode();

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
#endif
}

void CLeftView::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
#if 0
	WINI::INode* node = GetDocument()->GetCurrentNode();

	if(NULL == node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	switch(node->GetType())
	{
	case WINI_DATASET:
		break;
	case WINI_COLLECTION:
		break;
	default:
		pCmdUI->Enable(FALSE);
	}
#endif
}

void CLeftView::OnUpdateEditCut(CCmdUI* pCmdUI) 
{
#if 0
	WINI::INode* node = GetDocument()->GetCurrentNode();

	if(NULL == node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	switch(node->GetType())
	{
	case WINI_DATASET:
		break;
	case WINI_COLLECTION:
		break;
	default:
		pCmdUI->Enable(FALSE);
	}
#endif
}

//note: code is written this way not to change pCmdUI->Enable
//unless absolutely have to. This eliminates on screen flicker
//in the button
void CLeftView::OnUpdateEditPaste(CCmdUI* pCmdUI) 
{
#if 0
	WINI::INode* node = GetDocument()->GetCurrentNode();

	if(NULL == node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	int node_type = node->GetType();

	COleDataObject odo;
	odo.AttachClipboard();

	if(odo.IsDataAvailable(CF_HDROP))
	{
		if(WINI_COLLECTION != node_type)
		{
			pCmdUI->Enable(FALSE);
		}		
	}else if(odo.IsDataAvailable(gInqClipboardFormat))
	{
		switch(node_type)
		{
		case WINI_COLLECTION:
		case WINI_DATASET:
		case WINI_METADATA:
			break;
		default:
			pCmdUI->Enable(FALSE);
		}
	}else
	{
		pCmdUI->Enable(FALSE);
	}

	odo.Detach();
#endif
}

void CLeftView::OnUpdateViewLargeicon(CCmdUI* pCmdUI) 
{
		pCmdUI->Enable(FALSE);
}

void CLeftView::OnUpdateViewList(CCmdUI* pCmdUI) 
{
		pCmdUI->Enable(FALSE);
}

void CLeftView::OnUpdateViewSmallicon(CCmdUI* pCmdUI) 
{
		pCmdUI->Enable(FALSE);
}

void CLeftView::OnUpdateViewDetails(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(FALSE);	
}

void CLeftView::OnUpdateUpload(CCmdUI* pCmdUI) 
{
#if 0
	WINI::INode* current_node = GetDocument()->GetCurrentNode();
	if(NULL == current_node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	if(WINI_COLLECTION != current_node->GetType())
	{
		pCmdUI->Enable(FALSE);
	}
#endif
}

void CLeftView::OnUpdateUploadFolder(CCmdUI* pCmdUI) 
{
#if 0
	WINI::INode* current_node = GetDocument()->GetCurrentNode();
	if(NULL == current_node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	if(WINI_COLLECTION != current_node->GetType())
	{
		pCmdUI->Enable(FALSE);
	}
#endif
}

void CLeftView::OnUpdateDownload(CCmdUI* pCmdUI) 
{
#if 0
	WINI::INode* current_node = GetDocument()->GetCurrentNode();
	if(NULL == current_node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	switch(current_node->GetType())
	{
	case WINI_COLLECTION:
	case WINI_DATASET:
		break;
	default:
		pCmdUI->Enable(FALSE);
	}
#endif
}

void CLeftView::OnUpdateNewCollection(CCmdUI* pCmdUI) 
{
	WINI::INode* current_node = GetDocument()->GetCurrentNode();

	if(NULL == current_node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	if(WINI_COLLECTION != current_node->GetType())
		pCmdUI->Enable(FALSE);	
}

void CLeftView::OnUpdateAccessCtrl(CCmdUI* pCmdUI) 
{
#if 0
	CInquisitorDoc* pDoc = GetDocument();

	WINI::INode* current_node = pDoc->GetCurrentNode();

	if(NULL == current_node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	switch(current_node->GetType())
	{
	case WINI_COLLECTION:
		if(current_node == pDoc->GetRoot())
			pCmdUI->Enable(FALSE);	
		return;
	case WINI_DATASET:
		return;
	default:
		pCmdUI->Enable(FALSE);
		return;
	}
#endif
}


void CLeftView::OnUpdateDelete(CCmdUI* pCmdUI) 
{
	WINI::INode* node = GetDocument()->GetCurrentNode();

	if(node == NULL)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	switch(node->GetType())
	{
	case WINI_COLLECTION:
	case WINI_DATASET:
	case WINI_METADATA:
		break;
	default:
		pCmdUI->Enable(FALSE);
		break;
	}
}

void CLeftView::OnUpdateRename(CCmdUI* pCmdUI) 
{
	WINI::INode* node = GetDocument()->GetCurrentNode();

	if(node == NULL)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	if(WINI_DATASET != node->GetType())
		pCmdUI->Enable(FALSE);
}

void CLeftView::OnComment() 
{
	WINI::INode* node = GetDocument()->GetCurrentNode();

	ASSERT(NULL != node);

	CGenericDialog myDlg("L");

	if(IDOK == myDlg.DoModal())
	{
		gCount = 1;
		GetDocument()->SetComment(node, myDlg.m_Edit.LockBuffer());
		myDlg.m_Edit.UnlockBuffer();
	}
}

void CLeftView::OnUpdateComment(CCmdUI* pCmdUI) 
{
	WINI::INode* node = GetDocument()->GetCurrentNode();

	if(NULL == node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	if(WINI_DATASET != node->GetType())
		pCmdUI->Enable(FALSE);
}

void CLeftView::OnBeginlabeledit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TV_DISPINFO* pTVDispInfo = (TV_DISPINFO*)pNMHDR;

	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hItem = tree.GetSelectedItem();

	WINI::INode* node = (WINI::INode*)tree.GetItemData(hItem);

	if(NULL == node)
		*pResult = 0;
		

	if(node->GetType() == WINI_DATASET || node->GetType() == WINI_COLLECTION)
		*pResult = 0;
	else
		*pResult = 1;

}

void CLeftView::OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TV_DISPINFO* pTVDispInfo = (TV_DISPINFO*)pNMHDR;
	
	TVITEM* tv = &pTVDispInfo->item;

	if(NULL == tv->pszText)
	{
		//for TVN_ENDLABELEDIT, a NULL in pszText means the edit was cancelled.
		//GetTreeCtrl().EditLabel(pTVDispInfo->item.hItem);
		*pResult = 1;
		return;
	}

	/*if string is not acceptable, print a suitable dialog box, and then use this code to reset the dialog (just like explorer)*/
	/*it looks like the string stays the edited value by default so there's no setting...*/
	if(0 == strcmp(tv->pszText, ""))
	{
		AfxMessageBox("That is an invalid name.", MB_OK);
		GetTreeCtrl().EditLabel(pTVDispInfo->item.hItem);
		*pResult = 0;	/*returning this value tells it it's no good, but I guess you have to use EditLabel to reset it to original anyway*/
		return;
	}

	CTreeCtrl& tree = GetTreeCtrl();
	HTREEITEM hItem = tree.GetSelectedItem();
	WINI::INode* node = (WINI::INode*)tree.GetItemData(pTVDispInfo->item.hItem);
	
	*pResult = 1;


	gCount = 1;

	CInquisitorDoc* pDoc = GetDocument();

	if(!pDoc->SuspendWorkThread())
	{
		MessageBox("Couldn't Suspend Thread");
	
		return;
	}

	pDoc->Rename(node, tv->pszText);


	if(!pDoc->ResumeWorkThread())
	{
		MessageBox("Couldn't resume Thread");
	}
}

void CLeftView::OnNewMeta() 
{
	CMainFrame* ptr = ((CMainFrame*)AfxGetMainWnd());
	ptr->SetFocusMeta();
}

void CLeftView::OnUpdateNewMeta(CCmdUI* pCmdUI) 
{
	WINI::INode* node = GetDocument()->GetCurrentNode();

	if(NULL == node)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	int type = node->GetType();

	if(WINI_DATASET != type && WINI_COLLECTION != type)
		pCmdUI->Enable(FALSE);
}
