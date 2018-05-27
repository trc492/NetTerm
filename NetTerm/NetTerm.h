#if 0
/// Copyright (c) Titan Robotics Club. All rights reserved.
///
/// <module name="NetTerm.h" />
///
/// <summary>
///     This module contains the common definitions of the NetTerm program.
/// </summary>
///
/// <remarks>
///     Author: Michael Tsang (18-Apr-2012)
///     Environment: Windows application.
/// </remarks>
#endif

#pragma once

//
// Constants.
//

// Program constants.
#define PROG_NAME               L"NetTerm"
#define PROG_TITLE              L"Network Terminal for NI cRIO"
#define PROG_COPYRIGHT          L"Copyright (c) Titan Robotics Club (Team 492). " \
                                L"All rights reserved."
#define PROG_VERSION            L"Version 1.0"

#define NETTERMF_SHUTDOWN       0x80000000
#define NETTERMF_APPENDLF       0x00000001
#define NETTERMF_DUMPBIN        0x00000002
#define NETTERMF_LINEMODE       0x00000004
#define NETTERMF_NOCLIENT       0x00000008
#define NETTERMF_TCP            0x00000010

// Network constants.
#define REGSTR_PATH_NETTERM     L"SOFTWARE\\FIRST\\FRC\\NetTerm"
#define REGSTR_VALUE_REMOTEADDR L"RemoteAddr"
#define REGSTR_VALUE_REMOTEPORT L"RemotePort"
#define REGSTR_VALUE_LOCALPORT  L"LocalPort"

#define RECV_BUFF_SIZE          1024

#define KEYCODE_EXTENDED        0xe0
#define KEYCODE_F12             0x86
#define KEYCODE_CTRL_F12        0x8a

//
// Type definitions.
//
typedef struct _ConfigParams
{
    WCHAR szRemoteAddr[16];
    WCHAR szRemotePort[8];
    WCHAR szLocalPort[8];
    int   sockType;
    int   protocol;
} CONFIG_PARAMS, *PCONFIG_PARAMS;

//
// Global data.
//
extern DWORD   g_progFlags;
extern FILE   *g_hLogFile;
extern CmdArg  g_cmdArg;

//
// Function prototypes.
//

// Misc.cpp
HRESULT
PrintHelp(
    __in PARG_ENTRY argEntry
    );

HRESULT
RegistryGetConfig(
    __inout PCONFIG_PARAMS configParams
    );

HRESULT
RegistrySaveConfig(
    __in PCONFIG_PARAMS configParams
    );

