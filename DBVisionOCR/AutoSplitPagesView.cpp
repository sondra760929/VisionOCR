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

// AutoSplitPagesView.cpp: CAutoSplitPagesView 클래스의 구현
//

#include "stdafx.h"
// SHARED_HANDLERS는 미리 보기, 축소판 그림 및 검색 필터 처리기를 구현하는 ATL 프로젝트에서 정의할 수 있으며
// 해당 프로젝트와 문서 코드를 공유하도록 해 줍니다.
#ifndef SHARED_HANDLERS
#include "AutoSplitPages.h"
#endif

#include "AutoSplitPagesDoc.h"
#include "AutoSplitPagesView.h"
#include "MainFrm.h"
//#include <api/baseapi.h>
//#include <allheaders.h>
#include "MemoryDC.h"
//#include <Magick++.h>
#include <string>
#include <iostream>
#include "DlgRename.h"
#include <xlnt/xlnt.hpp>
#include "AutoCheckDlg.h"
using namespace std;

bool use_ai_ocr = true;
//using namespace Magick;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// structure used to pass parameters to a image adder thread
struct threadParamImage
{
	CString		folder;   // the folder to be scanned for thumbnails
	CAutoSplitPagesView*	pView;    // the view that accomodates them
	CMainFrame* pFrame;
	HANDLE		readPipe; // the pipe used to pass thumbnail filenames to the 
							  // JPEG loader thread
	bool use_code;
	bool use_mask;
};

// structure used to pass parameters to a JPEG image loader thread
struct threadParam
{
	CAutoSplitPagesView*	pView;    // the view that shows thumbnails
	HANDLE		readPipe; // the pipe used to pass thumbnail filenames to the
							  // JPEG loader thread
};

HANDLE	hThread = NULL; // handle to the JPEG image loader thread
HANDLE	readPipe = NULL; // read and write ends of the communication pipe
HANDLE	writePipe = NULL;
HANDLE skipImages = NULL; // handle to the semaphore that signals the pipe 
								  //does no longer hold consistent data
HANDLE imageFiller = NULL; // handle to the thumbnail adder thread
HANDLE imageFillerSemaphore = NULL; // thread termination flag (when this semaphore 
									// goes signaled, the thread must exit)
HANDLE imageFillerCR = NULL; // thumbnail adder thread critical section semaphore
HANDLE imageFillerWait = NULL; // second thumbnail adder thread critical section semaphore

void char_to_utf8(char* strMultibyte, char* out)
{
	memset(out, 0, 512);
	wchar_t strUni[512] = { 0, };
	int nLen = MultiByteToWideChar(CP_ACP, 0, strMultibyte, strlen(strMultibyte), NULL, NULL);
	MultiByteToWideChar(CP_ACP, 0, strMultibyte, strlen(strMultibyte), strUni, nLen);

	nLen = WideCharToMultiByte(CP_UTF8, 0, strUni, lstrlenW(strUni), NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, strUni, lstrlenW(strUni), out, nLen, NULL, NULL);
}

void lpctstr_to_utf8(LPCTSTR in, char* out)
{
	char    strMultibyte[512] = { 0, };
	strcpy_s(strMultibyte, 512, CT2A(in));
	char_to_utf8(strMultibyte, out);
}

wstring utf_to_unicode(string in)
{
	wstring return_string;
	wchar_t strUnicode[1024] = { 0, };
	char    strUTF8[1024] = { 0, };
	strcpy_s(strUTF8, 1024, in.c_str());// 이건 사실 멀티바이트지만 UTF8이라고 생각해주세요 -_-;;
	if (strlen(strUTF8) > 0)
	{
		int nLen = MultiByteToWideChar(CP_UTF8, 0, strUTF8, strlen(strUTF8), NULL, NULL);
		MultiByteToWideChar(CP_UTF8, 0, strUTF8, strlen(strUTF8), strUnicode, nLen);

		wstring strMulti(strUnicode);
		return strMulti;
	}
	return return_string;
}

string utf_to_multibyte(string in)
{
	string strMulti;
	wstring strUni = utf_to_unicode(in);
	strMulti.assign(strUni.begin(), strUni.end());
	return strMulti;
	//string strMulti(CW2A(utf_to_unicode(in)));
	//wchar_t strUnicode[1024] = { 0, };
	//char    strUTF8[1024] = { 0, };
	//strcpy_s(strUTF8, 1024, in.c_str());// 이건 사실 멀티바이트지만 UTF8이라고 생각해주세요 -_-;;
	//if (strlen(strUTF8) > 0)
	//{
	//	int nLen = MultiByteToWideChar(CP_UTF8, 0, strUTF8, strlen(strUTF8), NULL, NULL);
	//	MultiByteToWideChar(CP_UTF8, 0, strUTF8, strlen(strUTF8), strUnicode, nLen);

	//	string strMulti = CW2A(strUnicode);
	//	return strMulti;
	//}
	//return "";
}

CBitmap* MakeThumbnailImage(CAutoSplitPagesView* pView, CString file_path)
{
	CImage pngImage;
	if (!FAILED(pngImage.Load(file_path)))
	{

		CDC dc;
		dc.Attach(GetDC(pView->m_hWnd));

		CDC* pMDC = new CDC;
		pMDC->CreateCompatibleDC(&dc);

		CBitmap* pb = new CBitmap;
		int new_width = (double)pngImage.GetWidth() * pView->m_dThumbnailResolution;
		int new_height = (double)pngImage.GetHeight() * pView->m_dThumbnailResolution;
		pb->CreateCompatibleBitmap(&dc, new_width, new_height);

		CBitmap* pob = pMDC->SelectObject(pb);

		pMDC->SetStretchBltMode(HALFTONE);
		pngImage.StretchBlt(pMDC->m_hDC, 0, 0, new_width, new_height, 0, 0, pngImage.GetWidth(), pngImage.GetHeight(), SRCCOPY);

		pMDC->SelectObject(pob);

		return pb;
	}
	return NULL;
}

// thumbnail adder thread
UINT ImageFillerThread(void* param)
{
	// get thread parameters
	threadParamImage* pParam = (threadParamImage*)param;
	CString folder = pParam->folder;
	CAutoSplitPagesView* pView = pParam->pView;
	CMainFrame* pFrame = pParam->pFrame;
	HANDLE readPipe = pParam->readPipe;
	// cleanup
	delete pParam;
	CString temp_string;

	// wait for previous copies to stop
	WaitForSingleObject(imageFillerWait, INFINITE);

	// clear previous images from list control
	//pView->m_List.DeleteAllItems();
	//pView->item_names.clear();
	TRACE("delete image\n");
	::EnterCriticalSection(&(pView->m_cx));
	vector< CBitmap* > temp_images;
	for each(auto img in pView->m_arrImages)
	{
		temp_images.push_back(img.image);
		//delete img.image;
	}
	pView->m_arrImages.clear();
	::LeaveCriticalSection(&(pView->m_cx));

	for each(auto img in temp_images)
	{
		delete img;
	}

	if (pFrame)
	{
		temp_string.Format("%s Loading...", folder);
		pFrame->SetStatusBar(temp_string);
	}

	pView->Invalidate();
	// start scanning designated folder for thumbnails
	WIN32_FIND_DATA fd;
	HANDLE find;
	BOOL ok = TRUE;
	fd.dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_COMPRESSED |
		FILE_ATTRIBUTE_READONLY;
	find = FindFirstFile(folder + "\\*.*", &fd);

	// return if operation failed
	if (find == INVALID_HANDLE_VALUE)
	{
		ReleaseSemaphore(imageFillerWait, 1, NULL);
		ExitThread(0);
		return 0;
	}

	// critical section ends here
	ReleaseSemaphore(imageFillerCR, 1, NULL);

	pView->ReadCheckInfo(folder);
	vector< string > file_names;
	// start adding items to the list control
	do
	{
		if (WaitForSingleObject(imageFillerSemaphore, 0) == WAIT_OBJECT_0)
		{
			// thread is signaled to stop
			// signal skip to JPEG file loader
			int skip = -1;
			DWORD dummy;
			WriteFile(writePipe, &skip, sizeof(int), &dummy, NULL);
			ReleaseSemaphore(skipImages, 1, NULL);
			break;
		}

		ok = FindNextFile(find, &fd);
		if (fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)continue;

		if (ok)
		{
			file_names.push_back(fd.cFileName);
		}
	} while (find&&ok);
	FindClose(find);

	sort(file_names.begin(), file_names.end());
	CString check_file_path;
	for (int i = 0; i < file_names.size(); i++)
	{
		check_file_path.Format("%s\\%s", folder, file_names[i].c_str());
		CBitmap* pb = MakeThumbnailImage(pView, check_file_path);
		if (pb)
		{
			CString img_file_name(file_names[i].c_str());
			CImageInfo i(pb, img_file_name);
			i.is_checked = pView->IsFileChecked(img_file_name);
			pView->m_arrImages.push_back(i);
			if (pView->m_arrImages.size() == 1)
			{
				pView->SetCurrentFile(0, pFrame);
			}

			TRACE("ImageFillerThread\n");
			pView->Invalidate();
		}
		//CImage pngImage;
		//check_file_path.Format("%s\\%s", folder, file_names[i].c_str());
		//if (!FAILED(pngImage.Load(check_file_path)))
		//{

		//	CDC dc;
		//	dc.Attach(GetDC(pView->m_hWnd));

		//	CDC* pMDC = new CDC;
		//	pMDC->CreateCompatibleDC(&dc);

		//	CBitmap* pb = new CBitmap;
		//	int new_width = (double)pngImage.GetWidth() * pView->m_dThumbnailResolution;
		//	int new_height = (double)pngImage.GetHeight() * pView->m_dThumbnailResolution;
		//	pb->CreateCompatibleBitmap(&dc, new_width, new_height);

		//	CBitmap* pob = pMDC->SelectObject(pb);

		//	pMDC->SetStretchBltMode(HALFTONE);
		//	pngImage.StretchBlt(pMDC->m_hDC, 0, 0, new_width, new_height, 0, 0, pngImage.GetWidth(), pngImage.GetHeight(), SRCCOPY);

		//	pMDC->SelectObject(pob);

		//	CRect rect;
		//	CString img_file_name(file_names[i].c_str());
		//	CImageInfo i(pb, img_file_name);
		//	i.is_checked = pView->IsFileChecked(img_file_name);
		//	pView->m_arrImages.push_back(i);
		//	if (pView->m_arrImages.size() == 1)
		//	{
		//		pView->SetCurrentFile(0, pFrame);
		//	}

		//	TRACE("ImageFillerThread\n");
		//	pView->Invalidate();
		//}
	}

	if (pFrame)
	{
		if (pView->m_iPrevFileIndex > -1)
		{
			pView->SetCurrentFile(pView->m_iPrevFileIndex, pFrame);
			pView->m_iPrevFileIndex = -1;
		}
		temp_string.Format("%s Loaded", folder);
		pFrame->SetStatusBar(temp_string);
		
		if (pView->m_arrRectCode.size() > 0)
		{
			pView->m_arrOCRText.resize(pView->m_arrImages.size());
			pView->m_arrOCRPrevCount.resize(pView->m_arrImages.size(), 0);
			pView->m_arrRectCode.resize(pView->m_arrImages.size(), pView->m_arrRectCode[0]);
		}
		else
		{
			pView->m_arrOCRText.resize(pView->m_arrImages.size());
			pView->m_arrOCRPrevCount.resize(pView->m_arrImages.size(), 0);
			pView->m_arrRectCode.resize(pView->m_arrImages.size());
		}

		pView->InitPageNum();

		pFrame->m_wndTable.m_Dialog.m_Grid.SetNumberRows(pView->m_arrImages.size());
		pFrame->m_wndTable.m_Dialog.m_Grid.SetSH_NumberCols(1);
		pFrame->m_wndTable.m_Dialog.m_Grid.QuickSetText(-1, -1, "File Name");
		for(int index=0; index< pView->m_arrImages.size(); index++)
			pFrame->m_wndTable.m_Dialog.m_Grid.QuickSetText(-1, index, pView->m_arrImages[index].file_path);
		pView->InitGrid();
	}
	// done adding items
	ReleaseSemaphore(imageFillerWait, 1, NULL);

	ExitThread(0);
	return 0;
}

