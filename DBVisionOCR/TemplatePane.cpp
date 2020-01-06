#include "stdafx.h"
#include "TemplatePane.h"
#include "resource.h"

CTemplatePane::CTemplatePane() noexcept
{
}


CTemplatePane::~CTemplatePane()
{
}

BEGIN_MESSAGE_MAP(CTemplatePane, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

int CTemplatePane::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	BOOL bRet = m_Dialog.Create(IDD_DIALOG_TEMPLATE, this);
	m_Dialog.ShowWindow(SW_SHOW);
	//CRect rectDummy;
	return 0;
}

void CTemplatePane::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	m_Dialog.MoveWindow(0, 0, cx, cy);
}

void CTemplatePane::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);
	m_Dialog.SetFocus();
}


BOOL CTemplatePane::OnShowControlBarMenu(CPoint pt)
{
	CRect rc;
	GetClientRect(&rc);
	ClientToScreen(&rc);
	if (rc.PtInRect(pt))
		return TRUE;//hide a pane contextmenu on client rea
	//show on caption bar
	return CDockablePane::OnShowControlBarMenu(pt);
}