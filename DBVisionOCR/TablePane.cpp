#include "stdafx.h"
#include "TablePane.h"
#include "resource.h"


CTablePane::CTablePane()
{
}


CTablePane::~CTablePane()
{
}
BEGIN_MESSAGE_MAP(CTablePane, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()


int CTablePane::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	BOOL bRet = m_Dialog.Create(IDD_DIALOG_TABLE, this);
	m_Dialog.ShowWindow(SW_SHOW);

	return 0;
}


void CTablePane::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);

	m_Dialog.MoveWindow(0, 0, cx, cy);
}


void CTablePane::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);
	m_Dialog.SetFocus();
}
