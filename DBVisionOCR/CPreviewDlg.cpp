// CPreviewDlg.cpp: 구현 파일
//

#include "stdafx.h"
#include "AutoSplitPages.h"
#include "CPreviewDlg.h"
#include "afxdialogex.h"
#include "MainFrm.h"
#include "AutoSplitPagesView.h"
#include "resource.h"

// CPreviewDlg 대화 상자

IMPLEMENT_DYNAMIC(CPreviewDlg, CDialog)

CPreviewDlg::CPreviewDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIALOG_PREVIEW, pParent)
	, m_bLDown(false)
	, m_bRDown(false)
{

}

CPreviewDlg::~CPreviewDlg()
{
}

void CPreviewDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPreviewDlg, CDialog)
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_PAINT()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_COMMAND_RANGE(WM_USER + 1000, WM_USER + 1000 + 100, &(CPreviewDlg::SetBoxIndex))
END_MESSAGE_MAP()


// CPreviewDlg 메시지 처리기
void CPreviewDlg::ShowImage(CString file_path)
{
	CImage pngImage;
	CDC bmDC;
	CBitmap *pOldbmp;
	BITMAP  bi;
	UINT xPos = 450, yPos = 300;

	CClientDC dc(this);
	m_strFilePath = file_path;
	pngBmp.DeleteObject();
	if (!FAILED(pngImage.Load(file_path)))
	{
		// new code

		pngBmp.Attach(pngImage.Detach());
		BITMAP bm; //Create bitmap Handle to get dimensions
		pngBmp.GetBitmap(&bm); //Load bitmap into handle
		bitmapSize = CSize(bm.bmWidth, bm.bmHeight); // Get bitmap Sizes;
		//bmDC.CreateCompatibleDC(&dc);
	}
	Invalidate(1);
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	if (pFrame)
	{
		pFrame->m_wndTemplate1.m_Dialog.UpdateImages();
	}

	//pOldbmp = bmDC.SelectObject(&pngBmp);
	//pngBmp.GetBitmap(&bi);
	//dc.BitBlt(xPos, yPos, bi.bmWidth, bi.bmHeight, &bmDC, 0, 0, SRCCOPY);
	//bmDC.SelectObject(pOldbmp);
}

BOOL CPreviewDlg::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}


void CPreviewDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	Invalidate();
}


void CPreviewDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	ptFrom = point;
	rectSelect1_show.left = point.x;
	rectSelect1_show.top = point.y;
	rectSelect1_show.right = point.x;
	rectSelect1_show.bottom = point.y;
	m_bLDown = true;

	CDialog::OnLButtonDown(nFlags, point);
}


void CPreviewDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	ptEnd = point;
	rectSelect1_show.left = min(ptFrom.x, ptEnd.x);
	rectSelect1_show.right = max(ptFrom.x, ptEnd.x);
	rectSelect1_show.top = min(ptFrom.y, ptEnd.y);
	rectSelect1_show.bottom = max(ptFrom.y, ptEnd.y);
	m_bLDown = false;
	Invalidate();

	SaveTemplateFile1();
	CDialog::OnLButtonUp(nFlags, point);
}


void CPreviewDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bLDown)
	{
		ptEnd = point;
		rectSelect1_show.left = min(ptFrom.x, ptEnd.x);
		rectSelect1_show.right = max(ptFrom.x, ptEnd.x);
		rectSelect1_show.top = min(ptFrom.y, ptEnd.y);
		rectSelect1_show.bottom = max(ptFrom.y, ptEnd.y);

		Invalidate();
	}
	if (m_bRDown)
	{
		ptEnd = point;
		//rectSelect2_show.left = min(ptFrom.x, ptEnd.x);
		//rectSelect2_show.right = max(ptFrom.x, ptEnd.x);
		//rectSelect2_show.top = min(ptFrom.y, ptEnd.y);
		//rectSelect2_show.bottom = max(ptFrom.y, ptEnd.y);

		Invalidate();
	}

	CDialog::OnMouseMove(nFlags, point);
}

