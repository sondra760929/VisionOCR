// CDlgRename.cpp: 구현 파일
//

#include "stdafx.h"
#include "AutoSplitPages.h"
#include "DlgRename.h"
#include "afxdialogex.h"


// CDlgRename 대화 상자

IMPLEMENT_DYNAMIC(CDlgRename, CDialog)

CDlgRename::CDlgRename(CString init_file_name, CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIALOG_RENAME, pParent)
	, m_strFileName(init_file_name)
{

}

CDlgRename::~CDlgRename()
{
}

void CDlgRename::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_FILEANME, m_strFileName);
}


BEGIN_MESSAGE_MAP(CDlgRename, CDialog)
END_MESSAGE_MAP()


// CDlgRename 메시지 처리기
