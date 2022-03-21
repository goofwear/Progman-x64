// #include "shellprv.h"
#pragma  hdrstop

// #include "newres.h"

// BEGIN NEW INCLUDES
#include <Windows.h>
#include <Shlwapi.h>
#include "extract.h"
#include "newexe.h"

#define try         __try
#define except      __except

#define ExtractIcons PrivateExtractIconsW
// END NEW INCLUDES

#define ICON_MAGIC      0
#define ICO_MAGIC1      1
#define CUR_MAGIC1      2
#define BMP_MAGIC       ((WORD)'B'+((WORD)'M'<<8))
#define ANI_MAGIC       ((WORD)'R'+((WORD)'I'<<8))
#define ANI_MAGIC1      ((WORD)'F'+((WORD)'F'<<8))
#define ANI_MAGIC4      ((WORD)'A'+((WORD)'C'<<8))
#define ANI_MAGIC5      ((WORD)'O'+((WORD)'N'<<8))
#define MZMAGIC         ((WORD)'M'+((WORD)'Z'<<8))
#define PEMAGIC         ((WORD)'P'+((WORD)'E'<<8))
#define LEMAGIC         ((WORD)'L'+((WORD)'E'<<8))

#define RESOURCE_VA(x)        ((x)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress)
#define RESOURCE_SIZE(x)      ((x)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size)
#define NUMBER_OF_SECTIONS(x) ((x)->FileHeader.NumberOfSections)

#define FCC(c0,c1,c2,c3) ((DWORD)(c0)|((DWORD)(c1)<<8)|((DWORD)(c2)<<16)|((DWORD)(c3)<<24))

#define COM_FILE        FCC('.', 'c', 'o', 'm')
#define BAT_FILE        FCC('.', 'b', 'a', 't')
#define CMD_FILE        FCC('.', 'c', 'm', 'd')
#define PIF_FILE        FCC('.', 'p', 'i', 'f')
#define LNK_FILE        FCC('.', 'l', 'n', 'k')
#define ICO_FILE        FCC('.', 'i', 'c', 'o')
#define EXE_FILE        FCC('.', 'e', 'x', 'e')

#define M_llseek(fh, lOff, iOrg)            SetFilePointer((HANDLE)IntToPtr( fh ), lOff, NULL, (DWORD)iOrg)

#define MAGIC_ICON30            0
#define MAGIC_MARKZIBO          ((WORD)'M'+((WORD)'Z'<<8))

#define SEEK_FROMZERO           0
#define SEEK_FROMCURRENT        1
#define SEEK_FROMEND            2
#define NSMOVE                  0x0010
#define VER                     0x0300

#define CCHICONPATHMAXLEN 128

// This returns a pointer to the rsrc_nameinfo of the resource with the
// given index and type, if it is found, otherwise it returns NULL.

LPBYTE FindResWithIndex(LPBYTE lpResTable, INT iResIndex, LPBYTE lpResType)
{
    LPRESTYPEINFO lpResTypeInfo = (LPRESTYPEINFO)(lpResTable + sizeof(WORD));

    while (lpResTypeInfo->rt_id)
    {
        if ((lpResTypeInfo->rt_id & RSORDID) &&
            (MAKEINTRESOURCE(lpResTypeInfo->rt_id & ~RSORDID) == (LPTSTR)lpResType))
        {
            if (lpResTypeInfo->rt_nres > (WORD)iResIndex)
                return((LPBYTE)(lpResTypeInfo + 1) + iResIndex * sizeof(RESNAMEINFO));
        else
            return NULL;
        }
        lpResTypeInfo = (LPRESTYPEINFO)((LPBYTE)(lpResTypeInfo + 1) + lpResTypeInfo->rt_nres * sizeof(RESNAMEINFO));
    }
    return NULL;
}

/* This returns the index (1-relative) of the given resource-id
* in the resource table, if it is found, otherwise it returns NULL.
*/

