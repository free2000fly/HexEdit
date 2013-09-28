//
//  FileChange.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#define _WIN32_WINNT 0x501
#include <windows.h>
#include <tchar.h>
#include <assert.h>
#include <shlwapi.h>
#include "trace.h"
#include "FileChange.h"

DWORD WINAPI ChangeNotifyThread(LPVOID lpParam)
{
    HANDLE hChange;
    HANDLE hFind;
    DWORD  dwResult;
    TCHAR  szDirectory[MAX_PATH];
    WIN32_FIND_DATA fdCurFile = { 0 };
    NOTIFY_DATA *pnd = (NOTIFY_DATA *)lpParam;
    int nCurrentTime = 0;
    NMFILECHANGE nmfc = { { pnd->hwndNotify, 0, FCN_FILECHANGE }, pnd };

    lstrcpy(szDirectory, pnd->szFile);

    // get the directory name from filename
    if(GetFileAttributes(szDirectory) != FILE_ATTRIBUTE_DIRECTORY) {
        PathRemoveFileSpec(szDirectory);
    }

    // Save data of current file
    hFind = FindFirstFile(pnd->szFile, &fdCurFile);
    if (hFind != INVALID_HANDLE_VALUE) {
        FindClose(hFind);
    }

    // watch the specified directory for changes
    hChange = FindFirstChangeNotification(szDirectory, FALSE,
        FILE_NOTIFY_CHANGE_FILE_NAME  | \
        FILE_NOTIFY_CHANGE_DIR_NAME   | \
        FILE_NOTIFY_CHANGE_ATTRIBUTES | \
        FILE_NOTIFY_CHANGE_SIZE | \
        FILE_NOTIFY_CHANGE_LAST_WRITE);

    do {
        WIN32_FIND_DATA fdUpdated = { 0 };
        HANDLE hFind = NULL;
        HANDLE hEventList[2] = { hChange, pnd->hQuitEvent };

        dwResult = WaitForMultipleObjects(_countof(hEventList), hEventList, FALSE, INFINITE);
        if(dwResult != WAIT_OBJECT_0) {
            break;
        }
        // Check if the changes affect the current file
        hFind = FindFirstFile(pnd->szFile, &fdUpdated);
        if (INVALID_HANDLE_VALUE != hFind) {
            FindClose(hFind);
        }
        // Check if the file has been changed
        if (CompareFileTime(&fdCurFile.ftLastWriteTime,&fdUpdated.ftLastWriteTime) != 0 ||
            fdCurFile.nFileSizeLow != fdUpdated.nFileSizeLow ||
            fdCurFile.nFileSizeHigh != fdUpdated.nFileSizeHigh)
        {
            if (GetTickCount() - nCurrentTime > 1000) {
                PostMessage(pnd->hwndNotify, WM_NOTIFY, 0, (LPARAM)&nmfc);
                fdCurFile = fdUpdated;
                nCurrentTime = GetTickCount();
            }
        }
        FindNextChangeNotification(hChange);
    } while(TRUE);

    // cleanup
    FindCloseChangeNotification(hChange);
    CloseHandle(pnd->hQuitEvent);
    free(pnd);

    return 0;
}

HANDLE CreateFileChangeNotifier(LPCTSTR szPathName, HWND hwndNotify)
{
    NOTIFY_DATA *pnd = (NOTIFY_DATA *) calloc(1, sizeof(NOTIFY_DATA));
    HANDLE hThread = NULL;
    DWORD threadID = 0;

    pnd->hwndNotify = hwndNotify;
    lstrcpy(pnd->szFile, szPathName);

    hThread = CreateThread(0, 0, ChangeNotifyThread, pnd, CREATE_SUSPENDED, &threadID);

    if (hThread) {
        TCHAR evtName[MAX_PATH];
        _stprintf(evtName, _T("Notifier_%d"), (int)hThread);
        pnd->hQuitEvent = CreateEvent(NULL, FALSE, FALSE, evtName);
        pnd->hThread = hThread;
        ResumeThread(hThread);
    } else {
        free(pnd);
    }

    return hThread;
}

void CloseFileChangeNotifier(HANDLE notifier)
{
    if (notifier) {
        HANDLE quitEvent;
        TCHAR evtName[MAX_PATH];
        _stprintf(evtName, _T("Notifier_%d"), (int)notifier);
        quitEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, evtName);

        if (quitEvent) {
            SetEvent(quitEvent);
            if (WaitForSingleObject(notifier, INFINITE) != WAIT_OBJECT_0) {
                assert(FALSE);
                TerminateThread(notifier, 0xdead);
            }
            CloseHandle(notifier);
            CloseHandle(quitEvent);
        } else {
            TerminateThread(notifier, 0xdead);
        }
    }
}
