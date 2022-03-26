/* * * * * * * *\
	PMANFUNC.C -
		Made by Freedom
	DESCRIPTION -
		This file contains functions for making Program Manager work better
		Adapted from internal Microsoft functions
\* * * * * * * */

// Includes
#define _NO_CRT_STDIO_INLINE
#include <Shlwapi.h>
#include <strsafe.h>
#include <powerbase.h>
#include <powrprof.h>
#undef LoadEjectFunction
#include "pmanfunc.h"
#include "progman.h"

// Definitions
#define WCHAR_SPACE			L' '
#define WCHAR_COMMA			L','
#define WCHAR_SEMICOLON		L';'
#define WCHAR_HAT			L'^'
#define WCHAR_QUOTE			L'"'
#define WCHAR_NULL			L'\0'
#define INT_SIZE_LENGTH		20

#define UNLEN	256

// Shell related
#define SHGetUserDisplayName	GetUserName

// Function Prototypes
BOOL WINAPI SetInternalWindowPos(IN HWND hWnd, IN UINT cmdShow, IN LPRECT lpRect, IN LPPOINT lpPoint)
{
	BOOL bSetSuccess = TRUE;

	// lpPoint isn't used anymore. Keep this in mind.

	if (lpRect) {
		bSetSuccess = SetWindowPos(hWnd, HWND_NOTOPMOST, lpRect->left, lpRect->top, lpRect->right - lpRect->left, lpRect->bottom - lpRect->top, SWP_NOZORDER | SWP_FRAMECHANGED);
	}
	else {
		// if(IsIconic(hWnd))
		bSetSuccess = SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}

	ShowWindow(hWnd, cmdShow);

	return bSetSuccess;
}

WORD WINAPI GetInternalWindowPos(IN HWND hWnd, OUT LPRECT lpRect, OUT LPPOINT lpPoint)
{
	WINDOWPLACEMENT wp;
	BOOL bSetSuccess = TRUE;

	wp.length = sizeof(WINDOWPLACEMENT);
	bSetSuccess = GetWindowPlacement(hWnd, &wp);

	if (!bSetSuccess)
		return 0;

	if (lpRect) {
		CopyRect(lpRect, &wp.rcNormalPosition);
	}

	if (lpPoint) {
		*lpPoint = wp.ptMinPosition;
	}

	return wp.showCmd;
}

VOID APIENTRY CheckEscapesW(LPWSTR szFile, DWORD cch)
{
	LPWSTR szT;
	WCHAR* p, * pT;

	for (p = szFile; *p; p++) {

		switch (*p) {
		case WCHAR_SPACE:
		case WCHAR_COMMA:
		case WCHAR_SEMICOLON:
		case WCHAR_HAT:
		case WCHAR_QUOTE:
		{
			// this path contains an annoying character
			if (cch < (wcslen(szFile) + 2)) {
				return;
			}
			szT = (LPWSTR)LocalAlloc(LPTR, cch * sizeof(WCHAR));
			if (!szT) {
				return;
			}
			StringCchCopy(szT, cch, szFile); // ok to truncate, we checked size above
			p = szFile;
			*p++ = WCHAR_QUOTE;
			for (pT = szT; *pT; ) {
				*p++ = *pT++;
			}
			*p++ = WCHAR_QUOTE;
			*p = WCHAR_NULL;
			LocalFree(szT);
			return;
		}
		}
	}
}

LPTSTR SkipProgramName(LPTSTR lpCmdLine)
{
	LPTSTR  p = lpCmdLine;
	BOOL    bInQuotes = FALSE;

	//
	// Skip executable name
	//
	for (p; *p; p = CharNext(p))
	{
		if ((*p == TEXT(' ') || *p == TEXT('\t')) && !bInQuotes)
			break;

		if (*p == TEXT('\"'))
			bInQuotes = !bInQuotes;
	}

	while (*p == TEXT(' ') || *p == TEXT('\t'))
		p++;

	return (p);
}

