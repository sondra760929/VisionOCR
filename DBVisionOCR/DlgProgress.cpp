// DlgProgress.cpp: 구현 파일
//
#include "stdafx.h"
#include "DlgProgress.h"
#include "afxdialogex.h"
#include "resource.h"

// CDlgProgress 대화 상자

IMPLEMENT_DYNAMIC(CDlgProgress, CDialog)

CDlgProgress::CDlgProgress(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIALOG_PROGRESS, pParent)
{

}

CDlgProgress::~CDlgProgress()
{
}

void CDlgProgress::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_STATUS, m_stStatus);
	DDX_Control(pDX, IDC_PROGRESS_STATUS, m_progressBar);
}


BEGIN_MESSAGE_MAP(CDlgProgress, CDialog)
END_MESSAGE_MAP()


// CDlgProgress 메시지 처리기