// thumbnail adder thread
UINT ImageApplyThread(void* param)
{
	// get thread parameters
	threadParamImage* pParam = (threadParamImage*)param;
	CString folder = pParam->folder;
	CAutoSplitPagesView* pView = pParam->pView;
	CMainFrame* pFrame = pParam->pFrame;
	HANDLE readPipe = pParam->readPipe;
	bool use_code = pParam->use_code;
	bool use_mask = pParam->use_mask;
	// cleanup
	delete pParam;
	CString temp_string;

	// wait for previous copies to stop
	WaitForSingleObject(imageFillerWait, INFINITE);

	// clear previous images from list control
	//pView->m_List.DeleteAllItems();
	//pView->item_names.clear();
	//TRACE("delete image\n");
	//::EnterCriticalSection(&(pView->m_cx));
	//vector< CBitmap* > temp_images;
	//for each(auto img in pView->m_arrImages)
	//{
	//	temp_images.push_back(img.image);
	//	//delete img.image;
	//}
	//pView->m_arrImages.clear();
	//::LeaveCriticalSection(&(pView->m_cx));

	//for each(auto img in temp_images)
	//{
	//	delete img;
	//}

	//if (pFrame)
	//{
	//	temp_string.Format("%s Loading...", folder);
	//	pFrame->SetStatusBar(temp_string);
	//}

	//pView->Invalidate();
	// start scanning designated folder for thumbnails
	//WIN32_FIND_DATA fd;
	//HANDLE find;
	//BOOL ok = TRUE;
	//fd.dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_COMPRESSED |
	//	FILE_ATTRIBUTE_READONLY;
	//find = FindFirstFile(folder + "\\*.*", &fd);

	//// return if operation failed
	//if (find == INVALID_HANDLE_VALUE)
	//{
	//	ReleaseSemaphore(imageFillerWait, 1, NULL);
	//	ExitThread(0);
	//	return 0;
	//}

	// critical section ends here
	ReleaseSemaphore(imageFillerCR, 1, NULL);
	CString file_name = folder;
	int index = folder.ReverseFind('\\');
	if (index > 0)
	{
		file_name = file_name.Right(file_name.GetLength() - index - 1);
	}
	CRect rect1;
	if (pView->m_bMultiCode == false)
	{
		if (pView->m_arrRectCode.size() > 0)
		{
			if (pView->m_arrRectCode[0].size() > 0)
			{
				rect1.SetRect(
					(double)pView->m_arrRectCode[0][0].first.left * pView->m_dThumbnailResolution,
					(double)pView->m_arrRectCode[0][0].first.top * pView->m_dThumbnailResolution,
					(double)pView->m_arrRectCode[0][0].first.right * pView->m_dThumbnailResolution,
					(double)pView->m_arrRectCode[0][0].first.bottom * pView->m_dThumbnailResolution
				);
			}
		}
	}
	CRect rect2;
	if (pView->m_bMultiMask == false)
	{
		if (pView->m_arrRectMask.size() > 0)
		{
			if (pView->m_arrRectMask[0].size() > 0)
			{
				rect2.SetRect(
					(double)pView->m_arrRectMask[0][0].left * pView->m_dThumbnailResolution,
					(double)pView->m_arrRectMask[0][0].top * pView->m_dThumbnailResolution,
					(double)pView->m_arrRectMask[0][0].right * pView->m_dThumbnailResolution,
					(double)pView->m_arrRectMask[0][0].bottom * pView->m_dThumbnailResolution
				);
			}
		}
	}
	//rect1.SetRect(
	//	(double)pFrame->m_wndProperties.m_Dialog.rectSelect1_image.left * pView->m_dThumbnailResolution,
	//	(double)pFrame->m_wndProperties.m_Dialog.rectSelect1_image.top * pView->m_dThumbnailResolution,
	//	(double)pFrame->m_wndProperties.m_Dialog.rectSelect1_image.right * pView->m_dThumbnailResolution,
	//	(double)pFrame->m_wndProperties.m_Dialog.rectSelect1_image.bottom * pView->m_dThumbnailResolution
	//);
	//rect2.SetRect(
	//	(double)pFrame->m_wndProperties.m_Dialog.rectSelect2_image.left * pView->m_dThumbnailResolution,
	//	(double)pFrame->m_wndProperties.m_Dialog.rectSelect2_image.top * pView->m_dThumbnailResolution,
	//	(double)pFrame->m_wndProperties.m_Dialog.rectSelect2_image.right * pView->m_dThumbnailResolution,
	//	(double)pFrame->m_wndProperties.m_Dialog.rectSelect2_image.bottom * pView->m_dThumbnailResolution
	//);

	CString check_file_path;
	for (int i = 0; i < pView->m_arrImages.size(); i++)
	{
		bool current_use_code = false;
		bool current_use_mask = false;
		if (use_code)
		{
			if (use_mask)
			{
				//	folder - folder
				current_use_code = true;
				current_use_mask = true;
			}
			else
			{
				//	folder - current
				if (i == pView->m_iCurrentFileIndex)
				{
					current_use_code = true;
					current_use_mask = true;
				}
				else
				{
					current_use_code = true;
					current_use_mask = false;
				}
			}
		}
		else
		{
			if (use_mask)
			{
				//	current - folder
				if (i == pView->m_iCurrentFileIndex)
				{
					current_use_code = true;
					current_use_mask = true;
				}
				else
				{
					current_use_code = false;
					current_use_mask = true;
				}
			}
			else
			{
				//	current - current
				if (i == pView->m_iCurrentFileIndex)
				{
					current_use_code = true;
					current_use_mask = true;
				}
			}
		}

		CBitmap pngBmp;
		CImage pngImage;
		check_file_path.Format("%s\\%s", folder, pView->m_arrImages[i].file_path);
		if (!FAILED(pngImage.Load(check_file_path)))
		{
			CDC dc;
			dc.Attach(GetDC(pView->m_hWnd));

			CDC* pMDC = new CDC;
			pMDC->CreateCompatibleDC(&dc);

			CBitmap* pb = new CBitmap;
			int new_width = (double)pngImage.GetWidth() * pView->m_dThumbnailResolution;
			int new_height = (double)pngImage.GetHeight() * pView->m_dThumbnailResolution;
			pb->CreateCompatibleBitmap(&dc, new_width, new_height);

			CBitmap* pob = pMDC->SelectObject(pb);

			pMDC->SetStretchBltMode(HALFTONE);
			pngImage.StretchBlt(pMDC->m_hDC, 0, 0, new_width, new_height, 0, 0, pngImage.GetWidth(), pngImage.GetHeight(), SRCCOPY);
			//pngImage.BitBlt(pMDC->m_hDC, 0, 0, SRCCOPY);

			CBrush brushBlue(RGB(255, 255, 255));
			CBrush* pOldBrush = pMDC->SelectObject(&brushBlue);
			CPen penBlack;
			penBlack.CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
			CPen* pOldPen = pMDC->SelectObject(&penBlack);
			if (current_use_code)
			{
				if (pView->m_bMultiCode)
				{
					if (pView->m_arrRectCode.size() > i)
					{
						for (int j = 0; j < m_pCurrentView->m_arrRectCode[i].size(); j++)
						{
							rect1.SetRect(
								(double)pView->m_arrRectCode[i][j].first.left * pView->m_dThumbnailResolution,
								(double)pView->m_arrRectCode[i][j].first.top * pView->m_dThumbnailResolution,
								(double)pView->m_arrRectCode[i][j].first.right * pView->m_dThumbnailResolution,
								(double)pView->m_arrRectCode[i][j].first.bottom * pView->m_dThumbnailResolution
							);

							pMDC->Rectangle(rect1);
							if (pView->m_bUsingFolderName_Code)
							{
								CFont font;
								font.CreatePointFont(pView->m_iFontSize, "Arial", pMDC);
								CFont* pOldFont = pMDC->SelectObject(&(font));

								pMDC->SetTextColor(RGB(0, 0, 0));
								pMDC->DrawText(file_name, rect1, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
							}
							else
							{
								CString file_list[5] = { "logo.bmp", "logo.png", "logo.gif", "logo.jpg", "logo.jpeg" };
								for (int temp_i = 0; temp_i < 5; temp_i++)
								{
									CString logo_path = m_strAppPath + "\\" + file_list[temp_i];
									if (PathFileExists(logo_path))
									{
										CImage logo;
										if (!FAILED(logo.Load(logo_path)))
										{
											int l_width = pngImage.GetWidth();
											int l_height = pngImage.GetHeight();
											logo.StretchBlt(pMDC->m_hDC, rect1, SRCCOPY);
											break;
										}
									}
								}
							}
						}
					}
				}
				else
				{
					pMDC->Rectangle(rect1);
					if (pView->m_bUsingFolderName_Code)
					{
						CFont font;
						font.CreatePointFont(pView->m_iFontSize, "Arial", pMDC);
						CFont* pOldFont = pMDC->SelectObject(&(font));

						pMDC->SetTextColor(RGB(0, 0, 0));
						pMDC->DrawText(file_name, rect1, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
					}
					else
					{
						CString file_list[5] = { "logo.bmp", "logo.png", "logo.gif", "logo.jpg", "logo.jpeg" };
						for (int temp_i = 0; temp_i < 5; temp_i++)
						{
							CString logo_path = m_strAppPath + "\\" + file_list[temp_i];
							if (PathFileExists(logo_path))
							{
								CImage logo;
								if (!FAILED(logo.Load(logo_path)))
								{
									int l_width = pngImage.GetWidth();
									int l_height = pngImage.GetHeight();
									logo.StretchBlt(pMDC->m_hDC, rect1, SRCCOPY);
									break;
								}
							}
						}
					}
				}
			}

			if (current_use_mask)
			{
				if (pView->m_bMultiMask)
				{
					if (pView->m_arrRectMask.size() > i)
					{
						for (int j = 0; j < m_pCurrentView->m_arrRectMask[i].size(); j++)
						{
							rect2.SetRect(
								(double)pView->m_arrRectMask[i][j].left * pView->m_dThumbnailResolution,
								(double)pView->m_arrRectMask[i][j].top * pView->m_dThumbnailResolution,
								(double)pView->m_arrRectMask[i][j].right * pView->m_dThumbnailResolution,
								(double)pView->m_arrRectMask[i][j].bottom * pView->m_dThumbnailResolution
							);
							pMDC->Rectangle(rect2);
						}
					}
				}
				else
				{
					pMDC->Rectangle(rect2);
				}
			}

			pMDC->SelectObject(pob);

			if (pView->m_arrImages[i].result_image != NULL)
				delete pView->m_arrImages[i].result_image;
			pView->m_arrImages[i].result_image = pb;

			TRACE("ImageFillerThread\n");
			pView->Invalidate();
		}
	}

	//if (pFrame)
	//{
	//	temp_string.Format("%s Loaded", folder);
	//	pFrame->SetStatusBar(temp_string);
	//}
	// done adding items
	ReleaseSemaphore(imageFillerWait, 1, NULL);

	ExitThread(0);
	return 0;
}

void GetOCRText(CString input_path, CString output_path, int type)
{
	if (PathFileExists(input_path))
	{
		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		ZeroMemory(&si, sizeof(STARTUPINFO));
		si.cb = sizeof(STARTUPINFO);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.hStdOutput = NULL;
		si.hStdInput = NULL;
		si.hStdError = NULL;
		si.wShowWindow = SW_HIDE;       /* 눈에 보이지 않는 상태로 프로세스 시작 */

		_wsetlocale(LC_ALL, L"korean");

		//	folder가 없으면 이미지 생성
		CString command_string;
		HANDLE hProcess;
		if (use_ai_ocr)
		{
			command_string.Format(_T("AIOCRApp.exe \"%s\" \"%s\" %d"), input_path, output_path, type);
		}
		else
		{
			command_string.Format(_T("VisionOCRApp.exe \"%s\" \"%s\" %d"), input_path, output_path, type);
		}
		//DWORD ret = CreateProcess(NULL, command_string.GetBuffer(), NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &FTPServer_pi);
		DWORD ret = CreateProcess(NULL, command_string.GetBuffer(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
		if (ret)
		{
			hProcess = pi.hProcess;

			WaitForSingleObject(hProcess, 0xffffffff);
			CloseHandle(hProcess);
		}
	}
}


IMPLEMENT_DYNCREATE(CAutoSplitPagesView, CScrollView)

BEGIN_MESSAGE_MAP(CAutoSplitPagesView, CScrollView)
	// 표준 인쇄 명령입니다.
	ON_COMMAND(ID_FILE_PRINT, &CScrollView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CScrollView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CAutoSplitPagesView::OnFilePrintPreview)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
	ON_WM_CREATE()
	ON_WM_SIZE()
	//ON_NOTIFY(LVN_ITEMCHANGED, 4, OnItemchangedList)
	//ON_COMMAND(ID_CHECK_IMAGES, &CAutoSplitPagesView::OnCheckImages)
	ON_COMMAND(ID_SPLIT_PAGES, &CAutoSplitPagesView::OnSplitPages)
	ON_COMMAND(ID_BUTTON_STOP, &CAutoSplitPagesView::OnButtonStop)
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEWHEEL()
	ON_COMMAND(ID_SLIDER_SIZE, &CAutoSplitPagesView::OnSliderSize)
	ON_WM_LBUTTONDOWN()
	ON_COMMAND(ID_SLIDER_RESOLUTION, &CAutoSplitPagesView::OnSliderResolution)
	ON_COMMAND(ID_SPIN_SIZE, &CAutoSplitPagesView::OnSpinSize)
	ON_COMMAND(ID_SPIN_RESOLUTION, &CAutoSplitPagesView::OnSpinResolution)
	ON_WM_LBUTTONDBLCLK()
	ON_COMMAND(ID_CHECK_SUBFOLDER, &CAutoSplitPagesView::OnCheckSubfolder)
	ON_UPDATE_COMMAND_UI(ID_CHECK_SUBFOLDER, &CAutoSplitPagesView::OnUpdateCheckSubfolder)
	ON_COMMAND(ID_EDIT_CHECK_VALUE, &CAutoSplitPagesView::OnEditCheckValue)
	ON_COMMAND(ID_CHECK_USE_FOLERNAME, &CAutoSplitPagesView::OnCheckUseFolername)
	ON_COMMAND(ID_CHECK_LOGO, &CAutoSplitPagesView::OnCheckLogo)
	ON_COMMAND(ID_CHECK_CURRENT, &CAutoSplitPagesView::OnCheckCurrent)
	ON_COMMAND(ID_CHECK_FOLDER, &CAutoSplitPagesView::OnCheckFolder)
	ON_COMMAND(ID_BUTTON_CODE_COLOR, &CAutoSplitPagesView::OnButtonCodeColor)
	ON_UPDATE_COMMAND_UI(ID_CHECK_USE_FOLERNAME, &CAutoSplitPagesView::OnUpdateCheckUseFolername)
	ON_UPDATE_COMMAND_UI(ID_CHECK_LOGO, &CAutoSplitPagesView::OnUpdateCheckLogo)
	ON_UPDATE_COMMAND_UI(ID_CHECK_CURRENT, &CAutoSplitPagesView::OnUpdateCheckCurrent)
	ON_UPDATE_COMMAND_UI(ID_CHECK_FOLDER, &CAutoSplitPagesView::OnUpdateCheckFolder)
	ON_COMMAND(ID_CHECK_MASK_CURRENT, &CAutoSplitPagesView::OnCheckMaskCurrent)
	ON_UPDATE_COMMAND_UI(ID_CHECK_MASK_CURRENT, &CAutoSplitPagesView::OnUpdateCheckMaskCurrent)
	ON_COMMAND(ID_CHECK_MASK_FOLDER, &CAutoSplitPagesView::OnCheckMaskFolder)
	ON_UPDATE_COMMAND_UI(ID_CHECK_MASK_FOLDER, &CAutoSplitPagesView::OnUpdateCheckMaskFolder)
	ON_COMMAND(ID_BUTTON_MASK_COLOR, &CAutoSplitPagesView::OnButtonMaskColor)
	ON_COMMAND(ID_BUTTON_PROCESS, &CAutoSplitPagesView::OnButtonProcess)
	ON_COMMAND(ID_COMBO_FONT, &CAutoSplitPagesView::OnComboFont)
	ON_COMMAND(ID_EDIT_FONT_SIZE, &CAutoSplitPagesView::OnEditFontSize)
	ON_COMMAND(ID_BUTTON_APPLY, &CAutoSplitPagesView::OnButtonApply)
	ON_COMMAND(ID_CHECK_APPLY, &CAutoSplitPagesView::OnCheckApply)
	ON_UPDATE_COMMAND_UI(ID_CHECK_APPLY, &CAutoSplitPagesView::OnUpdateCheckApply)
	ON_COMMAND(ID_FILE_VIEW, &CAutoSplitPagesView::OnFileView)
	ON_UPDATE_COMMAND_UI(ID_FILE_VIEW, &CAutoSplitPagesView::OnUpdateFileView)
	ON_COMMAND(ID_RESULT, &CAutoSplitPagesView::OnResult)
	ON_UPDATE_COMMAND_UI(ID_RESULT, &CAutoSplitPagesView::OnUpdateResult)
	ON_COMMAND(ID_STANDARD, &CAutoSplitPagesView::OnStandard)
	ON_UPDATE_COMMAND_UI(ID_STANDARD, &CAutoSplitPagesView::OnUpdateStandard)
	ON_COMMAND(ID_IMAGE1, &CAutoSplitPagesView::OnImage1)
	ON_UPDATE_COMMAND_UI(ID_IMAGE1, &CAutoSplitPagesView::OnUpdateImage1)
	ON_COMMAND(ID_IMAGE2, &CAutoSplitPagesView::OnImage2)
	ON_UPDATE_COMMAND_UI(ID_IMAGE2, &CAutoSplitPagesView::OnUpdateImage2)
	ON_COMMAND(ID_CHECK_MULTI_CODE, &CAutoSplitPagesView::OnCheckMultiCode)
	ON_UPDATE_COMMAND_UI(ID_CHECK_MULTI_CODE, &CAutoSplitPagesView::OnUpdateCheckMultiCode)
	ON_COMMAND(ID_CHECK_MULTI_MASK, &CAutoSplitPagesView::OnCheckMultiMask)
	ON_UPDATE_COMMAND_UI(ID_CHECK_MULTI_MASK, &CAutoSplitPagesView::OnUpdateCheckMultiMask)
	ON_COMMAND(ID_CHECK_RESET_FOLDER, &CAutoSplitPagesView::OnCheckResetFolder)
	ON_UPDATE_COMMAND_UI(ID_CHECK_RESET_FOLDER, &CAutoSplitPagesView::OnUpdateCheckResetFolder)
	ON_COMMAND(ID_BUTTON_OCR, &CAutoSplitPagesView::OnButtonOCR_Select_all)
	ON_COMMAND(ID_BUTTON_OCR_PAGE, &CAutoSplitPagesView::OnButtonOCR_Select_page)
	ON_COMMAND(ID_BUTTON_OCR_PAGE_TEXT, &CAutoSplitPagesView::OnButtonOCR_Page_all)
	ON_COMMAND(ID_BUTTON_OCR_PAGE_TEXT_PAGE, &CAutoSplitPagesView::OnButtonOCR_Page_page)
	ON_COMMAND(ID_BUTTON_SAVE_EXCEL, &CAutoSplitPagesView::OnButtonSaveExcel)
	ON_COMMAND(ID_SPIN_PAGE_NUM, &CAutoSplitPagesView::OnSpinPageNum)
	ON_COMMAND(ID_BUTTON_SEL_CLEAR, &CAutoSplitPagesView::OnButtonSelClear)
	ON_COMMAND(ID_BUTTON_OCR_PAGE_TEXT_SUB, &CAutoSplitPagesView::OnButtonOcrPageTextSub)
	ON_COMMAND(ID_CHECK_PRESERVE_EXSITING, &CAutoSplitPagesView::OnCheckPreserveExsiting)
	ON_UPDATE_COMMAND_UI(ID_CHECK_PRESERVE_EXSITING, &CAutoSplitPagesView::OnUpdateCheckPreserveExsiting)
	ON_COMMAND(ID_CHECK_ALL, &CAutoSplitPagesView::OnCheckAll)
	ON_COMMAND(ID_CHECK_CURRENT, &CAutoSplitPagesView::OnCheckCur)
	ON_COMMAND(ID_CHECK_SEL, &CAutoSplitPagesView::OnCheckSel)
	ON_UPDATE_COMMAND_UI(ID_CHECK_ALL, &CAutoSplitPagesView::OnUpdateCheckAll)
	ON_UPDATE_COMMAND_UI(ID_CHECK_CURRENT, &CAutoSplitPagesView::OnUpdateCheckCur)
	ON_UPDATE_COMMAND_UI(ID_CHECK_SEL, &CAutoSplitPagesView::OnUpdateCheckSel)
	ON_COMMAND(ID_CHECK_CURRENT_PAGE, &CAutoSplitPagesView::OnCheckCurrentPage)
	ON_UPDATE_COMMAND_UI(ID_CHECK_CURRENT_PAGE, &CAutoSplitPagesView::OnUpdateCheckCurrentPage)
	ON_MESSAGE(WM_GRID_AUTO_CHECK, &CAutoSplitPagesView::OnGridAutoCheck)
	ON_COMMAND(ID_BUTTON_OCR_SEL, &CAutoSplitPagesView::OnButtonOcrSel)
END_MESSAGE_MAP()

// CAutoSplitPagesView 생성/소멸

CAutoSplitPagesView::CAutoSplitPagesView() noexcept
	: m_dMaxThumbnailWidth(256.0)
	, m_dMinThumbnailWidth(16.0)
	, m_dThumbnailWidth(128.0)
	, m_dThumbnailResolution(0.2)
	, m_dSpaceBetween(10.0)
	, m_dLeftSpace(16.0)
	, m_dTopSpace(16.0)
	, m_dTextSpace(16.0)
	, m_bFindSubFolder(false)
	, m_iCurrentFileIndex(-1)
	, m_iPrevFileIndex(-1)
	, m_bApplyFolder_Code(true)
	, m_bApplyFolder_Mask(true)
	, m_bUsingFolderName_Code(true)
	, m_bViewThumbnailView(false)
	, m_bMultiCode(true)
	, m_iSelPageType(0)
{
	// TODO: 여기에 생성 코드를 추가합니다.
	::InitializeCriticalSection(&m_cx);
}

CAutoSplitPagesView::~CAutoSplitPagesView()
{
	::DeleteCriticalSection(&m_cx);

	if (m_DlgProgress)
	{
		delete m_DlgProgress;
		m_DlgProgress = NULL;
	}
}

BOOL CAutoSplitPagesView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: CREATESTRUCT cs를 수정하여 여기에서
	//  Window 클래스 또는 스타일을 수정합니다.

	return CScrollView::PreCreateWindow(cs);
}

void CAutoSplitPagesView::_DrawBackgound(CMemoryDC* pDCDraw, const CRect& rcClient)
{
	pDCDraw->SelectStockObject(WHITE_BRUSH);
	pDCDraw->PatBlt(rcClient.left, rcClient.top, rcClient.Width(), rcClient.Height(), PATCOPY);
}

// Draw thumbnails. Please see the notes at the top of this source code. 
void CAutoSplitPagesView::_DrawThumbnails(CMemoryDC* pDCDraw, const CRect& rcClient)
{
	CDrawingManager draw(*pDCDraw);

	TRACE("_DrawThumbnails >>\n");
	::EnterCriticalSection(&m_cx);
	for (int nIndex = 0; nIndex < m_arrImages.size(); nIndex++)
	{

		CRect rcImage;
		CRect rcText;
		_GetThumbnailRect(nIndex, rcImage, rcText);
		CBitmap* pBitmap = m_arrImages[nIndex].image;
		if (m_bViewThumbnailView && (m_arrImages[nIndex].result_image != NULL))
		{
			pBitmap = m_arrImages[nIndex].result_image;
		}

		if (!rcImage.IsRectEmpty())
		{
			m_arrImages[nIndex].rect = rcImage;
			if (m_arrImages[nIndex].is_checked)
			{
				CString temp_string;
				temp_string.Format("<< %s >>", m_arrImages[nIndex].file_path);
				CMemoryDC dcMem(pDCDraw, pBitmap, rcImage, temp_string, rcText);
			}
			else
			{
				CMemoryDC dcMem(pDCDraw, pBitmap, rcImage, m_arrImages[nIndex].file_path, rcText);
			}
			if (nIndex == m_iCurrentFileIndex)
			{
				draw.DrawRect(rcImage, (COLORREF)-1, RGB(255, 0, 0));
			}
			if (m_arrImages[nIndex].is_checked)
			{
				draw.DrawRect(rcImage, (COLORREF)-1, RGB(255, 0, 0));
			}
			else
			{
				//draw.DrawRect(rcImage, (COLORREF)-1, RGB(255, 0, 0));
			}
		}
		// draw shadow
		//draw.DrawShadow(rcImage, m_nShadowDepth);
	}
	::LeaveCriticalSection(&m_cx);
	TRACE("_DrawThumbnails <<\n");
}

UINT nThumbnailCount = 0;
UINT nColCount = 0;
UINT nRowCount = 0;

UINT CAutoSplitPagesView::_GetColCount()
{
	CRect rcClient;
	GetClientRect(rcClient);
	return static_cast<UINT>(rcClient.Width() / (m_dThumbnailWidth + m_dSpaceBetween));
}

UINT CAutoSplitPagesView::_GetThumbnailCount()
{
	return (int)m_arrImages.size();
}

void CAutoSplitPagesView::_GetThumbnailRect(UINT nIndex, CRect& rc, CRect& rc_text)
{
	nColCount = _GetColCount();
	if (0 == nColCount)
	{
		rc.SetRectEmpty();
		return;
	}
	const CPoint ptScrollPos = GetScrollPosition();

	const UINT nCol = nIndex % nColCount;
	const UINT nRow = nIndex / nColCount;
	const UINT nLeft = static_cast<UINT>(m_dLeftSpace + (m_dThumbnailWidth + m_dSpaceBetween) * nCol) - ptScrollPos.x;
	double dThumbnailHeight = m_dThumbnailWidth;
	const UINT nTop = static_cast<UINT>(m_dTopSpace + (dThumbnailHeight + m_dSpaceBetween + m_dTextSpace) * nRow) - ptScrollPos.y;

	rc.SetRect(nLeft, nTop, static_cast<UINT>(nLeft + m_dThumbnailWidth), static_cast<UINT>(nTop + dThumbnailHeight));
	rc_text.SetRect(nLeft, static_cast<UINT>(nTop + dThumbnailHeight), static_cast<UINT>(nLeft + m_dThumbnailWidth), static_cast<UINT>(nTop + dThumbnailHeight + m_dTextSpace));
}

CSize CAutoSplitPagesView::_GetVirtualViewSize()
{
	CSize size(100, 100);
	nThumbnailCount = _GetThumbnailCount();
	if (nThumbnailCount > 0)
	{
		nColCount = _GetColCount();
		if (nColCount > 0)
		{
			size.cx = static_cast<LONG>(m_dLeftSpace + (m_dThumbnailWidth * nColCount));
			nRowCount = nThumbnailCount / nColCount + 1;
			double dThumbnailHeight = m_dThumbnailWidth + m_dSpaceBetween + m_dTextSpace;
			size.cy = static_cast<LONG>(m_dTopSpace + (dThumbnailHeight* nRowCount));
		}
	}
	return size;
}

// CAutoSplitPagesView 그리기

void CAutoSplitPagesView::OnDraw(CDC* pDCView)
{
	//CAutoSplitPagesDoc* pDoc = GetDocument();
	//ASSERT_VALID(pDoc);
	//if (!pDoc)
	//	return;

	ASSERT(MM_TEXT == pDCView->GetMapMode());
	pDCView->SelectClipRgn(NULL);
	CRect rcClient(0, 0, 0, 0);
	GetClientRect(rcClient);

	if (rcClient.Width() > 0 && rcClient.Height() > 0)
	{
		CMemoryDC dcDraw(pDCView, rcClient);
		_GetVirtualViewSize();
		_DrawBackgound(&dcDraw, rcClient);
		_DrawThumbnails(&dcDraw, rcClient);
	}
}


// CAutoSplitPagesView 인쇄


void CAutoSplitPagesView::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
	AFXPrintPreview(this);
#endif
}

BOOL CAutoSplitPagesView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// 기본적인 준비
	return DoPreparePrinting(pInfo);
}

void CAutoSplitPagesView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: 인쇄하기 전에 추가 초기화 작업을 추가합니다.
}

