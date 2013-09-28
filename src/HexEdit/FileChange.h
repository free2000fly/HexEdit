//
//  FileChange.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef FILECHANGE_INCLUDED
#define FILECHANGE_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NOTIFY_DATA {
	TCHAR		szFile[MAX_PATH];
	HWND		hwndNotify;
	HANDLE		hQuitEvent;
    HANDLE hThread;
} NOTIFY_DATA;

typedef struct NMFILECHANGE {
	NMHDR hdr;
	NOTIFY_DATA * data;
} NMFILECHANGE, *PNMFILECHANGE;

#define FCN_FILECHANGE 2000

HANDLE CreateFileChangeNotifier(LPCTSTR szPathName, HWND hwndNotify);
void CloseFileChangeNotifier(HANDLE notifier);

#ifdef __cplusplus
}
#endif

#endif
