#pragma once


// CDlgRename 대화 상자

class CDlgRename : public CDialog
{
	DECLARE_DYNAMIC(CDlgRename)

public:
	CDlgRename(CString init_file_name, CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgRename();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_RENAME };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	CString m_strFileName;
};
