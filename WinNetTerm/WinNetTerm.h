#if 0
/// Copyright (c) Titan Robotics Club. All rights reserved.
///
/// <module name="WinNetTerm.h" />
///
/// <summary>
///     This module contains the common definitions of the WinNetTerm program.
/// </summary>
///
/// <remarks>
///     Author: Michael Tsang (01-May-2012)
///     Environment: Windows application.
/// </remarks>
#endif

#pragma once

//
// Constants.
//
#ifdef _USE_TCP
  #define SOCKTYPE              SOCK_STREAM
  #define PROTOCOL              IPPROTO_TCP
#else
  #define SOCKTYPE              SOCK_DGRAM
  #define PROTOCOL              IPPROTO_UDP
#endif

#define LINE_BUFF_SIZE          1024
#define RECV_BUFF_SIZE          1024

#define REGSTR_PATH_WINNETTERM  L"SOFTWARE\\FIRST\\FRC\\WinNetTerm"
#define REGSTR_VALUE_REMOTEADDR L"RemoteAddr"
#define REGSTR_VALUE_REMOTEPORT L"RemotePort"
#define REGSTR_VALUE_LOCALPORT  L"LocalPort"
#define REGSTR_VALUE_XPOS       L"XPos"
#define REGSTR_VALUE_YPOS       L"YPos"
#define REGSTR_VALUE_WIDTH      L"Width"
#define REGSTR_VALUE_HEIGHT     L"Height"

#define MSGTYPE_INFO            0
#define MSGTYPE_WARN            1
#define MSGTYPE_ERR             2

//
// Type definitions.
//
typedef struct _ConfigParams
{
    WCHAR szRemoteAddr[16];
    WCHAR szRemotePort[8];
    WCHAR szLocalPort[8];
    DWORD xPos;
    DWORD yPos;
    DWORD nWidth;
    DWORD nHeight;
} CONFIG_PARAMS, *PCONFIG_PARAMS;

//
// Function prototypes.
//

// Terminal.cpp
LRESULT
CALLBACK
TerminalWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

// Util.cpp
HRESULT
MsgPrintf(
    __in_opt LPCWSTR pszTitle,
    __in     UINT msgType,
    __in     DWORD errCode,
    __in     LPCWSTR pszFormat,
    ...
    );

HRESULT
RegistryGetConfig(
    __inout PCONFIG_PARAMS configParams
    );

HRESULT
RegistrySaveConfig(
    __in PCONFIG_PARAMS configParams
    );

HRESULT
ParseCmdParams(
    __inout PCONFIG_PARAMS configParams
    );

//
// Global data.
//
extern HINSTANCE        g_hInstance;
extern LPCWSTR          g_progClass;
extern WCHAR            g_progName[];
extern CONFIG_PARAMS    g_configParams;
