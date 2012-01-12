// Query.cpp : implementation file
//

#include "stdafx.h"
#include "Inquisitor.h"
#include "Query.h"
#include "winiGlobals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CQuery dialog


CQuery::CQuery(WINI::ISession* session, WINI::INode* source, CWnd* pParent /*=NULL*/)
	: CDialog(CQuery::IDD, pParent)
{
	//{{AFX_DATA_INIT(CQuery)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_session = session;
	m_source = source;
}


void CQuery::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CQuery)
	DDX_Control(pDX, IDC_QUERY_LIST, m_List);
	DDX_Control(pDX, IDC_QUERY_VALUE, m_Value);
	DDX_Control(pDX, IDC_QUERY_OPERATION, m_Operation);
	DDX_Control(pDX, IDC_QUERY_ATTRIBUTE, m_Attribute);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CQuery, CDialog)
	//{{AFX_MSG_MAP(CQuery)
	ON_BN_CLICKED(IDC_QUERY_ADD, OnQueryAdd)
	ON_BN_CLICKED(IDC_QUERY_REMOVE, OnQueryRemove)
	ON_BN_CLICKED(IDC_QUERY_AND, OnQueryAnd)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQuery message handlers
#if 0
	char buf[255];

	m_Attribute.GetWindowText(buf, 255);
#if 0
	WINI::IMetadataNode* node = m_qop->Add(buf);

	m_qop->SetOperation(node, WINI_MD_EQUAL);

	m_Value.GetWindowText(buf, 255);
	m_qop->SetValue(node, buf);

	m_List.AddString(node->GetName());
#endif
#if 0
	WINI::IMetadataNode* node;

	m_qop->Add(&node);

	m_qop->SetAttribute(node, buf);
	m_qop->SetOperation(node, WINI_MD_EQUAL);

	m_Value.GetWindowText(buf, 255);
	m_qop->SetValue(node, buf);

	m_qop->Attach(m_query, node);

	m_List.AddString(node->GetName());
#endif
#endif
void CQuery::OnQueryAdd() 
{
	static char buf[1024];

	WINI::IMetadataNode* node;
	m_qOp->Add(&node);

	m_Attribute.GetWindowText(buf, 1024);
	m_qOp->SetAttribute(node, buf);

	m_Operation.GetWindowText(buf, 1024);

	if(0 == strcmp(buf, "="))
		m_qOp->SetOperation(node, WINI_MD_EQUAL);
	else if(0 == strcmp(buf, "<>"))
		m_qOp->SetOperation(node, WINI_MD_NOT_EQUAL);
	else if(0 == strcmp(buf, ">"))
		m_qOp->SetOperation(node, WINI_MD_GREATER_THAN);
	else if(0 == strcmp(buf, "<"))
		m_qOp->SetOperation(node,WINI_MD_LESS_THAN);
	else if(0 == strcmp(buf, ">="))
		m_qOp->SetOperation(node,WINI_MD_GREATER_OR_EQUAL);
	else if(0 == strcmp(buf, "<="))
		m_qOp->SetOperation(node, WINI_MD_LESS_OR_EQUAL);
	else if(0 == strcmp(buf, "between"))
		m_qOp->SetOperation(node, WINI_MD_BETWEEN);
	else if(0 == strcmp(buf, "not between"))
		m_qOp->SetOperation(node, WINI_MD_NOT_BETWEEN);
	else if(0 == strcmp(buf, "like"))
		m_qOp->SetOperation(node, WINI_MD_LIKE);
	else if(0 == strcmp(buf, "not like"))
		m_qOp->SetOperation(node, WINI_MD_NOT_LIKE);
	else if(0 == strcmp(buf, "sounds like"))
		m_qOp->SetOperation(node, WINI_MD_SOUNDS_LIKE);
	else if(0 == strcmp(buf, "sounds not like"))
		m_qOp->SetOperation(node, WINI_MD_SOUNDS_NOT_LIKE);

	m_Value.GetWindowText(buf, 255);
	m_qOp->SetValue(node, buf);

	int i = m_List.AddString(node->GetName());

	m_List.SetItemData(i, (DWORD)node);
	//m_list.push_back(node);
}


