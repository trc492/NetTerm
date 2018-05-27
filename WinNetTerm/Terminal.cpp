#if 0
/// Copyright (c) Titan Robotics Club. All rights reserved.
///
/// <module name="Terminal.cpp" />
///
/// <summary>
///     This module implements the functions for the terminal window.
/// </summary>
///
/// <remarks>
///     Author: Michael Tsang (03-May-2012)
///     Environment: Windows application.
/// </remarks>
#endif

#include "StdAfx.h"

#ifdef MOD_ID
    #undef MOD_ID
#endif
#define MOD_ID                  MOD_TERMINAL

#define INPUT_LINE_HEIGHT       22
#define STATIC_BOX_WIDTH        60
#define BUTTON_WIDTH            80

NetConn*g_netConn = NULL;
WCHAR   g_szLineBuff[LINE_BUFF_SIZE];
char    g_sendBuff[LINE_BUFF_SIZE];
HWND    g_hwndTerm = NULL;
HWND    g_hwndInput = NULL;
HWND    g_hwndStatic = NULL;
HWND    g_hwndSend = NULL;

/**
 *  This is a simple DialogProc that does nothing else but some basic
 *  processing.
 *
 *  @param hwndDlg Specifies the handle to the dialog box.
 *  @param uMsg Specifies the dialog message.
 *  @param wParam Specifies the message specific parameter.
 *  @param lParam Specifies the message specific parameter.
 *
 *  @return Returns TRUE if the DlgProc processed the message, FALSE otherwise
 */
INT_PTR
CALLBACK
SimpleDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    INT_PTR rc = FALSE;
    HWND hwndChild;

    TLevel(HIFREQ);
    TEnterMsg(("hwnd=%p,uMsg=%x,wParam=%x,lParam=%x",
               hwndDlg, uMsg, wParam, lParam));

    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        hwndChild = GetDlgItem(hwndDlg, IDOK);
        if (hwndChild == NULL)
        {
            hwndChild = GetDlgItem(hwndDlg, IDYES);
        }

        if (hwndChild != NULL)
        {
            SetFocus(hwndChild);
        }
        break;

    case WM_COMMAND:
        EndDialog(hwndDlg, wParam);
        break;
    }

    TExitMsg(("=%x", rc));
    return rc;
}   //SimpleDlgProc

/**
 *  This function is called to enumerate each child window.
 *
 *  @param hwnd Specifies the handle to the child window.
 *  @param lParam Specifies the parameter.
 *
 *  @return Returns TRUE to continue enumeration, FALSE otherwise.
 */
BOOL
CALLBACK
EnumChildProc(
    __in HWND hwnd,
    __in LPARAM lParam
    )
{
    BOOL rc = TRUE;
    LPRECT rcParent = (LPRECT)lParam;
    int idChild;

    TLevel(CALLBK);
    TEnterMsg(("hwnd=%p,lParam=%p", hwnd, lParam));

    idChild = GetWindowLong(hwnd, GWL_ID);
    switch (idChild)
    {
    case IDC_TERMINAL_TEXT:
        MoveWindow(hwnd,
                   0,
                   0,
                   rcParent->right,
                   rcParent->bottom - INPUT_LINE_HEIGHT,
                   TRUE);
        break;

    case IDC_INPUT_TEXT:
        MoveWindow(hwnd,
                   STATIC_BOX_WIDTH,
                   rcParent->bottom - INPUT_LINE_HEIGHT,
                   rcParent->right - STATIC_BOX_WIDTH - BUTTON_WIDTH,
                   INPUT_LINE_HEIGHT,
                   TRUE);
        break;

    case IDC_STATIC_TEXT:
        MoveWindow(hwnd,
                   0,
                   rcParent->bottom - INPUT_LINE_HEIGHT,
                   STATIC_BOX_WIDTH,
                   INPUT_LINE_HEIGHT,
                   TRUE);
        break;

    case IDC_BUTTON_SEND:
        MoveWindow(hwnd,
                   rcParent->right - BUTTON_WIDTH,
                   rcParent->bottom - INPUT_LINE_HEIGHT,
                   BUTTON_WIDTH,
                   INPUT_LINE_HEIGHT,
                   TRUE);
        break;
    }
    ShowWindow(hwnd, SW_SHOW);

    TExitMsg(("=%d", rc));
    return rc;
}   //EnumChildProc