int MyAtoi(LPTSTR  string)
{
	CHAR   szAnsi[INT_SIZE_LENGTH];
	BOOL   fDefCharUsed;

#ifdef UNICODE
	WideCharToMultiByte(CP_ACP, 0, string, INT_SIZE_LENGTH,
		szAnsi, INT_SIZE_LENGTH, NULL, &fDefCharUsed);

	// return(atoi(szAnsi));
	return(strtol(szAnsi, NULL, 10));
#else
	return(atoi(string));
#endif

}

BOOL RunFile(HWND hWndOwner,
	HICON	hIcon,
	LPWSTR	lpszDir,
	LPWSTR	lpszTitle,
	LPWSTR	lpszDesc,
	DWORD	dwFlags)
{
	HMODULE hRunLib = LoadLibrary(TEXT("shell32.dll"));
	if (hRunLib) {
		FARPROC fRunLib = GetProcAddress(hRunLib, MAKEINTRESOURCE(61));
		return fRunLib(hWndOwner, hIcon, lpszDir, lpszTitle, lpszDesc, dwFlags);
		FreeLibrary(hRunLib);
	}
	return 0;
}

BOOL ShutdownDlg(HWND hWndOwner)
{
	HMODULE hRunLib = LoadLibrary(TEXT("shell32.dll"));
	if (hRunLib) {
		FARPROC fRunLib = GetProcAddress(hRunLib, MAKEINTRESOURCE(60));
		return fRunLib(hWndOwner);
		FreeLibrary(hRunLib);
	}
	return 0;
}

BOOL CascadeChildWindows(HWND hwndParent, UINT nCode)
{
	return (BOOL)CascadeWindows(hwndParent, nCode, NULL, 0, NULL);
}

// GetIconIdFromIndex -
// turn an icon's index into a resource id
// through the slowest means possible
// RETURN VALUE: FALSE if function failed,
// positive UINT if function successful
UINT GetIconIdFromIndex(LPWSTR szIconPath, UINT iIconIndex)
{
	HMODULE	hModule; // used to load the HMOD for our icon file
	LPTSTR	lpIconId; // used as the lock for LockResource
	WORD	cbIconId; // used for something else i guess
	HICON	hIconIndex = { NULL }; // this will contain the hIcon that's wanted
	HICON	hIconId = { NULL }; // this will contain our comparison hIcon
	UINT	iIconAmount; // how many icons in the file?
	UINT	iIconId = 0; // resource id of icon

	// if we get bad values then don't continue
	if (!szIconPath)
		return 0;
	if (!iIconIndex)
		return 0;

	// create the hIcon that we really want
	hIconIndex = ExtractIcon(hAppInstance, &szIconPath, iIconIndex);
	if (hIconIndex == 1 || hIconIndex == NULL)
		return 0;

	// load the resource file
	hModule = LoadLibraryEx(szIconPath, NULL, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
	if (hModule == NULL)
		return 0; // no i will not return GetLastError.

	// get amount of icons in said resource file
	iIconAmount = ExtractIcon(hAppInstance, &szIconPath, -1);

	// the fun part, loop until the hicons are identical
	do {
		iIconId++;
		hIconId = FindResource(hModule, (LPTSTR)MAKEINTRESOURCE(iIconIndex), (LPTSTR)MAKEINTRESOURCE(RT_ICON));
		if (hIconId) {
			cbIconId = (WORD)SizeofResource(hModule, hIconId);
			hIconId = LoadResource(hModule, hIconId);
			lpIconId = LockResource(hIconId);
		}
	} while (hIconIndex != hIconId || iIconId > iIconAmount);
	
	// destroy any icons that may have been created
	if (hIconIndex)
		DestroyIcon(hIconIndex);
	if (hIconId)
		DestroyIcon(hIconId);

	// return icon resource id if possible
	if (iIconId)
		return iIconId;
	else
		return 0;
}

BOOL APIENTRY RegenerateUserEnvironment(void** pNewEnv, BOOL bSetCurrentEnv)
{
	HMODULE hRunLib = LoadLibrary(TEXT("shell32.dll"));
	if (hRunLib) {
		FARPROC fRunLib = GetProcAddress(hRunLib, "RegenerateUserEnvironment");
		return fRunLib(pNewEnv, bSetCurrentEnv);
		FreeLibrary(hRunLib);
	}
	return 0;
}
