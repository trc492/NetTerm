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
#include <shellapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <strsafe.h>
#include <stdlib.h>

//#define _USE_TCP
//#define _ENABLE_FUNCTRACE
//#define _ENABLE_MSGTRACE
#define _USE_COLORFONT

//
// Tracing Info.
//
#define MOD_TERMINAL            TGenModId(1)
#define MOD_CONFIG              TGenModId(2)
#define MOD_UTIL                TGenModId(3)
#define MOD_CLIENT              TGenModId(4)
#define MOD_SERVER              TGenModId(5)
#define MOD_DLIST               TGenModId(6)

#define TRACE_MODULES           (MOD_MAIN | MOD_TERMINAL | MOD_CONFIG)
#define TRACE_LEVEL             FUNC
#define MSG_LEVEL               INFO

//
// Constants
//

//
// Macros.
//
#define GETLASTHRESULT()    HRESULT_FROM_WIN32(GetLastError())
#define SAFE_DELETE(p)      if ((p) != NULL)            \
                            {                           \
                                delete(p);              \
                                (p) = NULL;             \
                            }

#include "DbgTrace.h"
#include "Ansi.h"
#include "DList.h"
#include "WsaServer.h"
#include "WsaClient.h"
#include "Resource.h"
#include "WinNetTerm.h"
#include "NetConn.h"