/**
 *  This is the WndProc for the program.
 *
 *  @param hwnd Specifies the handle to the window.
 *  @param uMsg Specifies the window message.
 *  @param wParam Specifies the message specific parameter.
 *  @param lParam Specifies the message specific parameter.
 *
 *  @return Returns message dependent result code. 
 */
LRESULT
CALLBACK
TerminalWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    LRESULT rc = 0;
    static HFONT hfontTerm = NULL;
    static HBRUSH hbrBkgnd = NULL;
    RECT rcClient;

    TLevel(HIFREQ);
    TEnterMsg(("hwnd=%p,uMsg=%x,wParam=%x,lParam=%x",
               hwnd, uMsg, wParam, lParam));

    switch (uMsg)
    {
    case WM_CREATE:
        if ((g_hwndTerm = CreateWindowW(
                                 L"EDIT",
                                 NULL,
                                 (WS_CHILD | WS_BORDER | WS_VSCROLL |
                                  ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL |
                                  ES_READONLY),
                                 0, 0, 0, 0,
                                 hwnd,
                                 (HMENU)IDC_TERMINAL_TEXT,
                                 g_hInstance,
                                 NULL)) == NULL)
        {
            MsgPrintf(g_progName, MSGTYPE_ERR, GETLASTHRESULT(),
                      L"Failed to create terminal window.");
        }
        else if ((g_hwndInput = CreateWindowW(
                                  L"EDIT",
                                  NULL,
                                  WS_CHILD | WS_BORDER | ES_LEFT,
                                  0, 0, 0, 0,
                                  hwnd,
                                  (HMENU)IDC_INPUT_TEXT,
                                  g_hInstance,
                                  NULL)) == NULL)
        {
            MsgPrintf(g_progName, MSGTYPE_ERR, GETLASTHRESULT(),
                      L"Failed to create input window.");
        }
        else if ((g_hwndStatic = CreateWindowW(
                                   L"STATIC",
                                   L"Input->",
                                   WS_CHILD | WS_BORDER | SS_CENTER,
                                   0, 0, 0, 0,
                                   hwnd,
                                   (HMENU)IDC_STATIC_TEXT,
                                   g_hInstance,
                                   NULL)) == NULL)
        {
            MsgPrintf(g_progName, MSGTYPE_ERR, GETLASTHRESULT(),
                      L"Failed to create static window.");
        }
        else if ((g_hwndSend = CreateWindowW(
                                   L"BUTTON",
                                   L"Send",
                                   WS_CHILD | SS_CENTER,
                                   0, 0, 0, 0,
                                   hwnd,
                                   (HMENU)IDC_BUTTON_SEND,
                                   g_hInstance,
                                   NULL)) == NULL)
        {
            MsgPrintf(g_progName, MSGTYPE_ERR, GETLASTHRESULT(),
                      L"Failed to create send button.");
        }
        else if ((hfontTerm = CreateFontW(0, 0, 0, 0, FW_DONTCARE,
                                          FALSE, FALSE, FALSE,
                                          ANSI_CHARSET,
                                          OUT_DEFAULT_PRECIS,
                                          CLIP_DEFAULT_PRECIS,
                                          DEFAULT_QUALITY,
                                          FIXED_PITCH,
                                          L"FixedSys")) == NULL)
        {
            MsgPrintf(g_progName, MSGTYPE_ERR, GETLASTHRESULT(),
                      L"Failed to create terminal font.");
        }
        else if ((hbrBkgnd = CreateSolidBrush(RGB(0, 0, 0))) == NULL)
        {
            MsgPrintf(g_progName, MSGTYPE_ERR, GETLASTHRESULT(),
                      L"Failed to create background brush.");
        }
        else if ((g_netConn = new NetConn()) == NULL)
        {
            MsgPrintf(g_progName, MSGTYPE_ERR, 0,
                      L"Failed to create net connection.");
        }
        else if (g_netConn->Initialize(&g_configParams, g_hwndTerm) ==
                 S_OK)
        {
            WCHAR szTitle[64];

            StringCchPrintfW(szTitle, ARRAYSIZE(szTitle), L"%s [%s]",
                             g_progName, g_configParams.szRemoteAddr);
            SetWindowTextW(hwnd, szTitle);
            SendMessageW(g_hwndTerm, WM_SETFONT, (WPARAM)hfontTerm, TRUE);
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        if (hfontTerm != NULL)
        {
            DeleteObject(hfontTerm);
        }
        if (hbrBkgnd != NULL)
        {
            DeleteObject(hbrBkgnd);
        }
        SAFE_DELETE(g_netConn);
        PostQuitMessage(0);
        break;

    case WM_CTLCOLORSTATIC:
        if ((HWND)lParam == g_hwndTerm)
        {
            SetBkColor((HDC)wParam, RGB(0, 0, 0));
            SetTextColor((HDC)wParam, RGB(0, 255, 0));
            rc = (INT_PTR)hbrBkgnd;
        }
        break;

    case WM_WINDOWPOSCHANGED:
    {
        LPWINDOWPOS winPos = (LPWINDOWPOS)lParam;
        g_configParams.xPos = winPos->x;
        g_configParams.yPos = winPos->y;
        g_configParams.nWidth = winPos->cx;
        g_configParams.nHeight = winPos->cy;
        RegistrySaveConfig(&g_configParams);
        if (!GetClientRect(hwnd, &rcClient))
        {
            MsgPrintf(g_progName, MSGTYPE_ERR, GETLASTHRESULT(),
                      L"Failed to get client rectangle.");
        }
        else
        {
            EnumChildWindows(hwnd, EnumChildProc, (LPARAM)&rcClient);
        }
        break;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDM_FILE_SAVEAS:
            break;

        case IDM_FILE_EXIT:
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            break;

        case IDM_CONFIG:
            break;

        case IDM_HELP_ABOUT:
            DialogBox(g_hInstance,
                      MAKEINTRESOURCE(IDD_ABOUTBOX),
                      hwnd,
                      SimpleDlgProc);
            break;

        case IDC_INPUT_TEXT:
            if (HIWORD(wParam) == EN_CHANGE)
            {
//                MsgPrintf(g_progName, MSGTYPE_INFO, 0,
//                          L"Input changed");
            }
            break;

        case IDC_BUTTON_SEND:
        {
            DWORD dwcb;

            GetDlgItemTextW(hwnd,
                            IDC_INPUT_TEXT,
                            g_szLineBuff,
                            ARRAYSIZE(g_szLineBuff));
            dwcb = WideCharToMultiByte(CP_ACP,
                                       0,
                                       g_szLineBuff,
                                       wcslen(g_szLineBuff) + 1,
                                       (LPSTR)g_sendBuff,
                                       sizeof(g_sendBuff),
                                       NULL,
                                       NULL);
            StringCbCatA((LPSTR)g_sendBuff, sizeof(g_sendBuff), "\n");
            g_netConn->SendData((LPBYTE)g_sendBuff,
                                strlen((LPSTR)g_sendBuff),
                                &dwcb,
                                INFINITE);
#if 0
            MsgPrintf(g_progName, MSGTYPE_INFO, 0,
                      L"len=%d, String=<%S>",
                      strlen((LPSTR)g_sendBuff), g_sendBuff);
//            SendMessageW(hwndTerm, EM_REPLACESEL, FALSE, (LPARAM)szLineBuff);
#endif
            SetDlgItemTextW(hwnd, IDC_INPUT_TEXT, L"");
            break;
        }
        }
        break;

    default:
        rc = DefWindowProc(hwnd, uMsg, wParam, lParam);
        break;
    }

    TExitMsg(("=%x", rc));
    return rc;
}   //TerminalWndProc
