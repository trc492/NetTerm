#if 0
/// Copyright (c) Titan Robotics Club. All rights reserved.
///
/// <module name="StdAfx.h" />
///
/// <summary>
///     Pre-compile C header file.
/// </summary>
///
/// <remarks>
///     Author: Michael Tsang (18-Apr-2012)
///     Environment: Windows application.
/// </remarks>
#endif

#pragma once

#include <SDKDDKVer.h>
#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <strsafe.h>
#include <stdlib.h>
#include <conio.h>

//#define _ENABLE_FUNCTRACE
//#define _ENABLE_MSGTRACE
#define _USE_COLORFONT

//
// Tracing Info.
//
#define MOD_NETCONN             TGenModId(1)
#define MOD_CONSOLE             TGenModId(2)
#define MOD_MISC                TGenModId(3)
#define MOD_UTIL                TGenModId(4)
#define MOD_CMDARG              TGenModId(5)
#define MOD_CLIENT              TGenModId(6)
#define MOD_SERVER              TGenModId(7)
#define MOD_DLIST               TGenModId(8)

#define TRACE_MODULES           (MOD_MAIN)
#define TRACE_LEVEL             FUNC
#define MSG_LEVEL               INFO

//
// Constants
//

//
// Macros.
//

//
// Function prototypes.
//

//
// Global data.
//
extern LPCWSTR g_progName;

#include "DbgTrace.h"
#include "Util.h"
#include "CmdArg.h"
#include "Ansi.h"
#include "DList.h"
#include "WsaServer.h"
#include "WsaClient.h"
#include "NetTerm.h"
#include "Console.h"
#include "NetConn.h"
