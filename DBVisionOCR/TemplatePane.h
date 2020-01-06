#pragma once
#include "CTemplateDlg.h"

class CTemplatePane :
	public CDockablePane
{
public:
	CTemplatePane() noexcept;
	~CTemplatePane();
	virtual BOOL OnShowControlBarMenu(CPoint pt);

	CTemplateDlg m_Dialog;

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);

	DECLARE_MESSAGE_MAP()
};