void CAutoSplitPagesView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: 인쇄 후 정리 작업을 추가합니다.
}

void CAutoSplitPagesView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CAutoSplitPagesView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}


// CAutoSplitPagesView 진단

#ifdef _DEBUG
void CAutoSplitPagesView::AssertValid() const
{
	CScrollView::AssertValid();
}

void CAutoSplitPagesView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}

CAutoSplitPagesDoc* CAutoSplitPagesView::GetDocument() const // 디버그되지 않은 버전은 인라인으로 지정됩니다.
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CAutoSplitPagesDoc)));
	return (CAutoSplitPagesDoc*)m_pDocument;
}
#endif //_DEBUG


// CAutoSplitPagesView 메시지 처리기


int CAutoSplitPagesView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CScrollView::OnCreate(lpCreateStruct) == -1)
		return -1;

	//m_List.Create(WS_CHILD | WS_VISIBLE | LVS_ICON, CRect(0, 0, 0, 0), this, 4);

	//// Create the image list with 100*100 icons and 32 bpp color depth
	//HIMAGELIST hImageList = ImageList_Create(100, 100, ILC_COLOR32, 0, 10);
	//m_imageList.Attach(hImageList);

	//// load the starting bitmap ("Loading..." and "Corrupt file")
	//CBitmap dummy;
	//dummy.LoadBitmap(IDB_MAIN);
	//m_imageList.Add(&dummy, RGB(0, 0, 0));

	//// Use the image list in the list view
	//m_List.SetImageList(&m_imageList, LVSIL_NORMAL);
	//m_List.SetImageList(&m_imageList, LVSIL_SMALL);

	return 0;
}


void CAutoSplitPagesView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{
	CScrollView::OnPrepareDC(pDC, pInfo);
	pDC->SetMapMode(MM_TEXT);           // force map mode to MM_TEXT
	pDC->SetViewportOrg(CPoint(0, 0));  // force view-port origin to zero
}


void CAutoSplitPagesView::OnSize(UINT nType, int cx, int cy)
{
	CSize size = _GetVirtualViewSize();
	SetScrollSizes(MM_TEXT, size);
	//CScrollView::OnSize(nType, cx, cy);

	//if (m_List)
	//{
	//	m_List.MoveWindow(0, 0, cx, cy);
	//	m_List.Arrange(LVA_DEFAULT);
	//}
}


BOOL CAutoSplitPagesView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (MK_CONTROL & nFlags)
	{
		if (zDelta > 0)
			m_dThumbnailWidth = min(m_dMaxThumbnailWidth, (m_dThumbnailWidth + 8));
		else
			m_dThumbnailWidth = max(m_dMinThumbnailWidth, (m_dThumbnailWidth - 8));
		Invalidate();
		//AfxGetMainWnd()->PostMessage(CThumbnailDemoApp::WM_APP_CHANGE_THUMBNAIL_SIZE, (WPARAM)(UINT)m_dThumbnailWidth);
		return FALSE;
	}

	return CScrollView::OnMouseWheel(nFlags, zDelta, pt);
}

// Fill in list control with thumbails from a specified folder
BOOL CAutoSplitPagesView::FillInImages(CString folder)
{
	if (m_bAutoResetMulti)
	{
		m_arrOCRPrevCount.clear();
		m_arrOCRText.clear();
		m_arrRectCode.clear();
		m_arrRectMask.clear();
		m_mapCodeImageFile.clear();
	}
	SetCurrentFile(-1, m_pFrame);
	WriteStringValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("CURRENT_PATH"), folder);

	// create semaphores the first time only
	if (!imageFillerSemaphore)
	{
		imageFillerSemaphore = CreateSemaphore(NULL, 0, 1, NULL);
		imageFillerCR = CreateSemaphore(NULL, 1, 1, NULL);
		imageFillerWait = CreateSemaphore(NULL, 1, 1, NULL);
	}

	// critical region starts here
	WaitForSingleObject(imageFillerCR, 0);

	m_strCurrentFolder = folder;
	// create thread parameters
	threadParamImage* pParam = new threadParamImage;
	pParam->folder = folder;
	pParam->pView = this;
	pParam->pFrame = m_pFrame;

	// and the thread
	DWORD dummy;
	imageFiller = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ImageFillerThread,
		pParam, 0, &dummy);

	return TRUE;
}

//void CAutoSplitPagesView::OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult)
//{
//	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
//
//	if ((pNMListView->uChanged & LVIF_STATE)
//		&& (pNMListView->uNewState & LVIS_SELECTED))
//	{
//		CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
//		if (pFrame)
//		{
//			m_iCurrentSelectedIndex = pNMListView->iItem;
//			pFrame->m_wndProperties.m_Dialog.ShowImage(m_strCurrentFolder + "\\" + item_names[pNMListView->iItem].c_str());
//		}
//	}
//}