INT GetResIndex(LPBYTE lpResTable, INT iResId, LPBYTE lpResType)
{
    WORD w;
    LPRESNAMEINFO lpResNameInfo;
    LPRESTYPEINFO lpResTypeInfo = (LPRESTYPEINFO)(lpResTable + sizeof(WORD));

    while (lpResTypeInfo->rt_id)
    {
        if ((lpResTypeInfo->rt_id & RSORDID) && (MAKEINTRESOURCE(lpResTypeInfo->rt_id & ~RSORDID) == (LPTSTR)lpResType))
        {
            lpResNameInfo = (LPRESNAMEINFO)(lpResTypeInfo + 1);
            for (w = 0; w < lpResTypeInfo->rt_nres; w++, lpResNameInfo++)
            {
                if ((lpResNameInfo->rn_id & RSORDID) && ((lpResNameInfo->rn_id & ~RSORDID) == iResId))
                    return(w + 1);
            }
            return 0;
        }
        lpResTypeInfo = (LPRESTYPEINFO)((LPBYTE)(lpResTypeInfo + 1) + lpResTypeInfo->rt_nres * sizeof(RESNAMEINFO));
    }
    return 0;
}


HANDLE SimpleLoadResource(HFILE fh, LPBYTE lpResTable, INT iResIndex, LPBYTE lpResType)
{
    INT      iShiftCount;
    HICON    hIcon;
    LPBYTE            lpIcon;
    DWORD             dwSize;
    DWORD             dwOffset;
    LPRESNAMEINFO     lpResPtr;

    /* The first 2 bytes in ResTable indicate the amount other values should be
    * shifted left.
    */
    iShiftCount = *((WORD*)lpResTable);

    lpResPtr = (LPRESNAMEINFO)FindResWithIndex(lpResTable, iResIndex, lpResType);

    if (!lpResPtr)
        return NULL;

    /* Left shift the offset to form a LONG. */
    dwOffset = MAKELONG(lpResPtr->rn_offset << iShiftCount, (lpResPtr->rn_offset) >> (16 - iShiftCount));
    dwSize = lpResPtr->rn_length << iShiftCount;

    if (M_llseek(fh, dwOffset, SEEK_FROMZERO) == -1L)
        return NULL;

    if (!(hIcon = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE, dwSize)))
        return NULL;

    if (!(lpIcon = GlobalLock(hIcon)))
        goto SLRErr1;

    if (_lread(fh, (LPVOID)lpIcon, dwSize) < dwSize)
        goto SLRErr2;

    GlobalUnlock(hIcon);
    return hIcon;

SLRErr2:
    GlobalUnlock(hIcon);
SLRErr1:
    GlobalFree(hIcon);
    return NULL;
}

VOID FreeIconList(HANDLE hIconList, int iKeepIcon)
{
    MYICONINFO *lpIconList;
    INT i;
    EXTRACTICONINFO ExtractIconInfo = { NULL, NULL, NULL, 0 };
    
    if (ExtractIconInfo.hIconList == hIconList) {
        ExtractIconInfo.hIconList = NULL;
    }
    if (NULL != (lpIconList = (MYICONINFO *)GlobalLock(hIconList))) {
        for (i = 0; i < ExtractIconInfo.nIcons; i++) {
            if (i != iKeepIcon) {
                DestroyIcon((lpIconList + i)->hIcon);
            }
        }
        GlobalUnlock(hIconList);
        GlobalFree(hIconList);
    }
}

// Returns a handle to a list of icons

