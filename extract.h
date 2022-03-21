/* * * * * * * *\
    EXTRACT.H -
        Made in 2022 by freedom
    DESCRIPTION -
        Header hand-made for extract.c by freedom :)
\* * * * * * * */

// Pragmas
#pragma once

// Typedefs

/* Header of the resource file in the new format */

typedef struct tagNEWHEADER {
    WORD    Reserved;
    WORD    ResType;
    WORD    ResCount;
} NEWHEADER, * LPNEWHEADER;

typedef struct tagICONDIR
{
    BYTE  Width;            /* 16, 32, 64 */
    BYTE  Height;           /* 16, 32, 64 */
    BYTE  ColorCount;       /* 2, 8, 16 */
    BYTE  reserved;
} ICONDIR;

typedef struct tagRESDIR
{
    ICONDIR Icon;
    WORD    Planes;
    WORD    BitCount;
    DWORD   BytesInRes;
    WORD    idIcon;
} RESDIR, * LPRESDIR;

typedef struct tagRESDIRDISK
{
    struct  tagICONDIR  Icon;

    WORD   Reserved[2];
    DWORD  BytesInRes;
    DWORD  Offset;
} RESDIRDISK, * LPRESDIRDISK;

typedef struct new_exe          NEWEXE, * LPNEWEXE;
typedef struct exe_hdr          EXEHDR, * LPEXEHDR;
typedef struct rsrc_nameinfo    RESNAMEINFO, * LPRESNAMEINFO;
typedef struct rsrc_typeinfo    RESTYPEINFO, * LPRESTYPEINFO;
typedef struct new_rsrc         RESTABLE, * LPRESTABLE;

typedef struct new_exe          NEWEXEHDR;
typedef NEWEXEHDR* PNEWEXEHDR;

typedef struct
{
    HANDLE hAppInst;
    HANDLE hFileName;
    HANDLE hIconList;
    INT    nIcons;
} EXTRACTICONINFO;

typedef struct _MyIconInfo
{
    HICON hIcon;
    INT   iIconId;
} MYICONINFO, *LPMYICONINFO;

// Definitions
#define InternalExtractIconList InternalExtractIconListW
#define ExtractIconResInfo		ExtractIconResInfoW

// Function Prototypes for extract.c
LPBYTE FindResWithIndex(LPBYTE lpResTable, INT iResIndex, LPBYTE lpResType);
INT GetResIndex(LPBYTE lpResTable, INT iResId, LPBYTE lpResType);
HANDLE SimpleLoadResource(HFILE fh, LPBYTE lpResTable, INT iResIndex, LPBYTE lpResType);
VOID FreeIconList(HANDLE hIconList, int iKeepIcon);
HANDLE APIENTRY InternalExtractIconListW(HANDLE hInst, LPWSTR lpszExeFileName, LPINT lpnIcons);
WORD APIENTRY ExtractIconResInfoW(HANDLE hInst, LPWSTR lpszFileName, WORD wIconIndex, LPWORD lpwSize, LPHANDLE lphIconRes);