//void CAutoSplitPagesView::OnCheckImages()
//{
//	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
//	if (pFrame)
//	{
//		m_bStopChecking = false;
//		Mat templ;
//		TCHAR file_path[MAX_PATH];
//		sprintf_s(file_path, "%stemp.png", m_strTempPath);
//		templ = imread(file_path, 1);
//		DoCheckImages(m_strCurrentFolder, templ);
//
//		ReadCheckInfo(m_strCurrentFolder);
//		for (int i = 0; i < m_arrImages.size(); i++)
//		{
//			m_arrImages[i].is_checked = IsFileChecked(m_arrImages[i].file_path);
//		}
//		Invalidate();
//	}
//	AfxMessageBox("Process Completed");
//}
//
//void CAutoSplitPagesView::DoCheckImages(CString folder, Mat& templ)
//{
//	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
//	if (pFrame)
//	{
//		WIN32_FIND_DATA fd;
//		HANDLE find;
//		BOOL ok = TRUE;
//		fd.dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_COMPRESSED |
//			FILE_ATTRIBUTE_READONLY;
//		find = FindFirstFile(folder + "\\*.*", &fd);
//
//		// return if operation failed
//		if (find == INVALID_HANDLE_VALUE)
//		{
//			pFrame->m_wndOutput.AddString(folder + " is not valid");
//
//			return;
//		}
//
//		pFrame->m_wndOutput.AddString("Checking folder : " + folder);
//		CString temp_string;
//		//for (match_method = 0; match_method <= 5; match_method++)
//		//{
//		//	temp_string.Format("match_method : %d\n", match_method);
//		//	pFrame->m_wndOutput.AddString(temp_string);
//
//		Mat img;
//		Mat result;
//		int match_method = 5;
//		TCHAR file_path[MAX_PATH];
//		time_t start_time;
//		time(&start_time);
//		sprintf_s(file_path, "%s\\check_info", folder);
//		FILE* fp;
//		fopen_s(&fp, file_path, "wt");
//		vector< CString > sub_folders;
//		int file_count = 0;
//		vector< string > file_names;
//		// start adding items to the list control
//		do
//		{
//			if (m_bStopChecking) break;
//			ok = FindNextFile(find, &fd);
//			if (fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
//			{
//				if (fd.cFileName[0] != '.')
//					sub_folders.push_back(fd.cFileName);
//				continue;
//			}
//
//			if (ok)
//			{
//				file_names.push_back(fd.cFileName);
//			}
//		} while (find&&ok);
//
//		// done adding items
//		FindClose(find);
//		sort(file_names.begin(), file_names.end());
//		CString check_file_path;
//		for (int i = 0; i < file_names.size(); i++)
//		{
//			file_count++;
//			sprintf_s(file_path, "%s\\%s", folder, file_names[i].c_str());
//			//if (m_iCurrentSelectedIndex == i)
//			//{
//			//}
//			//else
//			//{
//			if (PathFileExists(file_path))
//			{
//				img = imread(file_path, 1);
//				if (!img.empty())
//				{
//
//					int result_cols = img.cols - templ.cols + 1;
//					int result_rows = img.rows - templ.rows + 1;
//					if (result_rows > 0 && result_cols > 0)
//					{
//						result.create(result_rows, result_cols, img.type());
//
//						matchTemplate(img, templ, result, match_method);
//						//normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());
//						/// Localizing the best match with minMaxLoc
//						double minVal; double maxVal; Point minLoc; Point maxLoc;
//						Point matchLoc;
//
//						minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());
//
//						if (maxVal >= m_dCheckValue)
//						{
//							//temp_string.Format("<< %s >>", item_names[i].c_str());
//							//m_List.SetItemText(i, 0, temp_string);
//							//item_header_pages.push_back(i);
//							temp_string.Format("[%d] %s : processed(%.2lf) >> checked", file_count, file_names[i].c_str(), maxVal);
//							fprintf(fp, "%s1\n", file_names[i].c_str());
//						}
//						else
//						{
//							//m_List.SetItemText(i, 0, item_names[i].c_str());
//							temp_string.Format("[%d] %s : processed(%.2lf)", file_count, file_names[i].c_str(), maxVal);
//							fprintf(fp, "%s0\n", file_names[i].c_str());
//						}
//
//						pFrame->m_wndOutput.AddString(temp_string);
//						Invalidate();
//
//						MSG msg;
//						while (::PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
//						{
//							::TranslateMessage(&msg);
//							::DispatchMessage(&msg);
//						}
//					}
//				}
//			}
//		}
//		fclose(fp);
//
//		time_t end_time;
//		time(&end_time);
//		double spend_time = difftime(end_time, start_time);
//		//TRACE("spend_time : %lf\n\n", spend_time);
//		temp_string.Format("spend_time : %lf\n\n", spend_time);
//		pFrame->m_wndOutput.AddString(temp_string);
//
//		if (m_bFindSubFolder)
//		{
//			for (int i = 0; i < sub_folders.size() && m_bStopChecking == false; i++)
//			{
//				DoCheckImages(folder + "\\" + sub_folders[i], templ);
//			}
//		}
//	}
//}

void CAutoSplitPagesView::OnSplitPages()
{
	OnSplitPages(m_strCurrentFolder);

	FillInImages(m_strCurrentFolder);
}

void CAutoSplitPagesView::OnSplitPages(CString folder)
{
	if (m_pFrame)
	{
		//	하위 폴더 수집
		WIN32_FIND_DATA fd;
		HANDLE find;
		BOOL ok = TRUE;
		fd.dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_COMPRESSED |
			FILE_ATTRIBUTE_READONLY;
		find = FindFirstFile(folder + "\\*.*", &fd);

		// return if operation failed
		if (find == INVALID_HANDLE_VALUE)
		{
			 m_pFrame->m_wndOutput.AddString(folder + " is not valid");

			return;
		}

		 m_pFrame->m_wndOutput.AddString("Split Pages folder : " + folder);

		TCHAR file_path[MAX_PATH];
		time_t start_time;
		time(&start_time);
		vector< CString > sub_folders;
		// start adding items to the list control
		do
		{
			if (m_bStopChecking) break;
			ok = FindNextFile(find, &fd);
			if (fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			{
				if (fd.cFileName[0] != '.')
					sub_folders.push_back(fd.cFileName);
				continue;
			}
		} while (find&&ok);

		// done adding items
		FindClose(find);

		//	현재 폴더에 포함된 파일 정리
		ReadCheckInfo(folder);
		CString temp_string;
		CString from_file;
		CString to_file;

		for (int i = 0; i < m_arrHeaderFiles.size(); i++)
		{
			if (m_arrHeaderFiles[i].second)
			{
				temp_string.Format("%s\\%s", folder, m_arrHeaderFiles[i].first);
				int dot_index = temp_string.ReverseFind('.');
				if (dot_index > 0)
				{
					temp_string = temp_string.Left(dot_index);
				}

				CreateDirectory(temp_string, NULL);
				from_file.Format("Create Folder : %s\n", temp_string);
				 m_pFrame->m_wndOutput.AddString(from_file);
			}

			if (temp_string != "")
			{
				from_file.Format("%s\\%s", folder, m_arrHeaderFiles[i].first);
				to_file.Format("%s\\%s", temp_string, m_arrHeaderFiles[i].first);

				MoveFile(from_file, to_file);
			}
		}

		time_t end_time;
		time(&end_time);
		double spend_time = difftime(end_time, start_time);
		//TRACE("spend_time : %lf\n\n", spend_time);
		temp_string.Format("spend_time : %lf\n\n", spend_time);
		 m_pFrame->m_wndOutput.AddString(temp_string);

		if (m_bFindSubFolder)
		{
			for (int i = 0; i < sub_folders.size() && m_bStopChecking == false; i++)
			{
				OnSplitPages(folder + "\\" + sub_folders[i]);
			}
		}
	}
}

void CAutoSplitPagesView::OnButtonStop()
{
	m_bStopChecking = true;
}


void CAutoSplitPagesView::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();
	CSize sizeTotal;
	// TODO: calculate the total size of this view

	m_DlgProgress = new CDlgProgress();
	m_DlgProgress->Create(IDD_DIALOG_PROGRESS);

	sizeTotal.cx = sizeTotal.cy = 100;
	SetScrollSizes(MM_TEXT, sizeTotal);

	CString current_path = ReadStringValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("CURRENT_PATH"));
	int i_resolution = (int)ReadDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("RESOLUTION"));
	if (i_resolution < 0)
		i_resolution = 20;
	m_dThumbnailResolution = (double)((int)i_resolution) / 100.0;
	int i_size = (int)ReadDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("SIZE"));
	if (i_size < 0)
		i_size = 100;
	m_dThumbnailWidth = i_size;
	int i_checkvalue = (int)ReadDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("CHECK_VALUE"));
	if (i_checkvalue < 0)
		i_checkvalue = 90;
	m_dCheckValue = (double)i_checkvalue / 100.0;

	m_bApplyFolder_Code = (bool)ReadDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("CODE_APPLY_FOLDER"));
	m_bApplyFolder_Mask = (bool)ReadDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("MASK_APPLY_FOLDER"));
	m_bUsingFolderName_Code = (bool)ReadDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("CODE_USE_FOLDERNAME"));
	m_bFindSubFolder = (bool)ReadDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("INCLUDE_SUB_FOLDER"));
	
	m_color1 = (COLORREF)ReadDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("CODE_BOX_COLOR"));
	m_color2 = (COLORREF)ReadDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("MASK_BOX_COLOR"));

	int m_iFontSize = (int)ReadDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("CODE_FONT_SIZE"));
	if (m_iFontSize < 1)
	{
		m_iFontSize = 1;
	}

	m_pFrame = (CMainFrame*)AfxGetMainWnd();
	m_pCurrentView = this;
	if ( m_pFrame)
	{
		CString temp_string;
		CMFCRibbonSlider* pSlider = DYNAMIC_DOWNCAST(CMFCRibbonSlider,  m_pFrame->m_wndRibbonBar.FindByID(ID_SLIDER_RESOLUTION));
		CMFCRibbonEdit* pSpin = DYNAMIC_DOWNCAST(CMFCRibbonEdit, m_pFrame->m_wndRibbonBar.FindByID(ID_SPIN_RESOLUTION));
		pSlider->SetPos(i_resolution);
		temp_string.Format("%d", i_resolution);
		pSpin->SetEditText(temp_string);

		pSlider = DYNAMIC_DOWNCAST(CMFCRibbonSlider, m_pFrame->m_wndRibbonBar.FindByID(ID_SLIDER_SIZE));
		pSpin = DYNAMIC_DOWNCAST(CMFCRibbonEdit, m_pFrame->m_wndRibbonBar.FindByID(ID_SPIN_SIZE));
		pSlider->SetPos(i_size);
		temp_string.Format("%d", i_size);
		pSpin->SetEditText(temp_string);

		pSpin = DYNAMIC_DOWNCAST(CMFCRibbonEdit, m_pFrame->m_wndRibbonBar.FindByID(ID_SPIN_PAGE_NUM));
		pSpin->SetEditText("1");
		m_iCurrentPageNum = 1;

		m_bPreserveExsiting = false;

		//pSpin = DYNAMIC_DOWNCAST(CMFCRibbonEdit, m_pFrame->m_wndRibbonBar.FindByID(ID_EDIT_CHECK_VALUE));
		//temp_string.Format("%.2lf", m_dCheckValue);
		//pSpin->SetEditText(temp_string);

		//CMFCRibbonColorButton* pColor = DYNAMIC_DOWNCAST(CMFCRibbonColorButton, m_pFrame->m_wndRibbonBar.FindByID(ID_BUTTON_CODE_COLOR));
		//if (pColor)
		//{
		//	pColor->SetColor(m_color1);
		//}
		//pColor = DYNAMIC_DOWNCAST(CMFCRibbonColorButton, m_pFrame->m_wndRibbonBar.FindByID(ID_BUTTON_MASK_COLOR));
		//if (pColor)
		//{
		//	pColor->SetColor(m_color2);
		//}

		//pSpin = DYNAMIC_DOWNCAST(CMFCRibbonEdit, m_pFrame->m_wndRibbonBar.FindByID(ID_EDIT_FONT_SIZE));
		//temp_string.Format("%d", m_iFontSize);
		//pSpin->SetEditText(temp_string);

		if (PathFileExists(current_path))
		{
			m_pFrame->m_wndFileView.SetFolder(current_path);
		}
		m_pFrame->m_wndRibbonBar.ForceRecalcLayout();
	}

	TCHAR lpTempPathBuffer[MAX_PATH];
	DWORD dwRetVal = GetTempPath(MAX_PATH,          // length of the buffer
		lpTempPathBuffer); // buffer for path 
	m_strTempFolder = lpTempPathBuffer;
}


BOOL CAutoSplitPagesView::OnEraseBkgnd(CDC* pDC)
{
	return TRUE; // perform all drawing in OnDraw.
}

// NOTE: this code similar to code from CScrollView::OnScrollBy
//       except that ScrollWindow is not called.
BOOL CAutoSplitPagesView::OnScrollBy(CSize sizeScroll, BOOL bDoScroll)
{
	int xOrig, x;
	int yOrig, y;

	// don't scroll if there is no valid scroll range (ie. no scroll bar)
	CScrollBar* pBar;
	DWORD dwStyle = GetStyle();
	pBar = GetScrollBarCtrl(SB_VERT);
	if ((pBar != NULL && !pBar->IsWindowEnabled()) ||
		(pBar == NULL && !(dwStyle & WS_VSCROLL)))
	{
		// vertical scroll bar not enabled
		sizeScroll.cy = 0;
	}
	pBar = GetScrollBarCtrl(SB_HORZ);
	if ((pBar != NULL && !pBar->IsWindowEnabled()) ||
		(pBar == NULL && !(dwStyle & WS_HSCROLL)))
	{
		// horizontal scroll bar not enabled
		sizeScroll.cx = 0;
	}

	// adjust current x position
	xOrig = x = GetScrollPos(SB_HORZ);
	int xMax = GetScrollLimit(SB_HORZ);
	x += sizeScroll.cx;
	if (x < 0)
		x = 0;
	else if (x > xMax)
		x = xMax;

	// adjust current y position
	yOrig = y = GetScrollPos(SB_VERT);
	int yMax = GetScrollLimit(SB_VERT);
	y += sizeScroll.cy;
	if (y < 0)
		y = 0;
	else if (y > yMax)
		y = yMax;

	// did anything change?
	if (x == xOrig && y == yOrig)
		return FALSE;

	if (bDoScroll)
	{
		// do scroll and update scroll positions
		/// ScrollWindow(-(x-xOrig), -(y-yOrig)); // <-- removed
		Invalidate(); // <-- added // Invalidates whole client area
		if (x != xOrig)
			SetScrollPos(SB_HORZ, x);
		if (y != yOrig)
			SetScrollPos(SB_VERT, y);
	}
	return TRUE;
}


void CAutoSplitPagesView::OnSliderSize()
{
	if (m_pFrame)
	{
		CMFCRibbonSlider* pSlider = DYNAMIC_DOWNCAST(CMFCRibbonSlider, m_pFrame->m_wndRibbonBar.FindByID(ID_SLIDER_SIZE));
		CMFCRibbonEdit* pSpin = DYNAMIC_DOWNCAST(CMFCRibbonEdit, m_pFrame->m_wndRibbonBar.FindByID(ID_SPIN_SIZE));
		m_dThumbnailWidth = pSlider->GetPos();
		CString temp_string;
		temp_string.Format("%d", (int)m_dThumbnailWidth);
		pSpin->SetEditText(temp_string);
		CSize size = _GetVirtualViewSize();
		SetScrollSizes(MM_TEXT, size);
		Invalidate();
		WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("SIZE"), (DWORD)(int)m_dThumbnailWidth);
	}
}


void CAutoSplitPagesView::OnLButtonDown(UINT nFlags, CPoint point)
{
	for (int i = 0; i < m_arrImages.size(); i++)
	{
		if (m_arrImages[i].rect.left <= point.x && m_arrImages[i].rect.right >= point.x &&
			m_arrImages[i].rect.top <= point.y && m_arrImages[i].rect.bottom >= point.y)
		{
			SetCurrentFile(i, m_pFrame);
		}
	}
	//CString temp_string;
	//temp_string.Format("%d, %d", point.x, point.y);
	//AfxMessageBox(temp_string);

	CScrollView::OnLButtonDown(nFlags, point);
}

void CAutoSplitPagesView::SetCurrentFile(int index, CMainFrame* pFrame)
{
	//CString temp_string;
	//temp_string.Format("set current file : %d", index);
	//pFrame->m_wndOutput.AddString(temp_string);
	if (index != m_iCurrentFileIndex)
	{
		m_iCurrentFileIndex = index;
		UpdatePageImages(index);
		SavePageCodeImage(index);
		Invalidate();
		if (pFrame)
		{
			if (m_iCurrentFileIndex > -1 && m_iCurrentFileIndex < m_arrImages.size())
			{
				pFrame->m_wndProperties.m_Dialog.ShowImage(m_strCurrentFolder + "\\" + m_arrImages[m_iCurrentFileIndex].file_path);
			}
			else
			{
				pFrame->m_wndProperties.m_Dialog.ShowImage("");
			}
		}
	}
}


void CAutoSplitPagesView::OnSliderResolution()
{
	if (m_pFrame)
	{
		CMFCRibbonSlider* pSlider = DYNAMIC_DOWNCAST(CMFCRibbonSlider, m_pFrame->m_wndRibbonBar.FindByID(ID_SLIDER_RESOLUTION));
		CMFCRibbonEdit* pSpin = DYNAMIC_DOWNCAST(CMFCRibbonEdit, m_pFrame->m_wndRibbonBar.FindByID(ID_SPIN_RESOLUTION));
		int i_val = pSlider->GetPos();
		CString temp_string;
		temp_string.Format("%d", i_val);
		pSpin->SetEditText(temp_string);
		m_dThumbnailResolution = (double)(i_val) / 100.0;
		WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("RESOLUTION"), (DWORD)(int)i_val);
		//Invalidate();
	}
}