HANDLE APIENTRY InternalExtractIconListW(HANDLE hInst, LPWSTR lpszExeFileName, LPINT lpnIcons)
{
    UINT cIcons, uiResult, i;
    UINT * lpIDs = NULL;
    HICON * lpIcons = NULL;
    HGLOBAL hIconInfo = NULL;
    MYICONINFO *lpIconInfo = NULL;
    
    
    //
    // Determine the number of icons
    //
    
    cIcons = PtrToUlong( ExtractIconW(hInst, lpszExeFileName, (UINT)-1));
    
    if (cIcons <= 0)
        return NULL;
    
    
    //
    // Allocate space for an array of UINT's and HICON's
    //
    
    lpIDs = GlobalAlloc(GPTR, cIcons * sizeof(UINT));
    if (!lpIDs) {
        goto IconList_Exit;
    }
    
    lpIcons = GlobalAlloc(GPTR, cIcons * sizeof(HICON));
    if (!lpIcons) {
        goto IconList_Exit;
    }
    
    
    //
    // Allocate space for the array of icons
    //
    
    hIconInfo = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, cIcons * sizeof(MYICONINFO));
    if (!hIconInfo) {
        goto IconList_Exit;
    }
    
    
    //
    // This has to be GlobalLock'ed since the handle is going to
    // be passed back to the application.
    //
    
    lpIconInfo = GlobalLock(hIconInfo);
    if (!lpIconInfo) {
        goto IconList_Exit;
    }
    
    
    //
    // Call ExtractIcons to do the real work.
    //
    
    uiResult = ExtractIcons(lpszExeFileName,
        0,
        GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON),
        lpIcons,
        lpIDs,
        cIcons,
        0);
    
    if (uiResult <= 0) {
        goto IconList_Exit;
    }
    
    
    //
    // Loop through the icons and fill in the array.
    //
    
    for (i=0; i < cIcons; i++) {
        lpIconInfo[i].hIcon   = lpIcons[i];
        lpIconInfo[i].iIconId = lpIDs[i];
    }
    
    
    //
    // Unlock the array handle.
    //
    
    GlobalUnlock(hIconInfo);
    
    
    //
    // Clean up allocations
    //
    
    GlobalFree(lpIDs);
    GlobalFree(lpIcons);
    
    
    //
    // Success.
    //
    
    return hIconInfo;
    
    
IconList_Exit:
    
    //
    // Error case.  Clean up and return NULL
    //
    
    if (lpIconInfo)
        GlobalUnlock(hIconInfo);
    
    if (hIconInfo)
        GlobalFree(hIconInfo);
    
    if (lpIcons)
        GlobalFree(lpIcons);
    
    if (lpIDs)
        GlobalFree(lpIDs);
    
    return NULL;
}

/*  Returns the file's format: 2 for WIndows 2.X, 3 for WIndows 3.X,        */
/*                             0 if error.                                  */
/*  Returns the handle to the Icon resource corresponding to wIconIndex     */
/*  in lphIconRes, and the size of the resource in lpwSize.                 */
/*  This is used only by Progman which needs to save the icon resource      */
/*  itself in the .GRP files (the actual icon handle is not wanted).        */
/*                                                                          */
/*  08-04-91 JohanneC      Created.                                         */

