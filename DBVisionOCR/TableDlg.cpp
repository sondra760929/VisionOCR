// TableDlg.cpp: 구현 파일
//

#include "stdafx.h"
#include "TableDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "AutoSplitPagesView.h"
// CTableDlg 대화 상자

IMPLEMENT_DYNAMIC(CTableDlg, CDialog)

CTableDlg::CTableDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIALOG_TABLE, pParent)
{

}

CTableDlg::~CTableDlg()
{
}

void CTableDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CTableDlg, CDialog)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CTableDlg 메시지 처리기


BOOL CTableDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_Grid.AttachGrid(this, IDC_GRID);
	//m_iArrowIndex = m_Grid.AddCellType(&m_sortArrow);
	m_Grid.m_pParentView = this;
	m_Grid.SetCurrentCellMode(2);

	m_Grid.SetVScrollMode(UG_SCROLLTRACKING);
	m_Grid.SetHScrollMode(UG_SCROLLTRACKING);
	m_Grid.SetSH_ColWidth(-1, 0);
	m_Grid.SetMargin(10);

	m_Grid.SetNumberCols(9);
	m_Grid.SetNumberRows(0);

	m_Grid.SetMultiSelectMode(TRUE);
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

void CTableDlg::OnLClicked(MyCugEdit* grid, int col, long row, int updn)
{
	//if (updn == 0)
	//{
	//	int EnumNextBlock(int *startCol, long *startRow, int *endCol, long *endRow);
	//	int startCol, endCol;
	//	long startRow, endRow;
	//	if (m_Grid.EnumFirstBlock(&startCol, &startRow, &endCol, &endRow) == UG_SUCCESS)
	//	{
	//		if (startCol == endCol == 0)
	//		{
	//			if (startRow != endRow)
	//			{
	//				CString cell_string(m_Grid.QuickGetText(m_Grid.GetCurrentCol(), m_Grid.GetCurrentRow()));

	//				m_pCurrentView->OnGridAutoCheck(WPARAM(min(startRow, endRow)), LPARAM(max(startRow, endRow)));
	//				//auto_input_from = min(startRow, endRow);
	//				//auto_input_to = max(startRow, endRow);
	//				//PostMessage(WM_GRID_AUTO_CHECK, 0, 0);
	//				//CString temp_string;
	//				//CDlgAutoInput pDlg(cell_string, min(startRow, endRow), max(startRow, endRow), startCol);
	//				//if (pDlg.DoModal() == IDOK)
	//				//{

	//				//}
	//				//temp_string.Format(_T("%s 로 선택된 영역에 입력하시겠습니까?"), cell_string);
	//				//if (AfxMessageBox(temp_string, MB_YESNO) == IDYES)
	//				//{
	//				//	for (int i = min(startRow, endRow); i <= max(startRow, endRow); i++)
	//				//	{
	//				//		m_Grid.QuickSetText(startCol, i, cell_string);
	//				//	}
	//				//	m_Grid.RedrawAll();
	//				//}
	//			}
	//		}
	//	}
	//}
}

void CTableDlg::CellModified(MyCugEdit* grid, int col, long row, LPCTSTR string)
{
}

void CTableDlg::ResizeControl(int cx, int cy)
{
	int offset = 0;

	m_Grid.MoveWindow(offset, offset, cx - (offset * 2), cy - (offset * 2));
}


void CTableDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	ResizeControl(cx, cy);
}

void CTableDlg::OnDClicked(int col, long row)
{
	//if ((!IsNotUseExternalViewer) || (m_bIsAdmin < 3))
	//{
	//	if (row >= 0)
	//	{
	//		int col_size = m_Grid.GetNumberCols();
	//		CString file_path = StrDownloadFolder + "\\" + GetPathFromGridList(row);
	//		if (IsPDF(file_path))
	//		{
	//			ShellExecute(m_hWnd, _T("open"), file_path, nullptr, nullptr, SW_SHOW);
	//		}
	//	}
	//}
}

void CTableDlg::OnCellChange(int oldcol, int newcol, long oldrow, long newrow)
{
	if (m_pCurrentView)
	{
		m_pCurrentView->OnCellChange(oldcol, newcol, oldrow, newrow);
	}
		//if (m_pView)
		//{
		//	m_pView->OnCellChange(oldcol, newcol, oldrow, newrow);
		//}
		////int col_size = m_Grid.GetNumberCols();
		////CString file_path = StrDownloadFolder + "\\" + GetPathFromGridList(newrow);
		////if (PathFileExists(file_path))
		////{
		////	m_pFrame->m_wndProperties.DoPreview(file_path);
		////	m_Grid.SetFocus();
		////}
}


BOOL CTableDlg::PreTranslateMessage(MSG* pMsg)
{
	//if (pMsg->message == WM_KEYDOWN)
	//{
	//	if (m_pView)
	//	{
	//		if (pMsg->wParam == VK_F9)
	//		{
	//			m_pView->OnButtonProcess(); //code on F10
	//			return TRUE;
	//		}
	//		if (pMsg->wParam == VK_F5)
	//		{
	//			m_pView->OnButtonApply(); //code on F10
	//			return TRUE;
	//		}
	//		if (pMsg->wParam == VK_F3)
	//		{
	//			m_pView->SetImageRotateLeft();
	//			return TRUE;
	//		}
	//		if (pMsg->wParam == VK_F4)
	//		{
	//			m_pView->SetImageRotateRight();
	//			return TRUE;
	//		}
	//		if (pMsg->wParam == VK_F6)
	//		{
	//			m_pView->EraseCodeOne();
	//			return TRUE;
	//		}
	//		if (pMsg->wParam == VK_F7)
	//		{
	//			m_pView->EraseMaskOne();
	//			return TRUE;
	//		}
	//		if (pMsg->wParam == VK_F11)
	//		{
	//			m_pView->SetPrevImage();
	//			return TRUE;
	//		}
	//		if (pMsg->wParam == VK_F12)
	//		{
	//			m_pView->SetNextImage();
	//			return TRUE;
	//		}
	//	}
	//}
	return CDialog::PreTranslateMessage(pMsg);
}