void CAutoSplitPagesView::OnSpinSize()
{
	if (m_pFrame)
	{
		CMFCRibbonSlider* pSlider = DYNAMIC_DOWNCAST(CMFCRibbonSlider, m_pFrame->m_wndRibbonBar.FindByID(ID_SLIDER_SIZE));
		CMFCRibbonEdit* pSpin = DYNAMIC_DOWNCAST(CMFCRibbonEdit, m_pFrame->m_wndRibbonBar.FindByID(ID_SPIN_SIZE));
		CString temp_string = pSpin->GetEditText();
		m_dThumbnailWidth = atoi(temp_string);
		pSlider->SetPos(m_dThumbnailWidth);
		CSize size = _GetVirtualViewSize();
		SetScrollSizes(MM_TEXT, size);
		Invalidate();
		WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("SIZE"), (DWORD)(int)m_dThumbnailWidth);
	}
}


void CAutoSplitPagesView::OnSpinResolution()
{
	if (m_pFrame)
	{
		CMFCRibbonSlider* pSlider = DYNAMIC_DOWNCAST(CMFCRibbonSlider, m_pFrame->m_wndRibbonBar.FindByID(ID_SLIDER_RESOLUTION));
		CMFCRibbonEdit* pSpin = DYNAMIC_DOWNCAST(CMFCRibbonEdit, m_pFrame->m_wndRibbonBar.FindByID(ID_SPIN_RESOLUTION));
		CString temp_string = pSpin->GetEditText();
		int i_val = atoi(temp_string);
		pSlider->SetPos(i_val);
		m_dThumbnailResolution = (double)(i_val) / 100.0;
		WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("RESOLUTION"), (DWORD)(int)i_val);
		//Invalidate();
	}
}


void CAutoSplitPagesView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	for (int i = 0; i < m_arrImages.size(); i++)
	{
		if (m_arrImages[i].rect.left <= point.x && m_arrImages[i].rect.right >= point.x &&
			m_arrImages[i].rect.top <= point.y && m_arrImages[i].rect.bottom >= point.y)
		{
			m_arrImages[i].is_checked = !m_arrImages[i].is_checked;
			SaveCurrentCheckFile();
			Invalidate();
		}
	}
	CScrollView::OnLButtonDblClk(nFlags, point);
}

void CAutoSplitPagesView::ReadCheckInfo(CString folder)
{
	m_arrHeaderFiles.clear();
	TCHAR file_path[MAX_PATH];
	sprintf_s(file_path, "%s\\check_info", folder);
	FILE* fp;
	bool is_checked;
	fopen_s(&fp, file_path, "rt");
	if (fp)
	{
		while (fgets(file_path, MAX_PATH, fp) != NULL)
		{
			int len = strlen(file_path);
			file_path[len - 1] = '\0';
			if (file_path[len - 2] == '1')
			{
				is_checked = true;
			}
			else
			{
				is_checked = false;
			}
			file_path[len - 2] = '\0';

			m_arrHeaderFiles.push_back(make_pair(file_path, is_checked));
		}
		fclose(fp);
	}
}

bool CAutoSplitPagesView::IsFileChecked(CString& file_name)
{
	auto file_check = find_if(m_arrHeaderFiles.begin(), m_arrHeaderFiles.end(), [file_name](pair<CString, bool> curr) { return curr.first == file_name; });
	if (file_check != m_arrHeaderFiles.end())
	{
		return file_check->second;
	}
	return false;
}

void CAutoSplitPagesView::OnCheckSubfolder()
{
	m_bFindSubFolder = !m_bFindSubFolder;
	WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("INCLUDE_SUB_FOLDER"), (DWORD)m_bFindSubFolder);
}


void CAutoSplitPagesView::OnUpdateCheckSubfolder(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bFindSubFolder);
}


void CAutoSplitPagesView::OnEditCheckValue()
{
	if (m_pFrame)
	{
		CMFCRibbonEdit* pSpin = DYNAMIC_DOWNCAST(CMFCRibbonEdit, m_pFrame->m_wndRibbonBar.FindByID(ID_EDIT_CHECK_VALUE));
		CString temp_string = pSpin->GetEditText();
		m_dCheckValue = atof(temp_string);

		int save_value = (int)(m_dCheckValue*100.0);
		WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("CHECK_VALUE"), (DWORD)save_value);
	}

}

void CAutoSplitPagesView::SaveCurrentCheckFile()
{
	if (m_arrImages.size() > 0)
	{
		TCHAR file_path[MAX_PATH];
		sprintf_s(file_path, "%s\\check_info", m_strCurrentFolder);
		FILE* fp;
		fopen_s(&fp, file_path, "wt");

		if (fp)
		{
			for (int i = 0; i < m_arrImages.size(); i++)
			{
				if (m_arrImages[i].is_checked)
				{
					fprintf(fp, "%s1\n", m_arrImages[i].file_path);
				}
				else
				{
					fprintf(fp, "%s0\n", m_arrImages[i].file_path);
				}
			}
			fclose(fp);
		}
	}
}


void CAutoSplitPagesView::OnCheckUseFolername()
{
	m_bUsingFolderName_Code = true;
	WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("CODE_USE_FOLDERNAME"), (DWORD)m_bUsingFolderName_Code);
}


void CAutoSplitPagesView::OnCheckLogo()
{
	m_bUsingFolderName_Code = false;
	WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("CODE_USE_FOLDERNAME"), (DWORD)m_bUsingFolderName_Code);
}


void CAutoSplitPagesView::OnCheckCurrent()
{
	m_bApplyFolder_Code = false;
	WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("CODE_APPLY_FOLDER"), (DWORD)m_bApplyFolder_Code);
}


void CAutoSplitPagesView::OnCheckFolder()
{
	m_bApplyFolder_Code = true;
	WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("CODE_APPLY_FOLDER"), (DWORD)m_bApplyFolder_Code);
}


void CAutoSplitPagesView::OnButtonCodeColor()
{
	if (m_pFrame)
	{
		CMFCRibbonColorButton* pColor = DYNAMIC_DOWNCAST(CMFCRibbonColorButton, m_pFrame->m_wndRibbonBar.FindByID(ID_BUTTON_CODE_COLOR));
		if (pColor)
		{
			m_color1 = pColor->GetColor();
			WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("CODE_BOX_COLOR"), (DWORD)m_color1);
			m_pFrame->m_wndProperties.m_Dialog.Invalidate();
		}
	}
}


void CAutoSplitPagesView::OnUpdateCheckUseFolername(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bUsingFolderName_Code);
}


void CAutoSplitPagesView::OnUpdateCheckLogo(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(!m_bUsingFolderName_Code);
}


void CAutoSplitPagesView::OnUpdateCheckCurrent(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(!m_bApplyFolder_Code);
}


void CAutoSplitPagesView::OnUpdateCheckFolder(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bApplyFolder_Code);
}


void CAutoSplitPagesView::OnCheckMaskCurrent()
{
	m_bApplyFolder_Mask = false;
	WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("MASK_APPLY_FOLDER"), (DWORD)m_bApplyFolder_Mask);
}


void CAutoSplitPagesView::OnUpdateCheckMaskCurrent(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(!m_bApplyFolder_Mask);
}


void CAutoSplitPagesView::OnCheckMaskFolder()
{
	m_bApplyFolder_Mask = true;
	WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("MASK_APPLY_FOLDER"), (DWORD)m_bApplyFolder_Mask);
}


void CAutoSplitPagesView::OnUpdateCheckMaskFolder(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bApplyFolder_Mask);
}


void CAutoSplitPagesView::OnButtonMaskColor()
{
	if (m_pFrame)
	{
		CMFCRibbonColorButton* pColor = DYNAMIC_DOWNCAST(CMFCRibbonColorButton, m_pFrame->m_wndRibbonBar.FindByID(ID_BUTTON_MASK_COLOR));
		if (pColor)
		{
			m_color2 = pColor->GetColor();
			WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("MASK_BOX_COLOR"), (DWORD)m_color2);
			m_pFrame->m_wndProperties.m_Dialog.Invalidate();
		}
	}
}

void CAutoSplitPagesView::OnButtonProcess()
{
	OnProcessImages();
	//if (m_bApplyFolder_Code)
	//{
	//	if (m_bApplyFolder_Mask)
	//	{
	//		//	folder - folder
	//		for (int i = 0; i < m_arrImages.size(); i++)
	//		{
	//			ProcessImage(m_strCurrentFolder + "\\" + m_arrImages[i].file_path, true, true);
	//		}
	//	}
	//	else
	//	{
	//		//	folder - current
	//		for (int i = 0; i < m_arrImages.size(); i++)
	//		{
	//			if (i == m_iCurrentFileIndex)
	//			{
	//				ProcessImage(m_strCurrentFolder + "\\" + m_arrImages[i].file_path, true, true);
	//			}
	//			else
	//			{
	//				ProcessImage(m_strCurrentFolder + "\\" + m_arrImages[i].file_path, true, false);
	//			}
	//		}
	//	}
	//}
	//else
	//{
	//	if (m_bApplyFolder_Mask)
	//	{
	//		//	current - folder
	//		for (int i = 0; i < m_arrImages.size(); i++)
	//		{
	//			if (i == m_iCurrentFileIndex)
	//			{
	//				ProcessImage(m_strCurrentFolder + "\\" + m_arrImages[i].file_path, true, true);
	//			}
	//			else
	//			{
	//				ProcessImage(m_strCurrentFolder + "\\" + m_arrImages[i].file_path, false, true);
	//			}
	//		}
	//	}
	//	else
	//	{
	//		//	current - current
	//		ProcessImage(m_strCurrentFolder + "\\" + m_arrImages[m_iCurrentFileIndex].file_path, true, true);
	//	}
	//}
}

void CAutoSplitPagesView::ProcessImage(CString file_path, bool use_code, bool use_mask, int current_index)
{
	//CImage pngImage, dest_image;
	//CString file_name = file_path;
	//int index = file_path.ReverseFind('\\');
	//if (index > 0)
	//{
	//	file_name = file_name.Left(index);
	//	index = file_name.ReverseFind('\\');
	//	if (index > 0)
	//	{
	//		file_name = file_name.Right(file_name.GetLength() - index - 1);
	//	}
	//}

	//if (!FAILED(pngImage.Load(file_path)))
	//{
	//	if (m_pFrame)
	//	{
	//		int width = pngImage.GetWidth();
	//		int height = pngImage.GetHeight();
	//		CClientDC dc(this);

	//		CDC* pMDC = new CDC;
	//		pMDC->CreateCompatibleDC(&dc);
	//		CBitmap* pb = new CBitmap;
	//		pb->CreateCompatibleBitmap(&dc, width, height);
	//		CBitmap* pob = pMDC->SelectObject(pb);
	//		pMDC->SetStretchBltMode(HALFTONE);

	//		pngImage.BitBlt(pMDC->m_hDC, 0, 0, SRCCOPY);

	//		CBrush brushBlue(RGB(255, 255, 255));
	//		CBrush* pOldBrush = pMDC->SelectObject(&brushBlue);
	//		CPen penBlack;
	//		penBlack.CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
	//		CPen* pOldPen = pMDC->SelectObject(&penBlack);
	//		if (use_code)
	//		{
	//			if (m_bMultiCode)
	//			{
	//				if (m_arrRectCode.size() > current_index)
	//				{
	//					for (int j = 0; j < m_arrRectCode[current_index].size(); j++)
	//					{
	//						pMDC->Rectangle(m_arrRectCode[current_index][j].first);
	//						if (m_bUsingFolderName_Code)
	//						{
	//							CFont font;
	//							font.CreatePointFont(m_iFontSize, "Arial", pMDC);
	//							CFont* pOldFont = pMDC->SelectObject(&(font));

	//							pMDC->SetTextColor(RGB(0, 0, 0));
	//							pMDC->DrawText(file_name, m_arrRectCode[current_index][j].first, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
	//						}
	//						else
	//						{
	//							CString file_list[5] = { "logo.bmp", "logo.png", "logo.gif", "logo.jpg", "logo.jpeg" };
	//							for (int temp_i = 0; temp_i < 5; temp_i++)
	//							{
	//								CString logo_path = m_strAppPath + "\\" + file_list[temp_i];
	//								if (PathFileExists(logo_path))
	//								{
	//									CImage logo;
	//									if (!FAILED(logo.Load(logo_path)))
	//									{
	//										int l_width = pngImage.GetWidth();
	//										int l_height = pngImage.GetHeight();
	//										logo.StretchBlt(pMDC->m_hDC, m_arrRectCode[current_index][j].first, SRCCOPY);
	//										break;
	//									}
	//								}
	//							}
	//						}
	//					}
	//				}
	//			}
	//			else
	//			{
	//				if (m_arrRectCode.size() > 0)
	//				{
	//					if (m_arrRectCode[0].size() > 0)
	//					{
	//						pMDC->Rectangle(m_arrRectCode[0][0].first);
	//						if (m_bUsingFolderName_Code)
	//						{
	//							CFont font;
	//							font.CreatePointFont(m_iFontSize, "Arial", pMDC);
	//							CFont* pOldFont = pMDC->SelectObject(&(font));

	//							pMDC->SetTextColor(RGB(0, 0, 0));
	//							pMDC->DrawText(file_name, m_arrRectCode[0][0].first, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
	//						}
	//						else
	//						{
	//							CString file_list[5] = { "logo.bmp", "logo.png", "logo.gif", "logo.jpg", "logo.jpeg" };
	//							for (int temp_i = 0; temp_i < 5; temp_i++)
	//							{
	//								CString logo_path = m_strAppPath + "\\" + file_list[temp_i];
	//								if (PathFileExists(logo_path))
	//								{
	//									CImage logo;
	//									if (!FAILED(logo.Load(logo_path)))
	//									{
	//										int l_width = pngImage.GetWidth();
	//										int l_height = pngImage.GetHeight();
	//										logo.StretchBlt(pMDC->m_hDC, m_arrRectCode[0][0].first, SRCCOPY);
	//										break;
	//									}
	//								}
	//							}
	//						}
	//					}
	//				}
	//			}
	//		}

	//		if (use_mask)
	//		{
	//			if (m_bMultiMask)
	//			{
	//				if (m_arrRectMask.size() > current_index)
	//				{
	//					for (int j = 0; j < m_arrRectMask[current_index].size(); j++)
	//					{
	//						pMDC->Rectangle(m_arrRectMask[current_index][j]);
	//					}
	//				}
	//			}
	//			else
	//			{
	//				if (m_arrRectMask.size() > 0)
	//				{
	//					if (m_arrRectMask[0].size() > 0)
	//					{
	//						pMDC->Rectangle(m_arrRectMask[0][0]);
	//					}
	//				}
	//			}
	//		}

	//		pMDC->SelectObject(pob);

	//		//BITMAP bm; //Create bitmap Handle to get dimensions
	//		//pb->GetBitmap(&bm); //Load bitmap into handle

	//		dest_image.Attach((HBITMAP)(*pb));
	//		//dest_image.Create(_width, _height, pngImage.GetBPP());

	//		//pngImage.StretchBlt(dest_image.GetDC(), 0, 0, _width, _height, _left, _top, _width, _height, SRCCOPY);

	//		dest_image.Save(file_path);
	//		dest_image.ReleaseDC();

	//		string new_path(m_strTempFolder + "image");
	//		CopyFile(file_path, new_path.c_str(), FALSE);
	//		Image image;
	//		try {
	//			string file_name(new_path);
	//			image.read(file_name);
	//			image.resolutionUnits(PixelsPerInchResolution);
	//			image.density(Point(300, 300));   // could also use image.density("150x150")
	//			image.write(new_path);
	//			CopyFile(new_path.c_str(), file_path, FALSE);
	//		}
	//		catch (const Exception& e) {
	//			AfxMessageBox(e.what());
	//		}
	//	}
	//}
}

void DeleteFile2(CString strFilePath)

