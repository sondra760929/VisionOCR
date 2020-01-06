#pragma once
#include "MyCUGEdit.h"


// CTableDlg 대화 상자

class CTableDlg : public CDialog
{
	DECLARE_DYNAMIC(CTableDlg)

public:
	CTableDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CTableDlg();

	MyCugEdit m_Grid;
	void OnLClicked(MyCugEdit* grid, int col, long row, int updn);
	void CellModified(MyCugEdit* grid, int col, long row, LPCTSTR string);
	void ResizeControl(int cx, int cy);
	void OnCellChange(int oldcol, int newcol, long oldrow, long newrow);
	void OnDClicked(int col, long row);

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_TABLE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
