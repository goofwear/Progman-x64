/* * * * * * * *\
	PMTRAY.H -
		Adapted from various sources, made by Freedom
	DESCRIPTION -
		This file acts as the header for the notification area.
\* * * * * * * */

// Pragmas
#pragma once

// Includes
#include "progman.h"

// Definitions
#define szClassName		TEXT("Notification Area")

// Variables
BOOL		bRet;
HWND		hwndTray;
HINSTANCE	hinstTray;

// Function Prototypes
VOID TrayMain();