{
	SHFILEOPSTRUCT FileOp = { 0 };
	char szTemp[MAX_PATH];

	strcpy_s(szTemp, MAX_PATH, strFilePath);
	szTemp[strFilePath.GetLength() + 1] = NULL;

	FileOp.hwnd = NULL;
	FileOp.wFunc = FO_DELETE; // 삭제 속성 설정
	FileOp.pFrom = NULL;
	FileOp.pTo = NULL;
	// 확인메시지가 안뜨도록 설정
	FileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI;
	FileOp.fAnyOperationsAborted = false;
	FileOp.hNameMappings = NULL;
	FileOp.lpszProgressTitle = NULL;
	FileOp.pFrom = szTemp;

	SHFileOperation(&FileOp); // 삭제 작업
}

//BOOL CAutoSplitPagesView::PreTranslateMessage(MSG* pMsg)
//{
//	if (pMsg->message == WM_KEYDOWN && m_iCurrentFileIndex > -1)
//	{
//	//	int nCol = m_iCurrentFileIndex % nColCount;
//	//	int nRow = m_iCurrentFileIndex / nColCount;
//	//	int file_index = m_iCurrentFileIndex;
//	//	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
//	//	if (pFrame)
//	//	{
//	//		if (pMsg->wParam == VK_F9)
//	//		{
//	//			OnButtonProcess(); //code on F10
//	//		}
//	//		if (pMsg->wParam == VK_F5)
//	//		{
//	//			OnButtonApply(); //code on F10
//	//		}
//	//		if (pMsg->wParam == VK_LEFT)
//	//		{
//	//			file_index--;
//	//			if (file_index < 0)
//	//				file_index = nThumbnailCount - 1;
//	//			SetCurrentFile(file_index, pFrame);
//	//		}
//	//		if (pMsg->wParam == VK_RIGHT)
//	//		{
//	//			file_index++;
//	//			if (file_index >= nThumbnailCount)
//	//				file_index = 0;
//	//			SetCurrentFile(file_index, pFrame);
//	//		}
//	//		if (pMsg->wParam == VK_UP)
//	//		{
//	//			nRow--;
//	//			if (nRow < 0)
//	//			{
//	//				nRow = nRowCount - 1;
//	//			}
//	//			file_index = (nRow * nColCount) + nCol;
//	//			if (file_index >= nThumbnailCount)
//	//			{
//	//				nRow--;
//	//			}
//	//			file_index = (nRow * nColCount) + nCol;
//	//			SetCurrentFile(file_index, pFrame);
//	//		}
//	//		if (pMsg->wParam == VK_DOWN)
//	//		{
//	//			nRow++;
//	//			if (nRow >= nRowCount)
//	//			{
//	//				nRow = 0;
//	//			}
//	//			file_index = (nRow * nColCount) + nCol;
//	//			if (file_index >= nThumbnailCount)
//	//			{
//	//				nRow--;
//	//			}
//	//			file_index = (nRow * nColCount) + nCol;
//	//			SetCurrentFile(file_index, pFrame);
//	//		}
//	//		if (pMsg->wParam == VK_DELETE)
//	//		{
//	//			CString temp_string;
//	//			temp_string.Format("%s 파일을 삭제하시겠습니까?", m_arrImages[file_index].file_path);
//	//			if (AfxMessageBox(temp_string, MB_YESNO) == IDYES)
//	//			{
//	//				temp_string = m_strCurrentFolder + "\\" + m_arrImages[file_index].file_path;
//	//				DeleteFile2(temp_string);
//	//				auto itr = m_arrImages.begin() + file_index;
//	//				m_arrImages.erase(itr);
//	//				if (file_index == nThumbnailCount - 1)
//	//				{
//	//					file_index--;
//	//				}
//	//				m_iCurrentFileIndex = -2;
//	//				SetCurrentFile(file_index, pFrame);
//	//			}
//	//		}
//	//		if (pMsg->wParam == VK_F2)
//	//		{
//	//			CDlgRename pDlg(m_arrImages[file_index].file_path);
//	//			if (pDlg.DoModal() == IDOK)
//	//			{
//	//				CString new_file_name = pDlg.m_strFileName;
//	//				for (int i = 0; i < m_arrImages.size(); i++)
//	//				{
//	//					if (i != m_iCurrentFileIndex)
//	//					{
//	//						if (m_arrImages[i].file_path == new_file_name)
//	//						{
//	//							AfxMessageBox("동일한 이름의 파일이 존재합니다.");
//	//							return TRUE;
//	//						}
//	//					}
//	//				}
//
//	//				CString from_file_path = m_strCurrentFolder + "\\" + m_arrImages[file_index].file_path;
//	//				CString to_file_path = m_strCurrentFolder + "\\" + new_file_name;
//	//				rename(from_file_path, to_file_path);
//	//				m_arrImages[file_index].file_path = new_file_name;
//	//				m_iCurrentFileIndex = -2;
//	//				SetCurrentFile(file_index, pFrame);
//	//			}
//	//		}
//	//		if (pMsg->wParam == VK_F3)
//	//		{
//	//			CString file_path = m_strCurrentFolder + "\\" + m_arrImages[file_index].file_path;
//	//			string new_path(m_strTempFolder + "image");
//	//			CopyFile(file_path, new_path.c_str(), FALSE);
//
//	//			Image image;
//	//			try {
//	//				string file_name(new_path);
//	//				image.read(file_name);
//	//				image.virtualPixelMethod(Magick::TransparentVirtualPixelMethod);
//	//				image.rotate(-90.0);
//	//				image.write(new_path);
//	//				CopyFile(new_path.c_str(), file_path, FALSE);
//
//	//				m_arrImages[file_index].image = MakeThumbnailImage(this, file_path);
//	//				m_iCurrentFileIndex = -2;
//	//				SetCurrentFile(file_index, pFrame);
//	//			}
//	//			catch (const Exception& e) {
//	//				AfxMessageBox(e.what());
//	//			}
//	//		}
//	//		if (pMsg->wParam == VK_F4)
//	//		{
//	//			CString file_path = m_strCurrentFolder + "\\" + m_arrImages[file_index].file_path;
//	//			string new_path(m_strTempFolder + "image");
//	//			CopyFile(file_path, new_path.c_str(), FALSE);
//
//	//			Image image;
//	//			try {
//	//				string file_name(new_path);
//	//				image.read(file_name);
//	//				image.virtualPixelMethod(Magick::TransparentVirtualPixelMethod);
//	//				image.rotate(90.0);
//	//				image.write(new_path);
//	//				CopyFile(new_path.c_str(), file_path, FALSE);
//
//	//				m_arrImages[file_index].image = MakeThumbnailImage(this, file_path);
//	//				m_iCurrentFileIndex = -2;
//	//				SetCurrentFile(file_index, pFrame);
//	//			}
//	//			catch (const Exception& e) {
//	//				AfxMessageBox(e.what());
//	//			}
//	//		}
//	//		if (pMsg->wParam == VK_F6)
//	//		{
//	//			if (m_bMultiCode)
//	//			{
//	//				if (m_iCurrentFileIndex > -1)
//	//				{
//	//					if (m_iCurrentFileIndex < m_arrRectCode.size())
//	//					{
//	//						if (m_arrRectCode[m_iCurrentFileIndex].size() > 0)
//	//						{
//	//							m_arrRectCode[m_iCurrentFileIndex].erase(m_arrRectCode[m_iCurrentFileIndex].begin() + m_arrRectCode[m_iCurrentFileIndex].size() - 1);
//	//							m_pFrame->m_wndProperties.m_Dialog.Invalidate();
//	//						}
//	//					}
//	//				}
//	//			}
//	//		}
//	//		if (pMsg->wParam == VK_F7)
//	//		{
//	//			if (m_bMultiMask)
//	//			{
//	//				if (m_iCurrentFileIndex > -1)
//	//				{
//	//					if (m_iCurrentFileIndex < m_arrRectMask.size())
//	//					{
//	//						if (m_arrRectMask[m_iCurrentFileIndex].size() > 0)
//	//						{
//	//							m_arrRectMask[m_iCurrentFileIndex].erase(m_arrRectMask[m_iCurrentFileIndex].begin() + m_arrRectMask[m_iCurrentFileIndex].size() - 1);
//	//							m_pFrame->m_wndProperties.m_Dialog.Invalidate();
//	//						}
//	//					}
//	//				}
//	//			}
//	//		}
//	//	}
//	//	return TRUE;
//	}
//	return CScrollView::PreTranslateMessage(pMsg);
//}


void CAutoSplitPagesView::OnComboFont()
{
	if (m_pFrame)
	{
		//CMFCRibbonFontComboBox* pFont = DYNAMIC_DOWNCAST(CMFCRibbonFontComboBox, m_pFrame->m_wndRibbonBar.FindByID(ID_COMBO_FONT));
		//if (pFont)
		//{
		//	pFont->getfon
		//	m_color1 = pColor->GetColor();
		//	WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("CODE_BOX_COLOR"), (DWORD)m_color1);
		//	m_pFrame->m_wndProperties.m_Dialog.Invalidate();
		//}
	}
}


void CAutoSplitPagesView::OnEditFontSize()
{
	if (m_pFrame)
	{
		CMFCRibbonEdit* pSpin = DYNAMIC_DOWNCAST(CMFCRibbonEdit, m_pFrame->m_wndRibbonBar.FindByID(ID_EDIT_FONT_SIZE));
		CString temp_string = pSpin->GetEditText();
		m_iFontSize = atoi(temp_string);
		WriteDWORDValueInRegistry(HKEY_CURRENT_USER, (LPCTSTR)m_strKey, _T("CODE_FONT_SIZE"), (DWORD)(int)m_iFontSize);
	}
}

void CAutoSplitPagesView::OnApplyImages()
{
	// create semaphores the first time only
	if (!imageFillerSemaphore)
	{
		imageFillerSemaphore = CreateSemaphore(NULL, 0, 1, NULL);
		imageFillerCR = CreateSemaphore(NULL, 1, 1, NULL);
		imageFillerWait = CreateSemaphore(NULL, 1, 1, NULL);
	}

	// critical region starts here
	WaitForSingleObject(imageFillerCR, 0);

	// create thread parameters
	threadParamImage* pParam = new threadParamImage;
	pParam->folder = m_strCurrentFolder;
	pParam->pView = this;
	pParam->pFrame = m_pFrame;
	pParam->use_code = m_bApplyFolder_Code;
	pParam->use_mask = m_bApplyFolder_Mask;

	// and the thread
	DWORD dummy;
	imageFiller = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ImageApplyThread,
		pParam, 0, &dummy);
}

void CAutoSplitPagesView::OnProcessImages()
{
	if (m_pFrame)
	{
		m_iPrevFileIndex = m_iCurrentFileIndex;
		m_bStopChecking = false;
		OnProcessImages(m_strCurrentFolder);

		FillInImages(m_strCurrentFolder);
	}
	AfxMessageBox("Process Completed");
}

void CAutoSplitPagesView::OnProcessImages(CString folder)
{
	if (m_pFrame)
	{
		WIN32_FIND_DATA fd;
		HANDLE find;
		BOOL ok = TRUE;
		fd.dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_COMPRESSED |
			FILE_ATTRIBUTE_READONLY;
		find = FindFirstFile(folder + "\\*.*", &fd);

		// return if operation failed
		if (find == INVALID_HANDLE_VALUE)
		{
			m_pFrame->m_wndOutput.AddString(folder + " is not valid");

			return;
		}

		m_pFrame->m_wndOutput.AddString("Processing folder : " + folder);
		CString temp_string;
		//for (match_method = 0; match_method <= 5; match_method++)
		//{
		//	temp_string.Format("match_method : %d\n", match_method);
		//	m_pFrame->m_wndOutput.AddString(temp_string);
		time_t start_time;
		time(&start_time);

		TCHAR file_path[MAX_PATH];
		vector< CString > sub_folders;
		int file_count = 0;
		vector< string > file_names;
		// start adding items to the list control
		do
		{
			if (m_bStopChecking) break;
			ok = FindNextFile(find, &fd);
			if (fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			{
				if (fd.cFileName[0] != '.')
					sub_folders.push_back(fd.cFileName);
				continue;
			}

			if (ok)
			{
				file_names.push_back(fd.cFileName);
			}
		} while (find&&ok);

		// done adding items
		FindClose(find);
		sort(file_names.begin(), file_names.end());
		CString check_file_path;
		for (int i = 0; i < file_names.size(); i++)
		{
			sprintf_s(file_path, "%s\\%s", folder, file_names[i].c_str());

			if (m_bApplyFolder_Code)
			{
				if (m_bApplyFolder_Mask)
				{
					//	folder - folder
					ProcessImage(file_path, true, true, i);
				}
				else
				{
					//	folder - current
						if (i == m_iCurrentFileIndex)
						{
							ProcessImage(file_path, true, true, i);
						}
						else
						{
							ProcessImage(file_path, true, false, i);
						}
				}
			}
			else
			{
				if (m_bApplyFolder_Mask)
				{
					//	current - folder
						if (i == m_iCurrentFileIndex)
						{
							ProcessImage(file_path, true, true, i);
						}
						else
						{
							ProcessImage(file_path, false, true, i);
						}
				}
				else
				{
					//	current - current
					if (i == m_iCurrentFileIndex)
					{
						ProcessImage(file_path, true, true, i);
					}
				}
			}

			file_count++;
		}

		time_t end_time;
		time(&end_time);
		double spend_time = difftime(end_time, start_time);
		//TRACE("spend_time : %lf\n\n", spend_time);
		temp_string.Format("spend_time : %lf", spend_time);
		m_pFrame->m_wndOutput.AddString(temp_string);

		if (m_bFindSubFolder)
		{
			for (int i = 0; i < sub_folders.size() && m_bStopChecking == false; i++)
			{
				OnProcessImages(folder + "\\" + sub_folders[i]);
			}
		}
	}
}


void CAutoSplitPagesView::OnButtonApply()
{
	OnApplyImages();
	m_bViewThumbnailView = true;
}


void CAutoSplitPagesView::OnCheckApply()
{
	m_bViewThumbnailView = !m_bViewThumbnailView;
	Invalidate();
}


void CAutoSplitPagesView::OnUpdateCheckApply(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bViewThumbnailView);
}


void CAutoSplitPagesView::OnFileView()
{
	m_pFrame->m_wndFileView.ShowPane(!(m_pFrame->m_wndFileView.IsVisible()), FALSE, FALSE);
}


void CAutoSplitPagesView::OnUpdateFileView(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_pFrame->m_wndFileView.IsVisible());
}

void CAutoSplitPagesView::OnResult()
{
	m_pFrame->m_wndOutput.ShowPane(!(m_pFrame->m_wndOutput.IsVisible()), FALSE, FALSE);
}


void CAutoSplitPagesView::OnUpdateResult(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_pFrame->m_wndOutput.IsVisible());
}


void CAutoSplitPagesView::OnStandard()
{
	m_pFrame->m_wndProperties.ShowPane(!(m_pFrame->m_wndProperties.IsVisible()), FALSE, FALSE);
}


void CAutoSplitPagesView::OnUpdateStandard(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_pFrame->m_wndProperties.IsVisible());
}


void CAutoSplitPagesView::OnImage1()
{
	m_pFrame->m_wndTemplate1.ShowPane(!(m_pFrame->m_wndTemplate1.IsVisible()), FALSE, FALSE);
}


void CAutoSplitPagesView::OnUpdateImage1(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_pFrame->m_wndTemplate1.IsVisible());
}


void CAutoSplitPagesView::OnImage2()
{
	m_pFrame->m_wndTable.ShowPane(!(m_pFrame->m_wndTable.IsVisible()), FALSE, FALSE);
}


void CAutoSplitPagesView::OnUpdateImage2(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_pFrame->m_wndTable.IsVisible());
}


void CAutoSplitPagesView::OnCheckMultiCode()
{
	m_bMultiCode = !m_bMultiCode;
	m_pFrame->m_wndProperties.m_Dialog.Invalidate();
	Invalidate();
}


void CAutoSplitPagesView::OnUpdateCheckMultiCode(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bMultiCode);
}


