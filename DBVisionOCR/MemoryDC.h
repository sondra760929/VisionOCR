// MemDC.h: interface for the CMemoryDC class.
//
// Author: Ovidiu Cucu
// Homepage: http://www.codexpert.ro/
// Weblog: http://codexpert.ro/blog/

#pragma once


class CMemoryDC : public CDC  
{
    DECLARE_DYNAMIC(CMemoryDC)
private:
    CDC* m_pDestDC;
    CRect m_rcDest;
    CRect m_rcSrc;
    int m_nSavedDC;
    CBitmap m_bitmap;
    UINT m_nSrcWidth, m_nSrcHeight;
	CRect m_rcText;
	CString m_strText;
public:
    CMemoryDC(CDC* pDestDC, const CRect& rcDest);
	CMemoryDC(CDC* pDestDC, CBitmap* pBitmap, const CRect& rcDest);
	CMemoryDC(CDC* pDestDC, CBitmap* pBitmap, const CRect& rcDest, CString strText, const CRect& rcText);
	~CMemoryDC();
};

inline CMemoryDC::CMemoryDC(CDC* pDestDC, const CRect& rcDest)
    : m_pDestDC(pDestDC),
      m_rcDest(rcDest)
{
    ASSERT_VALID(m_pDestDC);
    ASSERT(! m_rcDest.IsRectEmpty());

    VERIFY(CreateCompatibleDC(m_pDestDC));
    m_nSavedDC = SaveDC();
    m_nSrcWidth = m_rcDest.Width();
    m_nSrcHeight =  m_rcDest.Height();
    VERIFY(m_bitmap.CreateCompatibleBitmap(m_pDestDC, m_nSrcWidth, m_nSrcHeight));
    SelectObject(&m_bitmap);
    
}

inline CMemoryDC::CMemoryDC(CDC* pDestDC, CBitmap* pBitmap, const CRect& rcDest)
	: m_pDestDC(pDestDC),
	m_rcDest(rcDest)
{
	ASSERT_VALID(m_pDestDC);
	ASSERT_VALID(pBitmap);
	ASSERT(!m_rcDest.IsRectEmpty());

	VERIFY(CreateCompatibleDC(m_pDestDC));
	m_nSavedDC = SaveDC();
	BITMAP bmp;
	pBitmap->GetBitmap(&bmp);
	m_nSrcWidth = bmp.bmWidth;
	m_nSrcHeight = bmp.bmHeight;
	SelectObject(pBitmap);
}

inline CMemoryDC::CMemoryDC(CDC* pDestDC, CBitmap* pBitmap, const CRect& rcDest, CString strText, const CRect& rcText)
	: m_pDestDC(pDestDC),
	m_rcDest(rcDest),
	m_strText(strText),
	m_rcText(rcText)
{
	ASSERT_VALID(m_pDestDC);
	ASSERT_VALID(pBitmap);
	ASSERT(!m_rcDest.IsRectEmpty());

	VERIFY(CreateCompatibleDC(m_pDestDC));
	m_nSavedDC = SaveDC();
	BITMAP bmp;
	pBitmap->GetBitmap(&bmp);
	m_nSrcWidth = bmp.bmWidth;
	m_nSrcHeight = bmp.bmHeight;
	SelectObject(pBitmap);
}

inline CMemoryDC::~CMemoryDC()
{
    ASSERT_VALID(m_pDestDC);

    if((m_rcDest.Width() == m_nSrcWidth) && (m_rcDest.Height() == m_nSrcHeight))
    {
        m_pDestDC->BitBlt(m_rcDest.left, m_rcDest.top, m_rcDest.Width(), m_rcDest.Height(), this, 0, 0, SRCCOPY);
    }
    else
    {
        m_pDestDC->SetStretchBltMode(HALFTONE);
        m_pDestDC->StretchBlt(m_rcDest.left, m_rcDest.top, m_rcDest.Width(), m_rcDest.Height(), this, 
            0, 0, m_nSrcWidth, m_nSrcHeight, SRCCOPY);
    }
    
	if (m_strText != "")
	{
		CFont font;
		font.CreatePointFont(75/*nPointSize*/, "Arial", m_pDestDC);
		CFont* pOldFont = m_pDestDC->SelectObject(&(font));

		m_pDestDC->DrawText(m_strText, m_rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

		m_pDestDC->SelectObject(pOldFont);
	}

    RestoreDC(m_nSavedDC);
    if(NULL != m_bitmap.GetSafeHandle())
    {
        m_bitmap.DeleteObject();
    }
}

