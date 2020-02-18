#pragma once


// CAutoCheckDlg 대화 상자

class CAutoCheckDlg : public CDialog
{
	DECLARE_DYNAMIC(CAutoCheckDlg)

public:
	CAutoCheckDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CAutoCheckDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_AUTO_CHECK };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	int m_iCheckType;
	afx_msg void OnBnClickedOk();
};
