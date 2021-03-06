// 이 MFC 샘플 소스 코드는 MFC Microsoft Office Fluent 사용자 인터페이스("Fluent UI")를
// 사용하는 방법을 보여 주며, MFC C++ 라이브러리 소프트웨어에 포함된
// Microsoft Foundation Classes Reference 및 관련 전자 문서에 대해
// 추가적으로 제공되는 내용입니다.
// Fluent UI를 복사, 사용 또는 배포하는 데 대한 사용 약관은 별도로 제공됩니다.
// Fluent UI 라이선싱 프로그램에 대한 자세한 내용은
// https://go.microsoft.com/fwlink/?LinkId=238214.
//
// Copyright (C) Microsoft Corporation
// All rights reserved.

// AutoSplitPagesView.h: CAutoSplitPagesView 클래스의 인터페이스
//

#pragma once
#include "DlgProgress.h"

class CAutoSplitPagesDoc;
class CMainFrame;

class CMemoryDC;
class CImageInfo
{
public:
	CBitmap* image;
	CBitmap* result_image;
	CString file_path;
	CRect rect;
	bool is_checked;
	CImageInfo(CBitmap* _image, CString& _path)
		: image(_image)
		, result_image(NULL)
		, file_path(_path)
		, is_checked(false)
	{
	};

	CImageInfo(const CImageInfo& s)
	{
		this->image = s.image;
		this->result_image = s.result_image;
		this->file_path = s.file_path;
		this->rect = s.rect;
		this->is_checked = s.is_checked;
	};

	~CImageInfo()
	{
	};
};

class CAutoSplitPagesView : public CScrollView
{
protected: // serialization에서만 만들어집니다.
	CAutoSplitPagesView() noexcept;
	DECLARE_DYNCREATE(CAutoSplitPagesView)

// 특성입니다.
public:
	CAutoSplitPagesDoc* GetDocument() const;
	CString m_strTempFolder;
	//CListCtrl m_List;
	//CImageList m_imageList;
	CString m_strCurrentFolder;
	//vector< string > item_names;
	//vector< int > item_header_pages;
	bool m_bStopChecking;
	vector< CImageInfo > m_arrImages;
	vector<vector<CRect>> m_arrRectMask;
	vector<vector<pair<CRect, CString >>> m_arrRectCode;
	vector< vector< CString > > m_arrOCRText;
	vector < int > m_arrOCRPrevCount;
	vector< vector< float > > m_arrPageWidth;
	map< CString, int > m_mapCodeImageFile;
	vector< pair<CString, bool> > m_arrHeaderFiles;
	void ReadCheckInfo(CString folder);
	bool IsFileChecked(CString& file_name);
	//void DoCheckImages(CString folder, Mat& templ);
	void OnSplitPages(CString folder);
	void ExtractSelectOCR(int index);
	void SetCodeRect(CRect& rect, CString file_name);
	void DeleteCodeRect(CPoint point);
	void SetMaskRect(CRect& rect);
	void InitGrid();
	CString ExtractFullText(int index);
	CString ExtractFullText(CString origin_file_path, int rect_info_index = 0);
	void CAutoSplitPagesView::ExtractFullTextFolder(CString folder, bool skip_files = false);
	int m_iTotalFileCount;
	int m_iCurrentFileCount;
	int m_iCurrentPageNum = 1;
	void InitPageNum();
	CDlgProgress* m_DlgProgress;
	void BeginProgress();
	void EndProgress();
	void UpdateProgress(int current, int total);
	CString m_strProgressStatus;
	time_t start_time;
	time_t prev_update_time;
	time_t end_time;
	bool m_bPreserveExsiting;
	int m_iSelPageType;

	// 작업입니다.
public:
	BOOL FillInImages(CString folder);
	double m_dThumbnailResolution;
	bool m_bFindSubFolder;
	double m_dCheckValue;
	CRITICAL_SECTION m_cx;
	void SaveCurrentCheckFile();
	int m_iCurrentFileIndex;
	int m_iPrevFileIndex;
	void SetCurrentFile(int index, CMainFrame* pFrame);

	bool m_bApplyFolder_Code;
	bool m_bApplyFolder_Mask;
	bool m_bUsingFolderName_Code;
	bool m_bMultiCode;
	bool m_bMultiMask;
	bool m_bAutoResetMulti;
	int m_iFontSize;
	CString m_sFontName;
	void ProcessImage(CString file_path, bool use_code, bool use_mask, int current_index);
	void OnProcessImages();
	void OnProcessImages(CString folder);
	void OnApplyImages();
	bool m_bViewThumbnailView;
	CMainFrame* m_pFrame;
	void SavePageCodeImage(int page_index);
	void UpdatePageImages(int page_index);
	bool UpdateRectImage(CRect rect, CString page_path, CString file_name);
	bool UpdateRectImage(int page_index, int rect_index, CString file_name);
	void OnCellChange(int oldcol, int newcol, long oldrow, long newrow);
	LRESULT OnGridAutoCheck(WPARAM wParam, LPARAM lParam);
private:
	const double m_dMaxThumbnailWidth;
	const double m_dMinThumbnailWidth;
	double m_dThumbnailWidth;
	const double m_dSpaceBetween;
	const double m_dLeftSpace;
	const double m_dTopSpace;
	const double m_dTextSpace;

// 재정의입니다.
public:
	virtual void OnDraw(CDC* pDCView);  // 이 뷰를 그리기 위해 재정의되었습니다.
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual BOOL OnScrollBy(CSize sizeScroll, BOOL bDoScroll = TRUE);
	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);

