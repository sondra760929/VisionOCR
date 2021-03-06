#pragma once


// CTemplateDlg 대화 상자

class CTemplateDlg : public CDialog
{
	DECLARE_DYNAMIC(CTemplateDlg)

public:
	CTemplateDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CTemplateDlg();
	void ShowImage(CString file_path);
	CBitmap pngBmp;
	CSize bitmapSize; // For Holding The bitmap Size
	CRect rectSelect;
	CPoint ptFrom;
	CPoint ptEnd;
	bool m_bDown;
	long source_left, source_top, source_width, source_height;
	float scale_ratio;
	CString m_strFilePath;
	void UpdateImages();
	long max_width;
	long total_height;

	vector< CBitmap* > m_aBmp;
	vector< CSize > m_aSize;

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_TEMPLATE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
};
