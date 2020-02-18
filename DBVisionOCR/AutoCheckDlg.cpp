// AutoCheckDlg.cpp: 구현 파일
//

#include "stdafx.h"
#include "AutoCheckDlg.h"
#include "afxdialogex.h"
#include "resource.h"

// CAutoCheckDlg 대화 상자

IMPLEMENT_DYNAMIC(CAutoCheckDlg, CDialog)

CAutoCheckDlg::CAutoCheckDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIALOG_AUTO_CHECK, pParent)
	, m_iCheckType(0)
{

}

CAutoCheckDlg::~CAutoCheckDlg()
{
}

void CAutoCheckDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO1, m_iCheckType);
}


BEGIN_MESSAGE_MAP(CAutoCheckDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CAutoCheckDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CAutoCheckDlg 메시지 처리기


void CAutoCheckDlg::OnBnClickedOk()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CDialog::OnOK();
}