void CAutoSplitPagesView::OnCheckMultiMask()
{
	m_bMultiMask = !m_bMultiMask;
	m_pFrame->m_wndProperties.m_Dialog.Invalidate();
	Invalidate();
}


void CAutoSplitPagesView::OnUpdateCheckMultiMask(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bMultiMask);
}

void CAutoSplitPagesView::DeleteCodeRect(CPoint point)
{
	if (m_iCurrentFileIndex > -1)
	{
		if (m_bMultiCode)
		{
			if (m_iCurrentFileIndex >= m_arrRectCode.size())
			{
				return;
			}

			for (int i = 0; i < m_arrRectCode[m_iCurrentFileIndex].size(); i++)
			{
				CRect rect = m_pFrame->m_wndProperties.m_Dialog.GetWindowRectFromImage(m_arrRectCode[m_iCurrentFileIndex][i].first);
				if (rect.left <= point.x && rect.right >= point.x && rect.top <= point.y && rect.bottom >= point.y)
				{
					m_mapCodeImageFile.erase(m_arrRectCode[m_iCurrentFileIndex][i].second);
					m_arrRectCode[m_iCurrentFileIndex].erase(m_arrRectCode[m_iCurrentFileIndex].begin() + i);
					m_pFrame->m_wndProperties.m_Dialog.UpdateWindow();

					if (m_iCurrentFileIndex == 0)
					{
						int page_count = m_arrRectCode.size();
						for (int j = 1; j < page_count; j++)
						{
							m_arrRectCode[j].erase(m_arrRectCode[j].begin() + i);
						}
					}

					return;
				}
			}
		}
		else if(m_arrRectCode.size() > 0)
		{
			for (int i = 0; i < m_arrRectCode[0].size(); i++)
			{
				CRect rect = m_pFrame->m_wndProperties.m_Dialog.GetWindowRectFromImage(m_arrRectCode[0][i].first);
				if (rect.left <= point.x && rect.right >= point.x && rect.top <= point.y && rect.bottom >= point.y)
				{
					m_mapCodeImageFile.erase(m_arrRectCode[0][i].second);
					m_arrRectCode[0].erase(m_arrRectCode[0].begin() + i);
					m_pFrame->m_wndProperties.m_Dialog.UpdateWindow();
					return;
				}
			}
		}
	}
}

void CAutoSplitPagesView::SavePageCodeImage(int page_index)
{
	if (page_index < 0 || page_index >= m_arrImages.size())
		return;

	if (page_index < 0 || page_index >= m_arrRectCode.size())
		return;

	CString page_path = m_strCurrentFolder + "\\" + m_arrImages[page_index].file_path;

	long _width = 0, _height = 0;
	for (int rect_index = 0; rect_index < m_arrRectCode[page_index].size(); rect_index++)
	{
		if (_width < m_arrRectCode[page_index][rect_index].first.Width())
		{
			_width = m_arrRectCode[page_index][rect_index].first.Width();
		}

		_height += m_arrRectCode[page_index][rect_index].first.Height();
		_height += 30;
	}

	CImage dest_image;
	CClientDC dc(this);
	CDC* pMDC = new CDC;
	pMDC->CreateCompatibleDC(&dc);
	CBitmap* pb = new CBitmap;
	pb->CreateCompatibleBitmap(&dc, _width, _height);
	CBitmap* pob = pMDC->SelectObject(pb);
	pMDC->SetStretchBltMode(HALFTONE);
	pMDC->FillSolidRect(0, 0, _width, _height, RGB(255, 255, 255));
	long current_y = 0;
	CString no_string;
	CString page_string;
	for (int rect_index = 0; rect_index < m_arrRectCode[page_index].size(); rect_index++)
	{
		no_string.Format(" << %d>> ", rect_index + 1);
		pMDC->SetTextColor(RGB(255, 0, 0));
		pMDC->DrawText(no_string, CRect(0, current_y, _width, current_y + 30), DT_LEFT | DT_TOP);
		current_y += 30;

		CImage pngImage;
		long current_width = m_arrRectCode[page_index][rect_index].first.Width();
		long current_height = m_arrRectCode[page_index][rect_index].first.Height();
		if (!FAILED(pngImage.Load(m_strTempPath + m_arrRectCode[page_index][rect_index].second)))
		{
			pngImage.StretchBlt(pMDC->m_hDC, 0, current_y, current_width, current_height, 0, 0, current_width, current_height, SRCCOPY);
		}
		current_y += current_height;
	}

	pMDC->SelectObject(pob);
	dest_image.Attach((HBITMAP)(*pb));
	page_string.Format("page_%d.png", page_index);
	dest_image.Save(m_strTempPath + page_string);
}

void CAutoSplitPagesView::UpdatePageImages(int page_index)
{
	if (page_index < 0 || page_index >= m_arrImages.size())
		return;

	if (page_index < 0 || page_index >= m_arrRectCode.size())
		return;

	CString page_path = m_strCurrentFolder + "\\" + m_arrImages[page_index].file_path;

	for (int rect_index = 0; rect_index < m_arrRectCode[page_index].size(); rect_index++)
	{
		UpdateRectImage(m_arrRectCode[page_index][rect_index].first, page_path, m_arrRectCode[page_index][rect_index].second);
	}
}

bool CAutoSplitPagesView::UpdateRectImage(CRect rect, CString page_path, CString file_name)
{
	long _left, _top, _width, _height;
	_left = rect.left;
	_top = rect.top;
	_width = rect.Width();
	_height = rect.Height();

	CImage pngImage, dest_image;
	if (!FAILED(pngImage.Load(page_path)))
	{
		CClientDC dc(this);

		CDC* pMDC = new CDC;
		pMDC->CreateCompatibleDC(&dc);
		CBitmap* pb = new CBitmap;
		pb->CreateCompatibleBitmap(&dc, _width, _height);
		CBitmap* pob = pMDC->SelectObject(pb);
		pMDC->SetStretchBltMode(HALFTONE);

		pngImage.StretchBlt(pMDC->m_hDC, 0, 0, _width, _height, _left, _top, _width, _height, SRCCOPY);
		pMDC->SelectObject(pob);

		dest_image.Attach((HBITMAP)(*pb));

		dest_image.Save(m_strTempPath + file_name);

		return true;
	}
	return false;
}

bool CAutoSplitPagesView::UpdateRectImage(int page_index, int rect_index, CString file_name)
{
	if (page_index < 0 || page_index >= m_arrImages.size())
		return false;

	if (page_index < 0 || page_index >= m_arrRectCode.size())
		return false;

	if (rect_index < 0 || rect_index >= m_arrRectCode[page_index].size())
		return false;

	CString page_path = m_strCurrentFolder + "\\" + m_arrImages[page_index].file_path;

	return UpdateRectImage(m_arrRectCode[page_index][rect_index].first, page_path, file_name);
}

void CAutoSplitPagesView::InitGrid()
{
	int page_count = m_arrImages.size();
	int row_count = 0;

	for (int i = 0; i < page_count; i++)
	{
		//if (row_count < (m_arrOCRText[i].size() + m_arrRectCode[i].size()))
		//	row_count = (m_arrOCRText[i].size() + m_arrRectCode[i].size());
		if (row_count < (m_arrOCRPrevCount[i] + m_arrRectCode[i].size()))
			row_count = (m_arrOCRPrevCount[i] + m_arrRectCode[i].size());
	}

	m_pFrame->m_wndTable.m_Dialog.m_Grid.SetNumberCols(row_count + 1);
	m_pFrame->m_wndTable.m_Dialog.m_Grid.QuickSetText(0, -1, "선택");
	for (int i = 1; i < row_count + 1; i++)
	{
		CString temp_string;
		temp_string.Format("[%d]", i);
		m_pFrame->m_wndTable.m_Dialog.m_Grid.QuickSetText(i, -1, temp_string);
	}
	for (int i = 0; i < page_count; i++)
	{
		m_pFrame->m_wndTable.m_Dialog.m_Grid.QuickSetCellType(0, i, UGCT_CHECKBOX);

		for (int j = 0; j < m_arrOCRText[i].size(); j++)
		{
			m_pFrame->m_wndTable.m_Dialog.m_Grid.QuickSetText(j + 1, i, m_arrOCRText[i][j]);
			m_pFrame->m_wndTable.m_Dialog.m_Grid.QuickSetBackColor(j + 1, i, RGB(255, 255, 200));
		}
		for (int j = m_arrOCRText[i].size(); j < row_count; j++)
		{
			m_pFrame->m_wndTable.m_Dialog.m_Grid.QuickSetBackColor(j + 1, i, RGB(255, 255, 255));
		}
	}
	m_pFrame->m_wndTable.m_Dialog.m_Grid.BestFit(-1, row_count, 0, UG_BESTFIT_TOPHEADINGS | UG_BESTFIT_AVERAGE);
}

void CAutoSplitPagesView::SetCodeRect(CRect& rect, CString file_name)
{
	//	선택 상자 저장되는 영역
	if (m_iCurrentFileIndex > -1)
	{
		if (m_bMultiCode)
		{
			int page_count = m_arrImages.size();
			m_arrRectCode.resize(page_count);
			//if (m_iCurrentFileIndex >= m_arrRectCode.size())
			//{
			//	m_arrRectCode.resize(m_iCurrentFileIndex + 1);
			//}
			switch (m_iSelPageType)
			{
			case 0:	// all
			{
				for (int i = 0; i < page_count; i++)
				{
					m_arrRectCode[i].push_back(make_pair(rect, file_name));
				}
			}
			break;
			case 1: //	current
			{
				m_arrRectCode[m_iCurrentFileIndex].push_back(make_pair(rect, file_name));
			}
			break;
			case 2: //	sel
			{
				for (int i = 0; i < page_count; i++)
				{
					CString sel_str = m_pFrame->m_wndTable.m_Dialog.m_Grid.QuickGetText(0, i);
					if (sel_str == "1")
					{
						m_arrRectCode[i].push_back(make_pair(rect, file_name));
					}
				}
			}
			break;
			}
			m_mapCodeImageFile[file_name] = 1;

			InitGrid();
		}
		else
		{
			m_arrRectCode.resize(1);
			m_arrRectCode[0].clear();
			m_arrRectCode[0].push_back(make_pair(rect, file_name));
		}
	}
}

void CAutoSplitPagesView::SetMaskRect(CRect& rect)
{
	if (m_iCurrentFileIndex > -1)
	{
		if (m_bMultiMask)
		{
			if (m_iCurrentFileIndex >= m_arrRectMask.size())
			{
				m_arrRectMask.resize(m_iCurrentFileIndex + 1);
			}

			m_arrRectMask[m_iCurrentFileIndex].push_back(rect);
		}
		else
		{
			m_arrRectMask.resize(1);
			m_arrRectMask[0].clear();
			m_arrRectMask[0].push_back(rect);
		}
	}
}

void CAutoSplitPagesView::OnCheckResetFolder()
{
	m_bAutoResetMulti = !m_bAutoResetMulti;
	//m_pFrame->m_wndProperties.m_Dialog.Invalidate();
	//Invalidate();
}


void CAutoSplitPagesView::OnUpdateCheckResetFolder(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bAutoResetMulti);
}

void CAutoSplitPagesView::OnButtonOCR_Select_all()
{
	BeginProgress();
	m_strProgressStatus = "Extract Selection Area Text";
	UpdateProgress(0, m_arrImages.size());

	CString page_string;
	CString page_txt_string;
	for (int index = 0; index < m_arrImages.size() && index < m_arrRectCode.size(); index++)
	{
		UpdatePageImages(index);
		SavePageCodeImage(index);

		ExtractSelectOCR(index);
		page_string.Format("[ %d / %d ]Extract Text from : %s", index + 1, m_arrImages.size(), m_arrImages[m_iCurrentFileIndex].file_path);
		m_pFrame->m_wndOutput.AddString(page_string);
		UpdateProgress(index + 1, m_arrImages.size());
	}
	EndProgress();
	UpdatePageImages(m_iCurrentFileIndex);
	m_pFrame->m_wndTable.m_Dialog.m_Grid.RedrawAll();
}

void CAutoSplitPagesView::ExtractSelectOCR(int index)
{
	CString page_string;
	CString page_txt_string;
	page_string.Format("page_%d.png", index);
	page_txt_string.Format("output_%d.txt", index);

	if ((!PathFileExists(page_txt_string)) || (!m_bPreserveExsiting))
	{
		GetOCRText(m_strTempPath + page_string, page_txt_string, 0);
	}

	if (PathFileExists(page_txt_string))
	{
		CString no_string;
		CStdioFile txt_file;
		CString str;
		vector< CString > all_str;
		if (txt_file.Open(page_txt_string, CFile::modeRead | CFile::typeText))
		{
			while (txt_file.ReadString(str))
			{
				if (str != "");
				all_str.push_back(str);
			}

			txt_file.Close();
		}

		int count = m_arrRectCode[index].size();
		vector< pair<int, int> > all_index;

		for (int i = 0; i < count; i++)
		{
			CString no_string;
			no_string.Format("%d>>", i + 1);
			for (int j = 0; j < all_str.size(); j++)
			{
				if (all_str[j].Find(no_string) > -1)
				{
					all_index.push_back(make_pair(i, j));
				}
			}
		}

		m_arrOCRText[index].resize(m_arrOCRPrevCount[index] + count);

		for (int i = 0; i < all_index.size(); i++)
		{
			no_string = "";
			if (i == all_index.size() - 1)
			{
				for (int j = all_index[i].second + 1; j < all_str.size(); j++)
				{
					no_string += all_str[j];
					no_string + " ";
				}
			}
			else
			{
				for (int j = all_index[i].second + 1; j < all_index[i + 1].second; j++)
				{
					no_string += all_str[j];
					no_string + " ";
				}
			}

			int col_index = m_arrOCRPrevCount[index] + all_index[i].first;
			m_arrOCRText[index][col_index] = no_string;
			//m_pFrame->m_wndTable.m_Dialog.m_Grid.QuickSetText(all_index[i].first + 1, index, no_string);
		}
		InitGrid();
		//m_pFrame->m_wndTable.m_Dialog.m_Grid.RedrawAll();
	}
}

void CAutoSplitPagesView::OnButtonOCR_Select_page()
{
	SavePageCodeImage(m_iCurrentFileIndex);
	ExtractSelectOCR(m_iCurrentFileIndex);
	m_pFrame->m_wndOutput.AddString("Extract Text from : " + m_arrImages[m_iCurrentFileIndex].file_path);
}

CString CAutoSplitPagesView::ExtractFullText(int index)
{
	CString origin_file_path = m_strCurrentFolder + "\\" + m_arrImages[index].file_path;
	return ExtractFullText(origin_file_path, index);
}

CString CAutoSplitPagesView::ExtractFullText(CString origin_file_path, int rect_info_index)
{
	CString output_file_path;
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	_splitpath_s(origin_file_path, drive, sizeof drive, dir, sizeof dir, fname, sizeof fname, ext, sizeof ext);
	output_file_path.Format("%s%s%s.txt", drive, dir, fname);

	if (!PathFileExists(output_file_path) || (!m_bPreserveExsiting))
	{
		if (m_iCurrentPageNum > 1)
		{
			CImage origin_image;
			if (!FAILED(origin_image.Load(origin_file_path)))
			{
				float origin_width = origin_image.GetWidth();
				float origin_height = origin_image.GetHeight();
				vector< pair<float, float> > page_widths;
				float prev_width = 0;
				float current_width = 0;
				float _width = 0;
				float _height = 0;
				float temp_width = 0;
				for (int rect_index = 0; rect_index < m_arrPageWidth[rect_info_index].size() - 1; rect_index++)
				{
					current_width = m_arrPageWidth[rect_info_index][rect_index] * origin_width;
					temp_width = current_width - prev_width;
					page_widths.push_back(make_pair(prev_width, temp_width));

					if (temp_width > _width)
						_width = temp_width;
					_height += origin_height;

					prev_width = current_width;
				}
				current_width = origin_width;
				temp_width = current_width - prev_width;
				page_widths.push_back(make_pair(prev_width, temp_width));

				if (temp_width > _width)
					_width = temp_width;
				_height += origin_height;

				CImage dest_image;
				CClientDC dc(this);
				CDC* pMDC = new CDC;
				pMDC->CreateCompatibleDC(&dc);
				CBitmap* pb = new CBitmap;
				pb->CreateCompatibleBitmap(&dc, (long)_width, (long)_height);
				CBitmap* pob = pMDC->SelectObject(pb);
				pMDC->SetStretchBltMode(HALFTONE);
				long current_y = 0;
				CString no_string;
				CString page_string;
				for (int rect_index = 0; rect_index < page_widths.size(); rect_index++)
				{
					origin_image.StretchBlt(pMDC->m_hDC, 0, current_y, page_widths[rect_index].second, origin_height, page_widths[rect_index].first, 0, page_widths[rect_index].second, origin_height, SRCCOPY);
					current_y += origin_height;
				}

				pMDC->SelectObject(pob);
				dest_image.Attach((HBITMAP)(*pb));
				page_string.Format("page_%s.png", fname);
				dest_image.Save(m_strTempPath + page_string);

				origin_file_path = m_strTempPath + page_string;
			}
		}

		GetOCRText(origin_file_path, output_file_path, 1);
	}
	return output_file_path;
}

