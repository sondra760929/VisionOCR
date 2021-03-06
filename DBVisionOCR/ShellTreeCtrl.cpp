// ShellTreeCtrl.cpp: 구현 파일
//

#include "stdafx.h"
#include "AutoSplitPages.h"
#include "ShellTreeCtrl.h"
#include "MainFrm.h"
#include "AutoSplitPagesView.h"

// CShellTreeCtrl
bool GetHasSubItems(HWND hwndOwner, LPSHELLFOLDER ShellFolder, int Flags)
// Finds if a shellfolder has children - a side effect of which is reconnecting network connections
// Note : If user input is required to perform the enumeration (username and password dialog)
// hwndOwner must be a owner form handle, If hwndOwner = 0 the connection
// and user input is required, it will silently fail.
{
	LPENUMIDLIST EnumList = NULL;
	HRESULT hr = ShellFolder->EnumObjects(NULL, Flags, &EnumList);
	LPITEMIDLIST pidlTemp;
	DWORD dwFetched = 1;

	// Enumerate the item's PIDLs:
	if (SUCCEEDED(EnumList->Next(1, &pidlTemp, &dwFetched)) && dwFetched)
	{
		return true;
	}
	return false;
}

LPITEMIDLIST GetPIDLFromPath(CString a_path)
{
	LPSHELLFOLDER pisfDesktop;
	if (SUCCEEDED(SHGetDesktopFolder(&pisfDesktop)))
	{
		LPITEMIDLIST pidl2 = NULL;
		pisfDesktop->AddRef();
		HRESULT hr = pisfDesktop->ParseDisplayName(0, NULL, CA2W(a_path), 0, &pidl2, 0);
		if (SUCCEEDED(hr))
		{
			return pidl2;
		}
	}
	return NULL;
}

bool GetPIDLShellFolder(LPITEMIDLIST pidl, LPSHELLFOLDER piFolder)
{
	LPSHELLFOLDER piDesktopFolder;
	if (SUCCEEDED(SHGetDesktopFolder(&piDesktopFolder)))
	{
		if (SUCCEEDED(piDesktopFolder->BindToObject(pidl, NULL, IID_IShellFolder, (void**)&piFolder)))
		{
			return true;
		}
	}
	return false;
}

bool ConnectToPath(HWND hwndOwner, CString APath)
{
	LPITEMIDLIST PItemIDList = GetPIDLFromPath(APath);
	if (PItemIDList != NULL)
	{
		LPSHELLFOLDER ShellFolder = NULL;
		if (GetPIDLShellFolder(PItemIDList, ShellFolder))
		{
			return GetHasSubItems(hwndOwner, ShellFolder, SHCONTF_FOLDERS | SHCONTF_INCLUDEHIDDEN);
		}
	}
	return false;
}

IMPLEMENT_DYNAMIC(CShellTreeCtrl, CMFCShellTreeCtrl)

CShellTreeCtrl::CShellTreeCtrl()
{
}

CShellTreeCtrl::~CShellTreeCtrl()
{
}


BEGIN_MESSAGE_MAP(CShellTreeCtrl, CMFCShellTreeCtrl)
	ON_NOTIFY_REFLECT(NM_DBLCLK, &CShellTreeCtrl::OnNMDblclk)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, &CShellTreeCtrl::OnTvnSelchanged)
	ON_NOTIFY_REFLECT(TVN_KEYDOWN, &CShellTreeCtrl::OnTvnKeydown)
	ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT, &CShellTreeCtrl::OnTvnEndlabeledit)
END_MESSAGE_MAP()



// CShellTreeCtrl 메시지 처리기

