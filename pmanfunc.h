/* * * * * * * *\
	PMANFUNC.H -
		Made by Freedom
	DESCRIPTION -
		This file contains functions for making Program Manager work better
		Adapted from internal Microsoft functions
\* * * * * * * */

// Includes
#include <Windows.h>

// Definitions
#define CheckEscapes		CheckEscapesW
#define GetDockedState()	GetDockedStateNT()
// RunFileDlg flags
#define RFF_NOBROWSE		0x0001	// Removes the browse button
#define RFF_NODEFAULT		0x0002	// No default item selected
#define RFF_CALCDIRECTORY	0x0004	// Calculates the working directory from the file name
#define RFF_NOLABEL			0x0008	// Removes the edit box label
#define RFF_NOSEPARATEMEM	0x0020  // Removes the Separate Memory Space check box, NT only
// RunFileDlg notification return values
#define RF_OK		        0x0000	// Allow the application to run
#define RF_CANCEL	        0x0001	// Cancel the operation and close the dialog
#define RF_RETRY            0x0002	// Cancel the operation, but leave the dialog open

// Function prototypes
BOOL WINAPI SetInternalWindowPos(IN HWND hWnd, IN UINT cmdShow, IN LPRECT lpRect, IN LPPOINT lpPoint);
WORD WINAPI GetInternalWindowPos(IN HWND hWnd, OUT LPRECT lpRect, OUT LPPOINT lpPoint);
VOID APIENTRY CheckEscapesW(LPWSTR szFile, DWORD cch);
LPTSTR SkipProgramName(LPTSTR lpCmdLine);
int MyAtoi(LPTSTR string);
BOOL RunFile(HWND hWndOwner, HICON hIcon, LPWSTR lpszDir, LPWSTR lpszTitle, LPWSTR lpszDesc, DWORD dwFlags);
BOOL ShutdownDlg(HWND hWndOwner);
BOOL CascadeChildWindows(HWND hwndParent, UINT nCode);
UINT GetIconIdFromIndex(LPWSTR szIconPath, UINT iIconIndex);
BOOL APIENTRY RegenerateUserEnvironment(void** pNewEnv, BOOL bSetCurrentEnv);