WORD APIENTRY ExtractIconResInfoW(HANDLE hInst, LPWSTR lpszFileName, WORD wIconIndex, LPWORD lpwSize, LPHANDLE lphIconRes)
{
    HFILE    fh;
    WORD     wMagic;
    BOOL     bNewResFormat;
    HANDLE   hIconDir;         /* Icon directory */
    LPBYTE   lpIconDir;
    HICON    hIcon = NULL;
    BOOL     bFormatOK = FALSE;
    INT      nIconId;
    WCHAR    szFullPath[MAX_PATH];
    int      cchPath;
    
    /* Try to open the specified file. */
    /* Try to open the specified file. */
    cchPath = SearchPathW(NULL, lpszFileName, NULL, ARRAYSIZE(szFullPath), szFullPath, NULL);
    if (cchPath == 0 || cchPath >= MAX_PATH)
        return 0;
    
    fh = HandleToLong(CreateFileW((LPCWSTR)szFullPath, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
    
    if (fh == HandleToLong(INVALID_HANDLE_VALUE)) {
        fh = HandleToLong(CreateFileW((LPCWSTR)szFullPath, GENERIC_READ, 0, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
    }
    
    if (fh == HandleToLong(INVALID_HANDLE_VALUE))
        return 0;
    
    /* Read the first two bytes in the file. */
    if (_lread(fh, (LPVOID)&wMagic, sizeof(wMagic)) != sizeof(wMagic))
        goto EIExit;
    
    switch (wMagic) {
    case MAGIC_ICON30:
        {
            INT           i;
            LPVOID        lpIcon;
            NEWHEADER     NewHeader;
            LPNEWHEADER   lpHeader;
            LPRESDIR      lpResDir;
            RESDIRDISK    ResDirDisk;
#define MAXICONS      10
            DWORD Offsets[MAXICONS];
            
            /* Only one icon per .ICO file. */
            if (wIconIndex) {
                break;
            }
            
            /* Read the header and check if it is a valid ICO file. */
            if (_lread(fh, ((LPBYTE)&NewHeader)+2, sizeof(NewHeader)-2) != sizeof(NewHeader)-2)
                goto EICleanup1;
            
            NewHeader.Reserved = MAGIC_ICON30;
            
            /* Check if the file is in correct format */
            if (NewHeader.ResType != 1)
                goto EICleanup1;
            
            /* Allocate enough space to create a Global Directory Resource. */
            hIconDir = GlobalAlloc(GHND, (LONG)(sizeof(NEWHEADER)+NewHeader.ResCount*sizeof(RESDIR)));
            if (hIconDir == NULL)
                goto EICleanup1;
            
            if ((lpHeader = (LPNEWHEADER)GlobalLock(hIconDir)) == NULL)
                goto EICleanup2;
            
            NewHeader.ResCount = (WORD)min((int)NewHeader.ResCount, MAXICONS);
            
            // fill in this structure for user
            
            *lpHeader = NewHeader;
            
            // read in the stuff from disk, transfer it to a memory structure
            // that user can deal with
            
            lpResDir = (LPRESDIR)(lpHeader + 1);
            for (i = 0; (WORD)i < NewHeader.ResCount; i++) {
                
                if (_lread(fh, (LPVOID)&ResDirDisk, sizeof(ResDirDisk)) < sizeof(RESDIR))
                    goto EICleanup3;
                
                Offsets[i] = ResDirDisk.Offset;
                
                *lpResDir = *((LPRESDIR)&ResDirDisk);
                lpResDir->idIcon = (WORD)(i+1);         // fill in the id
                
                lpResDir++;
            }
            
            /* Now that we have the Complete resource directory, let us find out the
            * suitable form of icon (that matches the current display driver).
            */
            lpIconDir = GlobalLock(hIconDir);
            if (!lpIconDir) {
                GlobalFree(hIconDir);
                goto EIErrExit;
            }
            wIconIndex = (WORD)(LookupIconIdFromDirectory(lpIconDir, TRUE) - 1);
            GlobalUnlock(hIconDir);
            lpResDir = (LPRESDIR)(lpHeader+1) + wIconIndex;
            
            /* Allocate memory for the Resource to be loaded. */
            if ((hIcon = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE, (DWORD)lpResDir->BytesInRes)) == NULL)
                goto EICleanup3;
            if ((lpIcon = GlobalLock(hIcon)) == NULL)
                goto EICleanup4;
            
            /* Seek to the correct place and read in the resource */
            if (M_llseek(fh, Offsets[wIconIndex], SEEK_FROMZERO) == -1L)
                goto EICleanup5;
            if (_lread(fh, (LPVOID)lpIcon, (DWORD)lpResDir->BytesInRes) < lpResDir->BytesInRes)
                goto EICleanup5;
            GlobalUnlock(hIcon);
            
            *lphIconRes = hIcon;
            *lpwSize = (WORD)lpResDir->BytesInRes;
            bFormatOK = TRUE;
            bNewResFormat = TRUE;
            goto EICleanup3;
            
EICleanup5:
            GlobalUnlock(hIcon);
EICleanup4:
            GlobalFree(hIcon);
            hIcon = (HICON)1;
EICleanup3:
            GlobalUnlock(hIconDir);
EICleanup2:
            GlobalFree(hIconDir);
EICleanup1:
            break;
        }
        
      case MAGIC_MARKZIBO:
          {
              INT           iTableSize;
              LPBYTE         lpResTable;
              DWORD         lOffset;
              HANDLE        hResTable;
              NEWEXEHDR     NEHeader;
              
              /* Make sure that the file is in the NEW EXE format. */
              if (M_llseek(fh, (LONG)0x3C, SEEK_FROMZERO) == -1L)
                  goto EIExit;
              if (_lread(fh, (LPVOID)&lOffset, sizeof(lOffset)) != sizeof(lOffset))
                  goto EIExit;
              if (lOffset == 0L)
                  goto EIExit;
              
              /* Read in the EXE header. */
              if (M_llseek(fh, lOffset, SEEK_FROMZERO) == -1L)
                  goto EIExit;
              if (_lread(fh, (LPVOID)&NEHeader, sizeof(NEHeader)) != sizeof(NEHeader))
                  goto EIExit;
              
              /* Is it a NEW EXE? */
              if (NE_MAGIC(NEHeader) != NEMAGIC)
                  goto EIExit;
              
              if ((NE_EXETYP(NEHeader) != NE_WINDOWS) &&
                  (NE_EXETYP(NEHeader) != NE_DEV386) &&
                  (NE_EXETYP(NEHeader) != NE_UNKNOWN))  /* Some Win2.X apps have NE_UNKNOWN in this field */
                  goto EIExit;
              
              hIcon = NULL;
              
              /* Are there any resources? */
              if (NE_RSRCTAB(NEHeader) == NE_RESTAB(NEHeader))
                  goto EIExit;
              
              /* Remember whether or not this is a Win3.0 EXE. */
              bNewResFormat = (NEHeader.ne_expver >= VER);
              
              /* Allocate space for the resource table. */
              iTableSize = NE_RESTAB(NEHeader) - NE_RSRCTAB(NEHeader);
              hResTable = GlobalAlloc(GMEM_ZEROINIT, (DWORD)iTableSize);
              if (!hResTable)
                  goto EIExit;
              
              /* Lock down the resource table. */
              lpResTable = GlobalLock(hResTable);
              if (!lpResTable) {
                  GlobalFree(hResTable);
                  goto EIExit;
              }
              
              /* Copy the resource table into memory. */
              if (M_llseek(fh, (LONG)(lOffset + NE_RSRCTAB(NEHeader)), SEEK_FROMZERO) == -1)
                  goto EIErrExit;
              if (_lread(fh, (LPBYTE)lpResTable, iTableSize) != (DWORD)iTableSize)
                  goto EIErrExit;
              
              
              /* Is this a Win3.0 EXE? */
              if (bNewResFormat) {
                  /* First, load the Icon directory. */
                  hIconDir = SimpleLoadResource(fh, lpResTable, (int)wIconIndex, (LPBYTE)RT_GROUP_ICON);
                  
                  if (!hIconDir)
                      goto EIErrExit;
                  lpIconDir = GlobalLock(hIconDir);
                  if (!lpIconDir) {
                      GlobalFree(hIconDir);
                      goto EIErrExit;
                  }
                  nIconId = LookupIconIdFromDirectory(lpIconDir, TRUE);
                  wIconIndex = (WORD)(GetResIndex(lpResTable, nIconId, (LPBYTE)RT_ICON) - 1);
                  GlobalUnlock(hIconDir);
                  /* We're finished with the icon directory. */
                  GlobalFree(hIconDir);
                  
                  
                  /* Now load the selected icon. */
                  *lphIconRes = SimpleLoadResource(fh, lpResTable, (int)wIconIndex, (LPBYTE)RT_ICON);
              }
              else {
                  /* Simply load the specified icon. */
                  *lphIconRes = SimpleLoadResource(fh, lpResTable, (int)wIconIndex, (LPBYTE)RT_ICON);
              }
              
              if (*lphIconRes) {
                  *lpwSize = (WORD)GlobalSize(*lphIconRes);
              }
              bFormatOK = TRUE;
              
EIErrExit:
              hResTable = NULL; // C4703 keeps bitching at me, oh lord!
              GlobalUnlock(hResTable);
              GlobalFree(hResTable);
              break;
          }
    }
EIExit:
    _lclose(fh);
    hInst;
    if (bFormatOK)
        return (WORD)(bNewResFormat ? 3 : 2);
    else
        return 0;
}