void CPreviewDlg::SaveTemplateFile1()
{
	long _left, _top, _width, _height;
	_left = (rectSelect1_show.left - source_left) * scale_ratio;
	_top = (rectSelect1_show.top - source_top) * scale_ratio;
	_width = rectSelect1_show.Width() * scale_ratio;
	_height = rectSelect1_show.Height() * scale_ratio;
	rectSelect1_image.SetRect(_left, _top, _left + _width, _top + _height);
	if (_width > 10 && _height > 10)
	{
		CString new_file_name;
		int index = 0;
		new_file_name.Format("temp%d.png", index);
		while (m_pCurrentView->m_mapCodeImageFile.find(new_file_name) != m_pCurrentView->m_mapCodeImageFile.end())
		{
			index++;
			new_file_name.Format("temp%d.png", index);
		}
		if (m_pCurrentView)
		{
			m_pCurrentView->SetCodeRect(rectSelect1_image, new_file_name);
		}

		if (_width > 10 && _height > 10)
		{
			WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("TEMP1_LEFT"), (DWORD)(int)_left);
			WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("TEMP1_RIGHT"), (DWORD)(int)_top);
			WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("TEMP1_WIDTH"), (DWORD)(int)_width);
			WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("TEMP1_HEIGHT"), (DWORD)(int)_height);

			CImage pngImage, dest_image;
			if (!FAILED(pngImage.Load(m_strFilePath)))
			{
				CClientDC dc(this);
				// new code

				//CBitmap temp_pngBmp;
				//temp_pngBmp.DeleteObject();
				//temp_pngBmp.Attach(pngImage.Detach());
				//BITMAP bm; //Create bitmap Handle to get dimensions
				//temp_pngBmp.GetBitmap(&bm); //Load bitmap into handle

				CDC* pMDC = new CDC;
				pMDC->CreateCompatibleDC(&dc);
				CBitmap* pb = new CBitmap;
				pb->CreateCompatibleBitmap(&dc, _width, _height);
				CBitmap* pob = pMDC->SelectObject(pb);
				pMDC->SetStretchBltMode(HALFTONE);

				pngImage.StretchBlt(pMDC->m_hDC, 0, 0, _width, _height, _left, _top, _width, _height, SRCCOPY);
				pMDC->SelectObject(pob);

				//BITMAP bm; //Create bitmap Handle to get dimensions
				//pb->GetBitmap(&bm); //Load bitmap into handle

				dest_image.Attach((HBITMAP)(*pb));

				//dest_image.Create(_width, _height, pngImage.GetBPP());

				//pngImage.StretchBlt(dest_image.GetDC(), 0, 0, _width, _height, _left, _top, _width, _height, SRCCOPY);

				dest_image.Save(m_strTempPath + new_file_name);
				//dest_image.ReleaseDC();

				CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
				if (pFrame)
				{
					pFrame->m_wndTemplate1.m_Dialog.ShowImage(m_strTempPath + new_file_name);
				}
			}
		}
	}
	else
	{
		//AfxMessageBox("Too small area selected.");
	}
}

CRect CPreviewDlg::GetImageRectFromWindow(CRect rect)
{
	long _left, _top, _width, _height;
	_left = (rect.left - source_left) * scale_ratio;
	_top = (rect.top - source_top) * scale_ratio;
	_width = rect.Width() * scale_ratio;
	_height = rect.Height() * scale_ratio;

	return CRect(_left, _top, _left + _width, _top + _height);
}

CRect CPreviewDlg::GetWindowRectFromImage(CRect rect)
{
	long _left, _top, _width, _height;
	_left = (rect.left / scale_ratio) + source_left;
	_top = (rect.top / scale_ratio) + source_top;
	_width = rect.Width() / scale_ratio;
	_height = rect.Height() / scale_ratio;

	return CRect(_left, _top, _left + _width, _top + _height);
}