void CAutoSplitPagesView::OnButtonOCR_Page_all()
{
	BeginProgress();
	m_strProgressStatus = "Extract Full Text";
	m_iTotalFileCount = 0;
	m_iCurrentFileCount = m_arrImages.size();
	UpdateProgress(m_iTotalFileCount, m_iCurrentFileCount);

	CString output_string;
	for (int index = 0; index < m_arrImages.size(); index++)
	{
		ExtractFullText(index);
		output_string.Format("[ %d / %d ] Extract Full Text from : %s", index + 1, m_arrImages.size(), m_arrImages[index].file_path);
		m_pFrame->m_wndOutput.AddString(output_string);
		m_iTotalFileCount++;
		UpdateProgress(m_iTotalFileCount, m_iCurrentFileCount);

	}
	EndProgress();
}

void CAutoSplitPagesView::OnButtonOCR_Page_page()
{
	CString output_file_path;
	output_file_path = ExtractFullText(m_iCurrentFileIndex);
	m_pFrame->m_wndOutput.AddString("Extract Full Text from : " + m_arrImages[m_iCurrentFileIndex].file_path);
	if (PathFileExists(output_file_path))
	{
		ShellExecute(NULL, "open", output_file_path, NULL, NULL, SW_SHOW);
	}
}

void SetCellValue(xlnt::worksheet& ws, int column, int row, CString value)
{
	if (value != "")
	{
		char strUtf8[512] = { 0, };

		lpctstr_to_utf8(value, strUtf8);
		ws.cell(column, row).value(strUtf8);
	}
}

void CAutoSplitPagesView::OnButtonSaveExcel()
{
	CFileDialog dlg(FALSE, _T("xlsx"), m_strCurrentFolder, 6UL, _T("Excel(*.xlsx)|*.xlsx|All Files(*.*)|*.*||"));
	dlg.m_ofn.lpstrInitialDir = m_strCurrentFolder;
	if (dlg.DoModal() == IDOK)
	{
		int row_count = m_pFrame->m_wndTable.m_Dialog.m_Grid.GetNumberRows();
		int col_count = m_pFrame->m_wndTable.m_Dialog.m_Grid.GetNumberCols();

		if (row_count > 0)
		{
			xlnt::workbook wb;
			xlnt::worksheet ws = wb.active_sheet();

			xlnt::border::border_property bp;
			bp.color(xlnt::rgb_color(0, 0, 0));
			bp.style(xlnt::border_style::thin);
			xlnt::border border;
			border.side(xlnt::border_side::bottom, bp)
				.side(xlnt::border_side::top, bp)
				.side(xlnt::border_side::start, bp)
				.side(xlnt::border_side::end, bp);

			xlnt::alignment alignment;
			alignment.horizontal(xlnt::horizontal_alignment::center);

			xlnt::font font;
			font.color(xlnt::rgb_color(255, 255, 255));

			for (int j = -1; j < col_count; j++)
			{
				SetCellValue(ws, j + 2, 1, m_pFrame->m_wndTable.m_Dialog.m_Grid.QuickGetText(j, -1));
				for (int i = 0; i < row_count; i++)
				{
					SetCellValue(ws, j + 2, i + 2, m_pFrame->m_wndTable.m_Dialog.m_Grid.QuickGetText(j, i));
				}
			}
			char    UTF[512] = { 0, };
			lpctstr_to_utf8(dlg.GetPathName(), UTF);
			wb.save(std::string(UTF));
		}

		CString temp_string;
		temp_string.Format(_T("[%s] 파일이 생성되었습니다."), dlg.GetPathName());
		AfxMessageBox(temp_string);
	}
}

void CAutoSplitPagesView::OnCellChange(int oldcol, int newcol, long oldrow, long newrow)
{
	if(oldrow != newrow)
		SetCurrentFile(newrow, m_pFrame);
}

void CAutoSplitPagesView::InitPageNum()
{
	m_arrPageWidth.resize(m_arrImages.size());
	for (int i = 0; i<m_arrImages.size(); i++)
	{
		m_arrPageWidth[i].resize(m_iCurrentPageNum);
		float width = 1.0f / (float)m_iCurrentPageNum;
		for (int j = 0; j < m_iCurrentPageNum; j++)
		{
			m_arrPageWidth[i][j] = (width * float(j+1));
		}
	}
}

void CAutoSplitPagesView::OnSpinPageNum()
{
	CMFCRibbonEdit* pSpin = DYNAMIC_DOWNCAST(CMFCRibbonEdit, m_pFrame->m_wndRibbonBar.FindByID(ID_SPIN_PAGE_NUM));
	if (pSpin)
	{
		int page_num = atoi(pSpin->GetEditText());
		if (page_num != m_iCurrentPageNum)
		{
			m_iCurrentPageNum = page_num;
			InitPageNum();
			m_pFrame->m_wndProperties.m_Dialog.Invalidate();
		}
	}
}


void CAutoSplitPagesView::OnButtonSelClear()
{
	int page_count = m_arrImages.size();
	m_arrRectCode.resize(page_count);

	for (int i = 0; i < page_count; i++)
	{
		m_arrOCRPrevCount[i] = m_arrOCRText[i].size();
		m_arrRectCode[i].clear();
	}

	InitGrid();
	m_pFrame->m_wndProperties.m_Dialog.Invalidate();
}

void CAutoSplitPagesView::BeginProgress()
{
	m_DlgProgress->ShowWindow(SW_SHOW);
	time(&start_time);
	prev_update_time = start_time;
}

void CAutoSplitPagesView::EndProgress()
{
	m_DlgProgress->ShowWindow(SW_HIDE);
}

void CAutoSplitPagesView::UpdateProgress(int current, int total)
{
	CString temp_string;
	int tm_hour;
	int tm_min;
	int tm_sec;
	time(&end_time);
	double d_diff = difftime(end_time, prev_update_time);
	if (d_diff > 0.5)
	{
		prev_update_time = end_time;
		d_diff = difftime(end_time, start_time);
		tm_hour = d_diff / (60 * 60);
		d_diff = d_diff - (tm_hour * 60 * 60);

		tm_min = d_diff / 60;
		d_diff = d_diff - (tm_min * 60);

		tm_sec = d_diff;
		float percent = 0;
		if (total > 0)
			percent = (float)current * 100.0f / (float)total;

		temp_string.Format(_T("%s : %.2lf%% [%d : %d : %d]"), m_strProgressStatus, percent, tm_hour, tm_min, tm_sec);
		m_DlgProgress->m_stStatus.SetWindowText(temp_string);
		m_DlgProgress->m_progressBar.SetRange(0, total);
		m_DlgProgress->m_progressBar.SetPos(current);
		m_DlgProgress->RedrawWindow();

		MSG msg;
		while (::PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}
}

void CAutoSplitPagesView::OnButtonOcrPageTextSub()
{
	BeginProgress();
	m_strProgressStatus = "Extract Full Text";
	m_iTotalFileCount = 0;
	m_iCurrentFileCount = m_arrImages.size();
	UpdateProgress(m_iTotalFileCount, m_iCurrentFileCount);

	CString output_string;
	for (int index = 0; index < m_arrImages.size(); index++)
	{
		ExtractFullText(index);
		output_string.Format("[ %d / %d ] Extract Full Text from : %s", index + 1, m_arrImages.size(), m_arrImages[index].file_path);
		m_pFrame->m_wndOutput.AddString(output_string);
		m_iTotalFileCount++;
		UpdateProgress(m_iTotalFileCount, m_iCurrentFileCount);
	}

	ExtractFullTextFolder(m_strCurrentFolder, true);
	EndProgress();
}

void CAutoSplitPagesView::ExtractFullTextFolder(CString folder, bool skip_files)
{
	if (m_pFrame)
	{
		WIN32_FIND_DATA fd;
		HANDLE find;
		BOOL ok = TRUE;
		fd.dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_COMPRESSED |
			FILE_ATTRIBUTE_READONLY;
		find = FindFirstFile(folder + "\\*.*", &fd);

		// return if operation failed
		if (find == INVALID_HANDLE_VALUE)
		{
			m_pFrame->m_wndOutput.AddString(folder + " is not valid");

			return;
		}

		m_pFrame->m_wndOutput.AddString("Processing folder : " + folder);
		CString temp_string;
		//for (match_method = 0; match_method <= 5; match_method++)
		//{
		//	temp_string.Format("match_method : %d\n", match_method);
		//	m_pFrame->m_wndOutput.AddString(temp_string);
		time_t start_time;
		time(&start_time);

		TCHAR file_path[MAX_PATH];
		vector< CString > sub_folders;
		map< CString, int > sub_folders_check;
		vector< string > file_names;
		// start adding items to the list control
		do
		{
			if (m_bStopChecking) break;
			ok = FindNextFile(find, &fd);
			if (fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			{
				if (fd.cFileName[0] != '.')
				{
					if (sub_folders_check.find(fd.cFileName) == sub_folders_check.end())
					{
						sub_folders.push_back(fd.cFileName);
						sub_folders_check[fd.cFileName] = sub_folders.size();
					}
				}
				continue;
			}

			if (ok)
			{
				CImage pngImage;
				sprintf_s(file_path, "%s\\%s", folder, fd.cFileName);
				if (!FAILED(pngImage.Load(file_path)))
				{
					file_names.push_back(fd.cFileName);
				}
			}
		} while (find&&ok);

		// done adding items
		FindClose(find);
		if (skip_files == false)
		{
			m_iCurrentFileCount += file_names.size();
			CString output_string;
			sort(file_names.begin(), file_names.end());
			CString check_file_path;
			for (int i = 0; i < file_names.size(); i++)
			{
				sprintf_s(file_path, "%s\\%s", folder, file_names[i].c_str());

				ExtractFullText(file_path);
				m_iTotalFileCount++;
				output_string.Format("[ %d / %d ] Extract Full Text from : %s", m_iTotalFileCount, m_iCurrentFileCount, file_names[i].c_str());
				m_pFrame->m_wndOutput.AddString(output_string);
				UpdateProgress(m_iTotalFileCount, m_iCurrentFileCount);
			}
		}

		time_t end_time;
		time(&end_time);
		double spend_time = difftime(end_time, start_time);
		//TRACE("spend_time : %lf\n\n", spend_time);
		temp_string.Format("spend_time : %lf", spend_time);
		m_pFrame->m_wndOutput.AddString(temp_string);

		for (int i = 0; i < sub_folders.size() && m_bStopChecking == false; i++)
		{
			ExtractFullTextFolder(folder + "\\" + sub_folders[i]);
		}
	}
}

void CAutoSplitPagesView::OnCheckPreserveExsiting()
{
	m_bPreserveExsiting = !m_bPreserveExsiting;
}


void CAutoSplitPagesView::OnUpdateCheckPreserveExsiting(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bPreserveExsiting);
}


void CAutoSplitPagesView::OnButtonOcrSelPageText()
{
}


void CAutoSplitPagesView::OnCheckAll()
{
	m_iSelPageType = 0;
}


void CAutoSplitPagesView::OnCheckCur()
{
	m_iSelPageType = 0;
}


void CAutoSplitPagesView::OnCheckSel()
{
	m_iSelPageType = 0;
}


void CAutoSplitPagesView::OnUpdateCheckAll(CCmdUI *pCmdUI)
{
	if(m_iSelPageType == 0)
		pCmdUI->SetCheck(TRUE);
	else
		pCmdUI->SetCheck(FALSE);
}


void CAutoSplitPagesView::OnUpdateCheckCur(CCmdUI *pCmdUI)
{
	if (m_iSelPageType == 1)
		pCmdUI->SetCheck(TRUE);
	else
		pCmdUI->SetCheck(FALSE);
}


void CAutoSplitPagesView::OnUpdateCheckSel(CCmdUI *pCmdUI)
{
	if (m_iSelPageType == 2)
		pCmdUI->SetCheck(TRUE);
	else
		pCmdUI->SetCheck(FALSE);
}


void CAutoSplitPagesView::OnCheckCurrentPage()
{
	m_iSelPageType = 1;
}


void CAutoSplitPagesView::OnUpdateCheckCurrentPage(CCmdUI *pCmdUI)
{
	if (m_iSelPageType == 1)
		pCmdUI->SetCheck(TRUE);
	else
		pCmdUI->SetCheck(FALSE);
}

LRESULT CAutoSplitPagesView::OnGridAutoCheck(WPARAM wParam, LPARAM lParam)
{
	CAutoCheckDlg pDlg;
	if (pDlg.DoModal() == IDOK)
	{
		int start = (int)wParam;
		int end = (int)lParam;

		switch (pDlg.m_iCheckType)
		{
		case 0:
		{
			for (int i = start; i <= end; i++)
			{
				m_pFrame->m_wndTable.m_Dialog.m_Grid.QuickSetText(0, i, "1");
			}
		}
		break;
		case 1:
		{
			for (int i = start; i <= end; i++)
			{
				m_pFrame->m_wndTable.m_Dialog.m_Grid.QuickSetText(0, i, "0");
			}
		}
		break;
		case 2:
		{
			int row = m_pFrame->m_wndTable.m_Dialog.m_Grid.GetNumberRows();
			for (int i = 0; i < row; i++)
			{
				m_pFrame->m_wndTable.m_Dialog.m_Grid.QuickSetText(0, i, "0");
			}
		}
		break;
		}

		m_pFrame->m_wndTable.m_Dialog.m_Grid.RedrawAll();
	}
	return 0;
}


void CAutoSplitPagesView::OnButtonOcrSel()
{
	int page_count = m_arrImages.size();
	vector< int > sel_page_index;
	for (int i = 0; i < page_count; i++)
	{
		CString sel_str = m_pFrame->m_wndTable.m_Dialog.m_Grid.QuickGetText(0, i);
		if (sel_str == "1")
		{
			sel_page_index.push_back(i);
		}
	}

	BeginProgress();
	m_strProgressStatus = "Extract Selection Area Text";
	UpdateProgress(0, sel_page_index.size());

	CString page_string;
	CString page_txt_string;
	for (int index = 0; index < sel_page_index.size() && index < m_arrRectCode.size(); index++)
	{
		UpdatePageImages(sel_page_index[index]);
		SavePageCodeImage(sel_page_index[index]);

		ExtractSelectOCR(sel_page_index[index]);
		page_string.Format("[ %d / %d ]Extract Text from : %s", index + 1, sel_page_index.size(), m_arrImages[sel_page_index[index]].file_path);
		m_pFrame->m_wndOutput.AddString(page_string);
		UpdateProgress(index + 1, sel_page_index.size());
	}
	EndProgress();
	UpdatePageImages(m_iCurrentFileIndex);
	m_pFrame->m_wndTable.m_Dialog.m_Grid.RedrawAll();
}
