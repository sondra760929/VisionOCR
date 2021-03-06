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

// stdafx.cpp : 표준 포함 파일만 들어 있는 소스 파일입니다.
// AutoSplitPages.pch는 미리 컴파일된 헤더가 됩니다.
// stdafx.obj에는 미리 컴파일된 형식 정보가 포함됩니다.

#include "stdafx.h"


CString m_strTempPath;
CString m_strAppPath;
CString m_strKey = _T("SOFTWARE\\DIGIBOOK\\MASKING\\Settings");

COLORREF m_color1 = RGB(255, 0, 0);
COLORREF m_color2 = RGB(0, 0, 255);
CAutoSplitPagesView* m_pCurrentView = NULL;

BOOL WriteStringValueInRegistry(HKEY hKey, LPCTSTR lpSubKey, LPCTSTR lpValueKey, LPCTSTR lpValue)
{
	CString strValue = _T("");
	HKEY hSubKey = NULL;

	// open the key 
	if (::RegOpenKeyEx(hKey, lpSubKey, 0, KEY_ALL_ACCESS, &hSubKey) == ERROR_SUCCESS)
	{
		DWORD cbSize = (DWORD)strlen(lpValue) + 1;
		BYTE *pBuf = new BYTE[cbSize];
		::ZeroMemory(pBuf, cbSize);
		::CopyMemory(pBuf, lpValue, cbSize - 1);
		::RegSetValueEx(hSubKey, lpValueKey, NULL, REG_SZ, pBuf, cbSize);

		// 키 닫기
		::RegCloseKey(hSubKey);
		delete[] pBuf;
		return TRUE;
	}
	else
	{
		long status = ::RegCreateKeyEx(hKey, lpSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hSubKey, NULL);
		if (status == ERROR_SUCCESS)
		{
			DWORD cbSize = (DWORD)strlen(lpValue) + 1;
			BYTE *pBuf = new BYTE[cbSize];
			::ZeroMemory(pBuf, cbSize);
			::CopyMemory(pBuf, lpValue, cbSize - 1);
			::RegSetValueEx(hSubKey, lpValueKey, NULL, REG_SZ, pBuf, cbSize);

			// 키 닫기
			::RegCloseKey(hSubKey);
			delete[] pBuf;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL WriteDWORDValueInRegistry(HKEY hKey, LPCTSTR lpSubKey, LPCTSTR lpValueKey, DWORD dValue)
{
	CString strValue = _T("");
	HKEY hSubKey = NULL;

	// open the key 
	if (::RegOpenKeyEx(hKey, lpSubKey, 0, KEY_ALL_ACCESS, &hSubKey) == ERROR_SUCCESS)
	{
		::RegSetValueEx(hSubKey, lpValueKey, NULL, REG_DWORD, (const BYTE*)&dValue, sizeof(dValue));

		// 키 닫기
		::RegCloseKey(hSubKey);
		return TRUE;
	}
	else
	{
		long status = ::RegCreateKeyEx(hKey, lpSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hSubKey, NULL);
		if (status == ERROR_SUCCESS)
		{
			::RegSetValueEx(hSubKey, lpValueKey, NULL, REG_DWORD, (const BYTE*)&dValue, sizeof(dValue));

			// 키 닫기
			::RegCloseKey(hSubKey);
			return TRUE;
		}
	}
	return FALSE;
}

CString ReadStringValueInRegistry(HKEY hKey, LPCTSTR lpSubKey, LPCTSTR lpValueKey)
{
	CString strValue = _T("");
	HKEY hSubKey = NULL;

	// open the key
	if (::RegOpenKeyEx(hKey, lpSubKey, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
	{
		DWORD buf_size = 0;

		// 문자열의 크기를 먼저 읽어온다. 
		if (::RegQueryValueEx(hSubKey, lpValueKey, NULL, NULL, NULL, &buf_size) == ERROR_SUCCESS)
		{
			// 메모리 할당하고..., 
			TCHAR *pBuf = new TCHAR[buf_size + 1];

			// 실제 값을 읽어온다. 
			if (::RegQueryValueEx(hSubKey, lpValueKey, NULL, NULL, (LPBYTE)pBuf, &buf_size) == ERROR_SUCCESS)
			{
				pBuf[buf_size] = _T('\0');
				strValue = CString(pBuf);
			}

			// to avoid leakage 
			delete[] pBuf;
		}

		// 키 닫기 
		::RegCloseKey(hSubKey);
	}
	return strValue;
}

DWORD ReadDWORDValueInRegistry(HKEY hKey, LPCTSTR lpSubKey, LPCTSTR lpValueKey)
{
	DWORD dwReturn = -1;
	DWORD dwBufSize = sizeof(DWORD);
	HKEY hSubKey = NULL;

	// open the key
	if (::RegOpenKeyEx(hKey, lpSubKey, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
	{
		DWORD error = RegQueryValueEx(hSubKey, lpValueKey, NULL, NULL, reinterpret_cast<LPBYTE>(&dwReturn), &dwBufSize);
		if (error == ERROR_SUCCESS)
		{
			printf("Key value is: %d \n", dwReturn);
		}
		else
		{
			printf("Cannot query for key value; Error is: %d\n", error);
		}
		// 키 닫기 
		::RegCloseKey(hSubKey);
	}
	return dwReturn;
}