void CQuery::Fill(WINI::INode* node)
{
	int count = node->CountChildren();
	WINI::INode* child;



	WINI::IMetadataNode* meta;

	for(int i = 0; i < count; i++)
	{
		child = node->GetChild(i);
		if(WINI_METADATA == child->GetType())
		{
			meta = (WINI::IMetadataNode*)child;
			m_Attribute.AddString(meta->GetAttribute());
			m_Value.AddString(meta->GetValue());
			Fill(child);
		}
	}
}


BOOL CQuery::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	m_Operation.AddString("=");
	m_Operation.AddString("<>");
	m_Operation.AddString(">");
	m_Operation.AddString("<");
	m_Operation.AddString(">=");
	m_Operation.AddString("<=");
//	m_Operation.AddString("between");
//	m_Operation.AddString("not between");
	m_Operation.AddString("like");
	m_Operation.AddString("not like");
	m_Operation.AddString("sounds like");
	m_Operation.AddString("sounds not like");

	Fill(m_source);

	CString blah;

	m_Operation.SetCurSel(0);

	if(0 < m_Attribute.GetCount())
	{
		m_Attribute.GetLBText(0, blah);
		m_Attribute.SetWindowText(blah);
	}

	if(0 < m_Value.GetCount())
	{
		m_Value.GetLBText(0, blah);
		m_Value.SetWindowText(blah);
	}

	m_qOp = (WINI::IQueryOperator*)m_session->GetOperator(WINI_QUERY);

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CQuery::OnOK() 
{
	// TODO: Add extra validation here
	int nCount = m_List.GetCount();
	
	switch(nCount)
	{
	case 0:
		AfxMessageBox("You have not entered a query");
		return;
	case 1:
		break;
	default:
		AfxMessageBox("You have entered more than one query. You must join them or remove all but one.");
		return;
	}

	WINI::IMetadataNode* blah = (WINI::IMetadataNode*)m_List.GetItemData(0);

	m_qOp->Create(&m_query);
	m_qOp->Attach(m_query, blah);
	m_qOp->Bind(m_query, m_source);
	//m_session->OpenTree(m_query, 1);

	CDialog::OnOK();

}

void CQuery::OnQueryRemove() 
{
	int nSelected = m_List.GetSelCount();

	if(nSelected == 0)
		return;
	
	int *pArray = new int[nSelected];
	m_List.GetSelItems(nSelected, pArray);

	WINI::IMetadataNode* result;
	for(int i = nSelected - 1; i > -1; i--)
	{
		result = (WINI::IMetadataNode*)m_List.GetItemData(pArray[i]);
		m_List.DeleteString(pArray[i]);

		if(NULL != result)
		{
			m_qOp->Remove(result);
		}
	}

	delete [] pArray;
}

void CQuery::OnQueryAnd() 
{
	int nSelected = m_List.GetSelCount();

	if(nSelected <= 1)
		return;
	
	int *pArray = new int[nSelected];
	m_List.GetSelItems(nSelected, pArray);

	WINI::IMetadataNode* left = (WINI::IMetadataNode*)m_List.GetItemData(pArray[0]);
	WINI::IMetadataNode* right = (WINI::IMetadataNode*)m_List.GetItemData(pArray[1]);
	WINI::IMetadataNode* result;

	m_qOp->And(left, right, &result);
	int i;
	for(i = 2; i < nSelected; i++)
	{
		right = (WINI::IMetadataNode*)m_List.GetItemData(pArray[i]);

		left = result;

		m_qOp->And(left, right, &result);
	}

	for(i = nSelected - 1; i > -1; i--)
	{
		m_List.DeleteString(pArray[i]);
	}
	
	delete [] pArray;

	int pos = m_List.AddString(result->GetName());
	m_List.SetItemData(pos, (DWORD)result);
}
