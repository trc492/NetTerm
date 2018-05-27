#if 0
/// Copyright (c) Titan Robotics Club. All rights reserved.
///
/// <module name="WinNetTerm.cpp" />
///
/// <summary>
///     A Windows app that communicates with the cRIO over the network using
///     UDP protocol.
/// </summary>
///
/// <remarks>
///     Author: Michael Tsang (01-May-2012)
///     Environment: Windows application.
/// </remarks>
#endif

#define _MAIN_FILE
#include "StdAfx.h"

#ifdef MOD_ID
    #undef MOD_ID
#endif
#define MOD_ID                  MOD_MAIN

//
// Global data.
//
HINSTANCE       g_hInstance = NULL;
LPCWSTR         g_progClass = L"WinNetTermClass";
WCHAR           g_progName[16] = L"WinApp";
CONFIG_PARAMS   g_configParams = {L"10.0.0.2",
                                  L"6668",
                                  L"6666",
                                  (DWORD)CW_USEDEFAULT,
                                  (DWORD)CW_USEDEFAULT,
                                  (DWORD)CW_USEDEFAULT,
                                  (DWORD)CW_USEDEFAULT};

/**
 *  This program provides the console access to the cRIO over the network.
 *
 *  @param hInstance Specifies the current instance handle to the program.
 *  @param hPrevInstance Specifies the previous instance handle.
 *  @param pszCmdLine Points to the command line string.
 *  @param nCmdShow Specifies how the window is to be shown.
 *
 *  @return Success: Returns ERROR_SUCCESS.
 *  @return Failure: Returns Win32 error code.
 */
int WINAPI
wWinMain(
    __in HINSTANCE hInstance,
    __in HINSTANCE hPrevInstance,
    __in LPWSTR    pszCmdLine,
    __in int       nCmdShow
    )
{
    HRESULT hr = S_OK;
    HWND hwnd = NULL;

    TLevel(INIT);
    TraceInit(TRACE_MODULES, TRACE_LEVEL, MSG_LEVEL);
    TEnterMsg(("hInst=%p,prevInst=%p,cmdLine=%ws,nCmdShow=%d",
               hInstance, hPrevInstance, pszCmdLine, nCmdShow));

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(pszCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    g_hInstance = hInstance;
    if (!LoadStringW(g_hInstance,
                     IDS_APP_TITLE,
                     g_progName,
                     ARRAYSIZE(g_progName)))
    {
        hr = GETLASTHRESULT();
        MsgPrintf(g_progName, MSGTYPE_ERR, hr,
                  L"Failed to load program title.");
    }
    else if ((hwnd = FindWindowW(g_progClass, g_progName)) != NULL)
    {
        //
        // WinNetTerm is already running, just set focus to it.
        //
        SetForegroundWindow(hwnd);
    }
    else if ((hr = RegistryGetConfig(&g_configParams)) != S_OK)
    {
        MsgPrintf(g_progName, MSGTYPE_ERR, hr,
                  L"Failed to get program parameters from the registry.");
    }
    else if ((hr = ParseCmdParams(&g_configParams)) != S_OK)
    {
        MsgPrintf(g_progName, MSGTYPE_ERR, hr,
                  L"Invalid command line syntax.\n\n"
                  L"Usage:\t%s [/t <TeamNumber>]\n"
                  L"\t%s [/l <LocalPort>] [/r <RemoteAddr:<RemoteAddr>]",
                  g_progName, g_progName);
    }
    else
    {
        WNDCLASSEXW wcex;

        RtlZeroMemory(&wcex, sizeof(wcex));
        wcex.cbSize = sizeof(wcex);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = TerminalWndProc;
        wcex.hInstance = g_hInstance;
        wcex.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_WINNETTERM));
        wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszMenuName = MAKEINTRESOURCE(IDD_MENU);
        wcex.lpszClassName = g_progClass;
        wcex.hIconSm = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_SMALL));
        if (!RegisterClassExW(&wcex))
        {
            hr = GETLASTHRESULT();
            MsgPrintf(g_progName, MSGTYPE_ERR, hr,
                      L"Failed to register Window class.");
        }
        else
        {
            HWND hwnd;

            hwnd = CreateWindowW(g_progClass,
                                    g_progName,
                                    WS_OVERLAPPEDWINDOW,
                                    g_configParams.xPos,
                                    g_configParams.yPos,
                                    g_configParams.nWidth,
                                    g_configParams.nHeight,
                                    NULL,
                                    NULL,
                                    g_hInstance,
                                    NULL);
            if (hwnd == NULL)
            {
                hr = GETLASTHRESULT();
                MsgPrintf(g_progName, MSGTYPE_ERR, hr,
                            L"Failed to create terminal window.");
            }
            else
            {
                MSG msg;
                BOOL bRet;

                ShowWindow(hwnd, nCmdShow);
                UpdateWindow(hwnd);

                while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
                {
                    if (bRet == -1)
                    {
                        hr = GETLASTHRESULT();
                        TErr(("Failed to get window message (hr=%x).", hr));
                    }
                    else
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }
            }
            UnregisterClassW(g_progClass, g_hInstance);
        }
    }

    TExitMsg(("=%x", hr));
    return hr;
}   //wWinMain
