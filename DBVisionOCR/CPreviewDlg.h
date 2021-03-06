#pragma once


// CPreviewDlg 대화 상자

class CPreviewDlg : public CDialog
{
	DECLARE_DYNAMIC(CPreviewDlg)

public:
	CPreviewDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CPreviewDlg();
	void ShowImage(CString file_path);
	CBitmap pngBmp;
	CSize bitmapSize; // For Holding The bitmap Size
	CRect rectSelect1_image;
	CRect rectSelect2_image;
	CRect rectSelect1_show;
	CRect rectSelect2_show;
	CPoint ptFrom;
	CPoint ptEnd;
	bool m_bLDown;
	bool m_bRDown;
	long source_left, source_top, source_width, source_height;
	float scale_ratio;
	CString m_strFilePath;
	CRect GetImageRectFromWindow(CRect rect);
	CRect GetWindowRectFromImage(CRect rect);
	int m_iCurrentSelectedBox;
	void SaveTemplateFile1();
	void SaveTemplateFile2();
	int m_iCurrentSelectedPageLine;
// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_PREVIEW };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	virtual BOOL OnInitDialog();
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void SetBoxIndex(UINT nID);
};
