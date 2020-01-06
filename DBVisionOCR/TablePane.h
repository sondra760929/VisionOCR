#pragma once
#include <afxdockablepane.h>
#include "TableDlg.h"

class CTablePane :
	public CDockablePane
{
public:
	CTablePane();
	~CTablePane();

	CTableDlg m_Dialog;
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
};

