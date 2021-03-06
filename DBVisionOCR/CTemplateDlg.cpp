// CTemplateDlg.cpp: 구현 파일
//

#include "stdafx.h"
#include "AutoSplitPages.h"
#include "CTemplateDlg.h"
#include "afxdialogex.h"
#include "AutoSplitPagesView.h"

// CTemplateDlg 대화 상자

IMPLEMENT_DYNAMIC(CTemplateDlg, CDialog)

CTemplateDlg::CTemplateDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIALOG_TEMPLATE, pParent)
{

}

CTemplateDlg::~CTemplateDlg()
{
}

void CTemplateDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CTemplateDlg, CDialog)
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_PAINT()
END_MESSAGE_MAP()


// CTemplateDlg 메시지 처리기

void CTemplateDlg::ShowImage(CString file_path)
{
	UpdateImages();

	//CImage pngImage;
	//CDC bmDC;
	//CBitmap *pOldbmp;
	//BITMAP  bi;
	//UINT xPos = 450, yPos = 300;

	//CClientDC dc(this);
	//m_strFilePath = file_path;
	//if (!FAILED(pngImage.Load(file_path)))
	//{
	//	// new code
	//	pngBmp.DeleteObject();
	//	pngBmp.Attach(pngImage.Detach());
	//	BITMAP bm; //Create bitmap Handle to get dimensions
	//	pngBmp.GetBitmap(&bm); //Load bitmap into handle
	//	bitmapSize = CSize(bm.bmWidth, bm.bmHeight); // Get bitmap Sizes;
	//	Invalidate(1);
	//	pngImage.Destroy();
	//}

	////bmDC.CreateCompatibleDC(&dc);

	////pOldbmp = bmDC.SelectObject(&pngBmp);
	////pngBmp.GetBitmap(&bi);
	////dc.BitBlt(xPos, yPos, bi.bmWidth, bi.bmHeight, &bmDC, 0, 0, SRCCOPY);
	////bmDC.SelectObject(pOldbmp);
}


BOOL CTemplateDlg::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CTemplateDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	Invalidate();
}

void CTemplateDlg::OnPaint()
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
	int client_width = rcClient.Width();
	int client_height = rcClient.Height();

	CBitmap mbitmap;
	mbitmap.CreateCompatibleBitmap(&dc, rcClient.Width(), rcClient.Height());
	mDC.SelectObject(&mbitmap);


	const CSize& sbitmap = bitmapSize;
	//mDC.CreateCompatibleDC(&dc);
	CBrush brush;
	brush.CreateSolidBrush(RGB(210, 210, 210));

	//dc.FillRect(&rcClient, &brush);
	mDC.FillRect(&rcClient, &brush);
	source_left, source_top, source_width, source_height;

	float ratio = (float)max_width / (float)total_height;
	float ratio1 = (float)client_width / (float)client_height;
	if (ratio > ratio1)
	{
		source_width = client_width;
		source_height = (float)client_width / ratio;
		source_top = (client_height - source_height) / 2;
		source_left = 0;
	}
	else
	{
		source_width = (float)client_height * ratio;
		source_height = client_height;
		source_left = (client_width - source_width) / 2;
		source_top = 0;
	}

	scale_ratio = (float)source_width / (float)max_width;
	CString no_string;
	long current_top = source_top;
	for (int i = 0; i < m_aSize.size(); i++)
	{
		no_string.Format(" <<%d>> ", i + 1);
		mDC.SetTextColor(RGB(255, 0, 0));
		mDC.DrawText(no_string, CRect(source_left + 2, current_top + 2, source_left + source_width, current_top + 30), DT_LEFT | DT_TOP);
		current_top += 30;

		long current_width = (long)((float)(m_aSize[i].cx) * scale_ratio);
		long current_height = (long)((float)(m_aSize[i].cy) * scale_ratio);
		CDC dcMemory;
		dcMemory.CreateCompatibleDC(&dc);
		CBitmap* pOldbitmap = dcMemory.SelectObject(m_aBmp[i]);
		mDC.StretchBlt(source_left, current_top, current_width, current_height, &dcMemory, 0, 0, m_aSize[i].cx, m_aSize[i].cy, SRCCOPY);

		current_top += current_height;
		current_top += 2;
		dcMemory.SelectObject(pOldbitmap);
	}

	CBrush brush1;
	brush1.CreateSolidBrush(RGB(10, 10, 10));
	mDC.FrameRect(&rectSelect, &brush1);
	//pDC->FrameRect(&rectSelect, &brush1);
	//pDC->BitBlt(0, 0, sbitmap.cx, sbitmap.cy, &dcMemory, 0, 0, SRCCOPY);

	dc.BitBlt(0, 0, client_width, client_height, &mDC, 0, 0, SRCCOPY);
}


BOOL CTemplateDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

void CTemplateDlg::UpdateImages()
{
	for (int i = 0; i < m_aBmp.size(); i++)
	{
		delete m_aBmp[i];
	}
	m_aBmp.clear();
	m_aSize.clear();

	CImage pngImage;
	max_width = 0;
	total_height = 0;
	if (m_pCurrentView)
	{
		int current_index = m_pCurrentView->m_iCurrentFileIndex;
		if (current_index >= m_pCurrentView->m_arrRectCode.size())
		{
			return;
		}

		for (int i = 0; i < m_pCurrentView->m_arrRectCode[current_index].size(); i++)
		{
			if (!FAILED(pngImage.Load(m_strTempPath + m_pCurrentView->m_arrRectCode[current_index][i].second)))
			{
				CBitmap* bmp = new CBitmap;
				bmp->Attach(pngImage.Detach());
				m_aBmp.push_back(bmp);
				m_aSize.push_back(CSize(m_pCurrentView->m_arrRectCode[current_index][i].first.Width(),
					m_pCurrentView->m_arrRectCode[current_index][i].first.Height()));

				if (m_pCurrentView->m_arrRectCode[current_index][i].first.Width() > max_width)
					max_width = m_pCurrentView->m_arrRectCode[current_index][i].first.Width();
				total_height += (m_pCurrentView->m_arrRectCode[current_index][i].first.Height() + 2);
				total_height += 30;
			}
		}
	}

	Invalidate();
}