// 구현입니다.
public:
	virtual ~CAutoSplitPagesView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	void _DrawBackgound(CMemoryDC* pDCDraw, const CRect& rcClient);
	void _DrawThumbnails(CMemoryDC* pDCDraw, const CRect& rcClient);
	UINT _GetColCount();
	UINT _GetThumbnailCount();
	void _GetThumbnailRect(UINT nIndex, CRect& rc, CRect& rc_text);
	CSize _GetVirtualViewSize();

// 생성된 메시지 맵 함수
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//afx_msg void OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult);
	//afx_msg void OnCheckImages();
	afx_msg void OnSplitPages();
	afx_msg void OnButtonStop();
	virtual void OnInitialUpdate();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnSliderSize();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSliderResolution();
	afx_msg void OnSpinSize();
	afx_msg void OnSpinResolution();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnCheckSubfolder();
	afx_msg void OnUpdateCheckSubfolder(CCmdUI *pCmdUI);
	afx_msg void OnEditCheckValue();
	afx_msg void OnCheckUseFolername();
	afx_msg void OnCheckLogo();
	afx_msg void OnCheckCurrent();
	afx_msg void OnCheckFolder();
	afx_msg void OnButtonCodeColor();
	afx_msg void OnUpdateCheckUseFolername(CCmdUI *pCmdUI);
	afx_msg void OnUpdateCheckLogo(CCmdUI *pCmdUI);
	afx_msg void OnUpdateCheckCurrent(CCmdUI *pCmdUI);
	afx_msg void OnUpdateCheckFolder(CCmdUI *pCmdUI);
	afx_msg void OnCheckMaskCurrent();
	afx_msg void OnUpdateCheckMaskCurrent(CCmdUI *pCmdUI);
	afx_msg void OnCheckMaskFolder();
	afx_msg void OnUpdateCheckMaskFolder(CCmdUI *pCmdUI);
	afx_msg void OnButtonMaskColor();
	afx_msg void OnButtonProcess();
	//virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnComboFont();
	afx_msg void OnEditFontSize();
	afx_msg void OnButtonApply();
	afx_msg void OnCheckApply();
	afx_msg void OnUpdateCheckApply(CCmdUI *pCmdUI);
	afx_msg void OnFileView();
	afx_msg void OnUpdateFileView(CCmdUI *pCmdUI);
	afx_msg void OnResult();
	afx_msg void OnUpdateResult(CCmdUI *pCmdUI);
	afx_msg void OnStandard();
	afx_msg void OnUpdateStandard(CCmdUI *pCmdUI);
	afx_msg void OnImage1();
	afx_msg void OnUpdateImage1(CCmdUI *pCmdUI);
	afx_msg void OnImage2();
	afx_msg void OnUpdateImage2(CCmdUI *pCmdUI);
	afx_msg void OnCheckMultiCode();
	afx_msg void OnUpdateCheckMultiCode(CCmdUI *pCmdUI);
	afx_msg void OnCheckMultiMask();
	afx_msg void OnUpdateCheckMultiMask(CCmdUI *pCmdUI);
	afx_msg void OnCheckResetFolder();
	afx_msg void OnUpdateCheckResetFolder(CCmdUI *pCmdUI);
	afx_msg void OnButtonOCR_Select_all();
	afx_msg void OnButtonOCR_Select_page();
	afx_msg void OnButtonOCR_Page_all();
	afx_msg void OnButtonOCR_Page_page();
	afx_msg void OnButtonSaveExcel();
	afx_msg void OnSpinPageNum();
	afx_msg void OnButtonSelClear();
	afx_msg void OnButtonOcrPageTextSub();
	afx_msg void OnCheckPreserveExsiting();
	afx_msg void OnUpdateCheckPreserveExsiting(CCmdUI *pCmdUI);
	afx_msg void OnButtonOcrSelPageText();
	afx_msg void OnCheckAll();
	afx_msg void OnCheckCur();
	afx_msg void OnCheckSel();
	afx_msg void OnUpdateCheckAll(CCmdUI *pCmdUI);
	afx_msg void OnUpdateCheckCur(CCmdUI *pCmdUI);
	afx_msg void OnUpdateCheckSel(CCmdUI *pCmdUI);
	afx_msg void OnCheckCurrentPage();
	afx_msg void OnUpdateCheckCurrentPage(CCmdUI *pCmdUI);
	afx_msg void OnButtonOcrSel();
};

#ifndef _DEBUG  // AutoSplitPagesView.cpp의 디버그 버전
inline CAutoSplitPagesDoc* CAutoSplitPagesView::GetDocument() const
   { return reinterpret_cast<CAutoSplitPagesDoc*>(m_pDocument); }
#endif