void CPreviewDlg::SaveTemplateFile2()
{
	//long _left, _top, _width, _height;
	//CRect image_rect = GetImageRectFromWindow(rectSelect2_show);
	//_left = image_rect.left;
	//_top = image_rect.top;
	//_width = image_rect.Width();
	//_height = image_rect.Height();
	//rectSelect2_image.SetRect(_left, _top, _left + _width, _top + _height);

	////_left = (rectSelect2_show.left - source_left) * scale_ratio;
	////_top = (rectSelect2_show.top - source_top) * scale_ratio;
	////_width = rectSelect2_show.Width() * scale_ratio;
	////_height = rectSelect2_show.Height() * scale_ratio;
	////rectSelect2_image.SetRect(_left, _top, _left + _width, _top + _height);

	//if (m_pCurrentView)
	//{
	//	m_pCurrentView->SetMaskRect(rectSelect2_image);
	//}

	//if (_width > 10 && _height > 10)
	//{
	//	WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("TEMP2_LEFT"), (DWORD)(int)_left);
	//	WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("TEMP2_RIGHT"), (DWORD)(int)_top);
	//	WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("TEMP2_WIDTH"), (DWORD)(int)_width);
	//	WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("TEMP2_HEIGHT"), (DWORD)(int)_height);

	//	CImage pngImage, dest_image;
	//	if (!FAILED(pngImage.Load(m_strFilePath)))
	//	{
	//		CClientDC dc(this);
	//		// new code

	//		//CBitmap temp_pngBmp;
	//		//temp_pngBmp.DeleteObject();
	//		//temp_pngBmp.Attach(pngImage.Detach());
	//		//BITMAP bm; //Create bitmap Handle to get dimensions
	//		//temp_pngBmp.GetBitmap(&bm); //Load bitmap into handle

	//		CDC* pMDC = new CDC;
	//		pMDC->CreateCompatibleDC(&dc);
	//		CBitmap* pb = new CBitmap;
	//		pb->CreateCompatibleBitmap(&dc, _width, _height);
	//		CBitmap* pob = pMDC->SelectObject(pb);
	//		pMDC->SetStretchBltMode(HALFTONE);

	//		pngImage.StretchBlt(pMDC->m_hDC, 0, 0, _width, _height, _left, _top, _width, _height, SRCCOPY);
	//		pMDC->SelectObject(pob);

	//		//BITMAP bm; //Create bitmap Handle to get dimensions
	//		//pb->GetBitmap(&bm); //Load bitmap into handle

	//		dest_image.Attach((HBITMAP)(*pb));

	//		//dest_image.Create(_width, _height, pngImage.GetBPP());

	//		//pngImage.StretchBlt(dest_image.GetDC(), 0, 0, _width, _height, _left, _top, _width, _height, SRCCOPY);

	//		dest_image.Save(m_strTempPath + "temp2.png");
	//		//dest_image.ReleaseDC();

	//		CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	//		if (pFrame)
	//		{
	//			pFrame->m_wndTemplate2.m_Dialog.ShowImage(m_strTempPath + "temp2.png");
	//		}
	//	}
	//}
}

void CPreviewDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: 여기에 메시지 처리기 코드를 추가합니다.
					   // 그리기 메시지에 대해서는 CDialog::OnPaint()을(를) 호출하지 마십시오.
	
	CDC mDC;
	mDC.CreateCompatibleDC(&dc);

	CDC dcMemory;
	dcMemory.CreateCompatibleDC(&dc);
	CBitmap* pOldbitmap = dcMemory.SelectObject(&pngBmp);
	CRect rcClient;
	GetClientRect(&rcClient);

	CBitmap mbitmap;
	mbitmap.CreateCompatibleBitmap(&dc, rcClient.Width(), rcClient.Height());
	mDC.SelectObject(&mbitmap);


	const CSize& sbitmap = bitmapSize;
	//mDC.CreateCompatibleDC(&dc);
	CBrush brush;
	brush.CreateSolidBrush(RGB(210, 210, 210));

	//dc.FillRect(&rcClient, &brush);
	mDC.FillRect(&rcClient, &brush);
	//source_left, source_top, source_width, source_height;

	float ratio = (float)sbitmap.cx / (float)sbitmap.cy;
	float ratio1 = (float)rcClient.Width() / (float)rcClient.Height();
	if (ratio > ratio1)
	{
		source_width = rcClient.Width();
		source_height = (float)rcClient.Width() / ratio;
		source_top = (rcClient.Height() - source_height) / 2;
		source_left = 0;
	}
	else
	{
		source_width = (float)rcClient.Height() * ratio;
		source_height = rcClient.Height();
		source_left = (rcClient.Width() - source_width) / 2;
		source_top = 0;
	}

	scale_ratio = (float)sbitmap.cx / (float)source_width;
	mDC.StretchBlt(source_left, source_top, source_width, source_height, &dcMemory, 0, 0, sbitmap.cx, sbitmap.cy, SRCCOPY);

	//if (!m_bLDown)
	//{
	//	long _left, _top, _width, _height;
	//	_left = (rectSelect1_image.left / scale_ratio) + source_left;
	//	_top = (rectSelect1_image.top / scale_ratio) + source_top;
	//	_width = rectSelect1_image.Width() / scale_ratio;
	//	_height = rectSelect1_image.Height() / scale_ratio;
	//	rectSelect1_show.SetRect(_left, _top, _left + _width, _top + _height);
	//}

	//if(!m_bRDown)
	//{
	//	long _left, _top, _width, _height;
	//	_left = (rectSelect2_image.left / scale_ratio) + source_left;
	//	_top = (rectSelect2_image.top / scale_ratio) + source_top;
	//	_width = rectSelect2_image.Width() / scale_ratio;
	//	_height = rectSelect2_image.Height() / scale_ratio;
	//	rectSelect2_show.SetRect(_left, _top, _left + _width, _top + _height);
	//}

	CPen qCirclePen1(PS_SOLID, 2, m_color1);
	CPen* pqOrigPen = mDC.SelectObject(&qCirclePen1);
	long _left, _top, _width, _height;
	CString no_string;
	if (m_pCurrentView)
	{
		if (m_pCurrentView->m_bMultiCode)
		{
			if (m_pCurrentView->m_arrRectCode.size() > m_pCurrentView->m_iCurrentFileIndex)
			{
				for (int i = 0; i < m_pCurrentView->m_arrRectCode[m_pCurrentView->m_iCurrentFileIndex].size(); i++)
				{
					_left = (m_pCurrentView->m_arrRectCode[m_pCurrentView->m_iCurrentFileIndex][i].first.left / scale_ratio) + source_left;
					_top = (m_pCurrentView->m_arrRectCode[m_pCurrentView->m_iCurrentFileIndex][i].first.top / scale_ratio) + source_top;
					_width = m_pCurrentView->m_arrRectCode[m_pCurrentView->m_iCurrentFileIndex][i].first.Width() / scale_ratio;
					_height = m_pCurrentView->m_arrRectCode[m_pCurrentView->m_iCurrentFileIndex][i].first.Height() / scale_ratio;

					mDC.MoveTo(_left, _top);
					mDC.LineTo(_left + _width, _top);
					mDC.LineTo(_left + _width, _top + _height);
					mDC.LineTo(_left, _top + _height);
					mDC.LineTo(_left, _top);

					no_string.Format("%d", i + 1);
					mDC.SetTextColor(RGB(255, 0, 0));
					mDC.DrawText(no_string, CRect(_left + 2, _top + 2, _left + _width - 2, _top + 30), DT_LEFT | DT_TOP);
				}
			}

			if (m_pCurrentView->m_arrPageWidth.size() > m_pCurrentView->m_iCurrentFileIndex)
			{
				for (int i = 0; i < m_pCurrentView->m_arrPageWidth[m_pCurrentView->m_iCurrentFileIndex].size() - 1; i++)
				{
					if (m_bRDown && m_iCurrentSelectedPageLine  == i)
					{
						_width = ((float)source_width * m_pCurrentView->m_arrPageWidth[m_pCurrentView->m_iCurrentFileIndex][i]) + (ptEnd.x - ptFrom.x);

						CPen qCirclePen2(PS_SOLID, 2, RGB(255, 255, 0));
						pqOrigPen = mDC.SelectObject(&qCirclePen2);

						mDC.MoveTo(source_left + _width, source_top);
						mDC.LineTo(source_left + _width, source_top + source_height);
					}
					else
					{
						_width = ((float)source_width * m_pCurrentView->m_arrPageWidth[m_pCurrentView->m_iCurrentFileIndex][i]);

						CPen qCirclePen2(PS_SOLID, 2, RGB(0, 0, 255));
						pqOrigPen = mDC.SelectObject(&qCirclePen2);

						mDC.MoveTo(source_left + _width, source_top);
						mDC.LineTo(source_left + _width, source_top + source_height);
					}
				}
			}
		}
		else
		{
			if (m_pCurrentView->m_arrRectCode.size() > 0)
			{
				if (m_pCurrentView->m_arrRectCode[0].size() > 0)
				{
					_left = (m_pCurrentView->m_arrRectCode[0][0].first.left / scale_ratio) + source_left;
					_top = (m_pCurrentView->m_arrRectCode[0][0].first.top / scale_ratio) + source_top;
					_width = m_pCurrentView->m_arrRectCode[0][0].first.Width() / scale_ratio;
					_height = m_pCurrentView->m_arrRectCode[0][0].first.Height() / scale_ratio;

					mDC.MoveTo(_left, _top);
					mDC.LineTo(_left + _width, _top);
					mDC.LineTo(_left + _width, _top + _height);
					mDC.LineTo(_left, _top + _height);
					mDC.LineTo(_left, _top);
				}
			}
		}
	}
	if (m_bLDown)
	{
		mDC.MoveTo(rectSelect1_show.left, rectSelect1_show.top);
		mDC.LineTo(rectSelect1_show.left + rectSelect1_show.Width(), rectSelect1_show.top);
		mDC.LineTo(rectSelect1_show.left + rectSelect1_show.Width(), rectSelect1_show.top + rectSelect1_show.Height());
		mDC.LineTo(rectSelect1_show.left, rectSelect1_show.top + rectSelect1_show.Height());
		mDC.LineTo(rectSelect1_show.left, rectSelect1_show.top);
	}

	CPen qCirclePen2(PS_SOLID, 2, m_color2);
	pqOrigPen = mDC.SelectObject(&qCirclePen2);
	if (m_pCurrentView)
	{
		if (m_pCurrentView->m_bMultiMask)
		{
			if (m_pCurrentView->m_arrRectMask.size() > m_pCurrentView->m_iCurrentFileIndex)
			{
				for (int i = 0; i < m_pCurrentView->m_arrRectMask[m_pCurrentView->m_iCurrentFileIndex].size(); i++)
				{
					_left = (m_pCurrentView->m_arrRectMask[m_pCurrentView->m_iCurrentFileIndex][i].left / scale_ratio) + source_left;
					_top = (m_pCurrentView->m_arrRectMask[m_pCurrentView->m_iCurrentFileIndex][i].top / scale_ratio) + source_top;
					_width = m_pCurrentView->m_arrRectMask[m_pCurrentView->m_iCurrentFileIndex][i].Width() / scale_ratio;
					_height = m_pCurrentView->m_arrRectMask[m_pCurrentView->m_iCurrentFileIndex][i].Height() / scale_ratio;

					mDC.MoveTo(_left, _top);
					mDC.LineTo(_left + _width, _top);
					mDC.LineTo(_left + _width, _top + _height);
					mDC.LineTo(_left, _top + _height);
					mDC.LineTo(_left, _top);

					no_string.Format("%d", i + 1);
					mDC.SetTextColor(RGB(255, 0, 0));
					mDC.DrawText(no_string, CRect(_left + 2, _top + 2, _left + _width - 2, _top + 30), DT_LEFT | DT_TOP);
				}
			}
		}
		else
		{
			if (m_pCurrentView->m_arrRectMask.size() > 0)
			{
				if (m_pCurrentView->m_arrRectMask[0].size() > 0)
				{
					_left = (m_pCurrentView->m_arrRectMask[0][0].left / scale_ratio) + source_left;
					_top = (m_pCurrentView->m_arrRectMask[0][0].top / scale_ratio) + source_top;
					_width = m_pCurrentView->m_arrRectMask[0][0].Width() / scale_ratio;
					_height = m_pCurrentView->m_arrRectMask[0][0].Height() / scale_ratio;

					mDC.MoveTo(_left, _top);
					mDC.LineTo(_left + _width, _top);
					mDC.LineTo(_left + _width, _top + _height);
					mDC.LineTo(_left, _top + _height);
					mDC.LineTo(_left, _top);
				}
			}
		}
	}

	if (m_bRDown)
	{
		mDC.MoveTo(rectSelect2_show.left, rectSelect2_show.top);
		mDC.LineTo(rectSelect2_show.left + rectSelect2_show.Width(), rectSelect2_show.top);
		mDC.LineTo(rectSelect2_show.left + rectSelect2_show.Width(), rectSelect2_show.top + rectSelect2_show.Height());
		mDC.LineTo(rectSelect2_show.left, rectSelect2_show.top + rectSelect2_show.Height());
		mDC.LineTo(rectSelect2_show.left, rectSelect2_show.top);
	}
	//mDC.Rectangle(&rectSelect1_show);
	//CBrush brush1;
	//brush1.CreateSolidBrush(m_color1);
	//mDC.FrameRect(&rectSelect1_show, &brush1);
	//pDC->FrameRect(&rectSelect, &brush1);
	//pDC->BitBlt(0, 0, sbitmap.cx, sbitmap.cy, &dcMemory, 0, 0, SRCCOPY);
	dcMemory.SelectObject(pOldbitmap);

	dc.BitBlt(0, 0, rcClient.Width(), rcClient.Height(), &mDC, 0, 0, SRCCOPY);
}


void CPreviewDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	ptFrom = point;
	//rectSelect2_show.left = point.x;
	//rectSelect2_show.top = point.y;
	//rectSelect2_show.right = point.x;
	//rectSelect2_show.bottom = point.y;
	//m_bRDown = true;
	m_iCurrentSelectedPageLine = -1;
	if (m_pCurrentView->m_arrPageWidth.size() > m_pCurrentView->m_iCurrentFileIndex)
	{
		long current_length = source_width;
		for (int i = 0; i < m_pCurrentView->m_arrPageWidth[m_pCurrentView->m_iCurrentFileIndex].size() - 1; i++)
		{
			long _width = source_left + ((float)source_width * m_pCurrentView->m_arrPageWidth[m_pCurrentView->m_iCurrentFileIndex][i]);

			long length = abs(point.x - _width);
			if (length < current_length)
			{
				current_length = length;
				m_iCurrentSelectedPageLine = i;
				m_bRDown = true;
			}
		}
	}


	CDialog::OnRButtonDown(nFlags, point);
}

void CPreviewDlg::SetBoxIndex(UINT nID)
{
	if (m_iCurrentSelectedBox > -1)
	{
		int current_index = m_pCurrentView->m_iCurrentFileIndex;
		int page_count = m_pCurrentView->m_arrRectCode.size();

		int to_index = nID - (WM_USER + 1000);
		if (to_index == 0)
		{
			m_pCurrentView->m_mapCodeImageFile.erase(m_pCurrentView->m_arrRectCode[current_index][m_iCurrentSelectedBox].second);
			m_pCurrentView->m_arrRectCode[current_index].erase(m_pCurrentView->m_arrRectCode[current_index].begin() + m_iCurrentSelectedBox);

			if (current_index == 0)
			{
				for (int j = 1; j < page_count; j++)
				{
					m_pCurrentView->m_arrRectCode[j].erase(m_pCurrentView->m_arrRectCode[j].begin() + m_iCurrentSelectedBox);
				}
				m_pCurrentView->InitGrid();
			}
		}
		else
		{
			to_index--;
			if (to_index < m_iCurrentSelectedBox)
			{
				for (int i = m_iCurrentSelectedBox; i > to_index; i--)
				{
					std::swap(m_pCurrentView->m_arrRectCode[current_index].at(i), m_pCurrentView->m_arrRectCode[current_index].at(i - 1));
					if (current_index == 0)
					{
						for (int j = 1; j < page_count; j++)
						{
							std::swap(m_pCurrentView->m_arrRectCode[j].at(i), m_pCurrentView->m_arrRectCode[j].at(i - 1));
						}
					}
				}
			}
			else if (to_index > m_iCurrentSelectedBox)
			{
				for (int i = m_iCurrentSelectedBox; i < to_index; i++)
				{
					std::swap(m_pCurrentView->m_arrRectCode[current_index].at(i), m_pCurrentView->m_arrRectCode[current_index].at(i + 1));
					if (current_index == 0)
					{
						for (int j = 1; j < page_count; j++)
						{
							std::swap(m_pCurrentView->m_arrRectCode[j].at(i), m_pCurrentView->m_arrRectCode[j].at(i + 1));
						}
					}
				}
			}
			else
			{
				return;
			}
		}

		Invalidate();
		CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
		if (pFrame)
		{
			pFrame->m_wndTemplate1.m_Dialog.UpdateImages();
		}
	}
}

void CPreviewDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	m_iCurrentSelectedBox = -1;
	int current_index = m_pCurrentView->m_iCurrentFileIndex;
	if (current_index > -1 && current_index <= m_pCurrentView->m_arrRectCode.size())
	{
		for (int i = 0; i < m_pCurrentView->m_arrRectCode[current_index].size(); i++)
		{
			CRect rect = GetWindowRectFromImage(m_pCurrentView->m_arrRectCode[current_index][i].first);
			if (rect.left <= point.x && rect.right >= point.x && rect.top <= point.y && rect.bottom >= point.y)
			{
				m_bRDown = false;
				m_iCurrentSelectedPageLine = -1;
				Invalidate();

				m_iCurrentSelectedBox = i;

				CMenu menu;
				menu.CreatePopupMenu();
				CString temp_string;
				menu.AppendMenu(MF_STRING, WM_USER + 1000, "Delete");
				for (int i = 0; i < m_pCurrentView->m_arrRectCode[m_pCurrentView->m_iCurrentFileIndex].size(); i++)
				{
					temp_string.Format("%d", i + 1);
					menu.AppendMenu(MF_STRING, WM_USER + 1000 + i + 1, temp_string);
				}
				CPoint	pt = point;

				ClientToScreen(&pt);

				menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this);
				menu.DestroyMenu();
				
				return;
			}
		}
	}

	if (m_pCurrentView->m_arrPageWidth.size() > m_pCurrentView->m_iCurrentFileIndex)
	{
		if (m_bRDown && m_iCurrentSelectedPageLine > -1)
		{
			float _width = ((float)source_width * m_pCurrentView->m_arrPageWidth[m_pCurrentView->m_iCurrentFileIndex][m_iCurrentSelectedPageLine]) + (float)(ptEnd.x - ptFrom.x);
			m_pCurrentView->m_arrPageWidth[m_pCurrentView->m_iCurrentFileIndex][m_iCurrentSelectedPageLine] = _width / (float)source_width;
			sort(m_pCurrentView->m_arrPageWidth[m_pCurrentView->m_iCurrentFileIndex].begin(), m_pCurrentView->m_arrPageWidth[m_pCurrentView->m_iCurrentFileIndex].end());
			m_bRDown = false;
			m_iCurrentSelectedPageLine = -1;
			Invalidate();
		}
	}


	//ptEnd = point;
	//rectSelect2_show.left = min(ptFrom.x, ptEnd.x);
	//rectSelect2_show.right = max(ptFrom.x, ptEnd.x);
	//rectSelect2_show.top = min(ptFrom.y, ptEnd.y);
	//rectSelect2_show.bottom = max(ptFrom.y, ptEnd.y);
	//m_bRDown = false;
	//Invalidate();

	//SaveTemplateFile2();

	CDialog::OnRButtonUp(nFlags, point);
}


BOOL CPreviewDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	int _left = (int)ReadDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("TEMP1_LEFT"));
	int _top = (int)ReadDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("TEMP1_RIGHT"));
	int _width = (int)ReadDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("TEMP1_WIDTH"));
	int _height = (int)ReadDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("TEMP1_HEIGHT"));
	rectSelect1_image.SetRect(_left, _top, _left + _width, _top + _height);

	_left = (int)ReadDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("TEMP2_LEFT"));
	_top = (int)ReadDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("TEMP2_RIGHT"));
	_width = (int)ReadDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("TEMP2_WIDTH"));
	_height = (int)ReadDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("TEMP2_HEIGHT"));
	rectSelect2_image.SetRect(_left, _top, _left + _width, _top + _height);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}


void CPreviewDlg::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	//m_pCurrentView->DeleteCodeRect(point);
	//CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	//if (pFrame)
	//{
	//	pFrame->m_wndTemplate1.m_Dialog.UpdateImages();
	//}

	CDialog::OnRButtonDblClk(nFlags, point);
}