HRESULT CShellTreeCtrl::EnumObjects(HTREEITEM hParentItem, LPSHELLFOLDER pParentFolder, LPITEMIDLIST pidlParent)
{
	ASSERT_VALID(this);
	ASSERT_VALID(afxShellManager);

	LPENUMIDLIST pEnum = NULL;
	//m_dwFlags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_SHAREABLE | SHCONTF_STORAGE | SHCONTF_INCLUDESUPERHIDDEN;
	HRESULT hr = pParentFolder->EnumObjects(NULL, m_dwFlags, &pEnum);
	if (FAILED(hr) || pEnum == NULL)
	{
		return hr;
	}

	LPITEMIDLIST pidlTemp;
	DWORD dwFetched = 1;

	// Enumerate the item's PIDLs:
	while (SUCCEEDED(pEnum->Next(1, &pidlTemp, &dwFetched)) && dwFetched)
	{
		TVITEM tvItem;
		ZeroMemory(&tvItem, sizeof(tvItem));

		// Fill in the TV_ITEM structure for this item:
		tvItem.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;

		// AddRef the parent folder so it's pointer stays valid:
		pParentFolder->AddRef();

		// Put the private information in the lParam:
		LPAFX_SHELLITEMINFO pItem = (LPAFX_SHELLITEMINFO)GlobalAlloc(GPTR, sizeof(AFX_SHELLITEMINFO));
		ENSURE(pItem != NULL);

		pItem->pidlRel = pidlTemp;
		pItem->pidlFQ = afxShellManager->ConcatenateItem(pidlParent, pidlTemp);

		pItem->pParentFolder = pParentFolder;
		tvItem.lParam = (LPARAM)pItem;

		CString strItem = OnGetItemText(pItem);
		tvItem.pszText = strItem.GetBuffer(strItem.GetLength());
		tvItem.iImage = OnGetItemIcon(pItem, FALSE);
		tvItem.iSelectedImage = OnGetItemIcon(pItem, TRUE);

		// Determine if the item has children:
		DWORD dwAttribs = SFGAO_HASSUBFOLDER | SFGAO_FOLDER | SFGAO_DISPLAYATTRMASK | SFGAO_CANRENAME | SFGAO_FILESYSANCESTOR;

		pParentFolder->GetAttributesOf(1, (LPCITEMIDLIST*)&pidlTemp, &dwAttribs);
		tvItem.cChildren = (dwAttribs & (SFGAO_HASSUBFOLDER | SFGAO_FILESYSANCESTOR));

		// Determine if the item is shared:
		if (dwAttribs & SFGAO_SHARE)
		{
			tvItem.mask |= TVIF_STATE;
			tvItem.stateMask |= TVIS_OVERLAYMASK;
			tvItem.state |= INDEXTOOVERLAYMASK(1); //1 is the index for the shared overlay image
		}

		// Fill in the TV_INSERTSTRUCT structure for this item:
		TVINSERTSTRUCT tvInsert;

		tvInsert.item = tvItem;
		tvInsert.hInsertAfter = TVI_LAST;
		tvInsert.hParent = hParentItem;

		string str(strItem);

		if (str != "제어판")
		{
			if (tvItem.cChildren & SFGAO_HASSUBFOLDER)
			{
				InsertItem(&tvInsert);
			}
			else
			{
				//string ext = (str.substr(str.find_last_of(".") + 1));
				//if ((ext == "pdf") || (ext == "xls") || (ext == "xlsx"))
				//{
					InsertItem(&tvInsert);
				//}
			}
		}
		dwFetched = 0;
	}

	pEnum->Release();
	return S_OK;
}


void CShellTreeCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	CString file_path;
	if (GetItemPath(file_path, GetSelectedItem()))
	{
		ShellExecute(m_hWnd, "open", file_path, nullptr, nullptr, SW_SHOW);
	}

	*pResult = 0;
}


void CShellTreeCtrl::OnTvnSelchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	CString file_path;
	if (GetItemPath(file_path, GetSelectedItem()))
	{
		m_strCurrentPath = file_path;
		CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
		if (pFrame)
		{
			CAutoSplitPagesView* pview = (CAutoSplitPagesView*)(pFrame->GetActiveView());
			if (pview)
			{
				pview->FillInImages(m_strCurrentPath);
				pFrame->m_wndFileView.SetFolderText(m_strCurrentPath);
				//pview->SelectFileFromTree(file_path);
				this->SetFocus();
			}
		}
	}
	*pResult = 0;
}


void CShellTreeCtrl::OnTvnKeydown(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVKEYDOWN pTVKeyDown = reinterpret_cast<LPNMTVKEYDOWN>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	if (pTVKeyDown->wVKey == VK_F2)
	{
		if (PathFileExists(m_strCurrentPath))
		{
			HTREEITEM sel_item = GetSelectedItem();
			if (sel_item)
			{
				canUpdateLabel = false;
				m_strCurrentLabel = GetItemText(sel_item);
				m_pEditLabel = EditLabel(sel_item);
			}
		}
	}
	if (pTVKeyDown->wVKey == VK_F5)
	{
		if (PathFileExists(m_strCurrentPath))
		{
			Refresh();
			SelectPath(m_strCurrentPath);
		}
	}
	else if (pTVKeyDown->wVKey == VK_RETURN)
	{
		EndEditLabelNow(FALSE);
		m_pEditLabel = NULL;
	}
	else if (pTVKeyDown->wVKey == VK_ESCAPE)
	{
		EndEditLabelNow(TRUE);
		m_pEditLabel = NULL;
	}
	*pResult = 0;
}


void CShellTreeCtrl::OnTvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	if (canUpdateLabel)
	{
		CString file_name = pTVDispInfo->item.pszText;
		CString file_path = m_strCurrentPath;
		if (PathFileExists(file_path))
		{
			CString new_file_path;
			char drive[_MAX_DRIVE];
			char dir[_MAX_DIR];
			char fname[_MAX_FNAME];
			char ext[_MAX_EXT];
			_splitpath_s(file_path, drive, sizeof drive, dir, sizeof dir, fname, sizeof fname, ext, sizeof ext);

			if (m_strCurrentLabel.ReverseFind('.') > -1)
			{
				new_file_path.Format("%s%s%s", drive, dir, file_name);
			}
			else
			{
				new_file_path.Format("%s%s%s%s", drive, dir, file_name, ext);
			}

			if (MoveFile(file_path, new_file_path))
			{
				Refresh();
				SelectPath(new_file_path);
			}
			else
			{

			}
		}
		canUpdateLabel = false;
	}
	*pResult = 0;
}

