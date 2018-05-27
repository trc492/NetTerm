#if 0
/// Copyright (c) Titan Robotics Club. All rights reserved.
///
/// <module name="NetTerm.cpp" />
///
/// <summary>
///     A console app implementing an ANSI terminal communicating with the
///     cRIO over the network using UDP protocol. It supports ANSI escape
///     sequence to change font color attributes.
/// </summary>
///
/// <remarks>
///     Author: Michael Tsang (18-Apr-2012)
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
LPCWSTR         g_progName = NULL;
DWORD           g_progFlags = 0;
FILE           *g_hLogFile = NULL;

//
// Local data.
//
LPWSTR          g_pszLogFile = NULL;
DWORD           g_teamNumber = 0;
LPWSTR          g_pszLocal = NULL;
LPWSTR          g_pszRemote = NULL;
CONFIG_PARAMS   g_configParams = {L"10.0.0.2", L"6668", L"6666",
                                  SOCK_DGRAM, IPPROTO_UDP};
ARG_ENTRY       g_cmdArgs[] =
                {
                    {
                        L"?", ARGTYPE_FUNCTION,
                        PrintHelp, 0,
                        NULL,
                        L"Print usage syntax summary"
                    },
                    {
                        L"help", ARGTYPE_FUNCTION,
                        PrintHelp, 0,
                        NULL,
                        L"Print usage help message"
                    },
                    {
                        L"appendlf", ARGTYPE_SWITCH,
                        &g_progFlags, NETTERMF_APPENDLF,
                        NULL,
                        L"Append line-feed at end-of-line of the received data"
                    },
                    {
                        L"dumpbin", ARGTYPE_SWITCH,
                        &g_progFlags, NETTERMF_DUMPBIN,
                        NULL,
                        L"Dump received data as binary data"
                    },
                    {
                        L"linemode", ARGTYPE_SWITCH,
                        &g_progFlags, NETTERMF_LINEMODE,
                        NULL,
                        L"Send data by line instead of by character"
                    },
                    {
                        L"noclient", ARGTYPE_SWITCH,
                        &g_progFlags, NETTERMF_NOCLIENT,
                        NULL,
                        L"Do not start network client"
                    },
                    {
                        L"tcp", ARGTYPE_SWITCH,
                        &g_progFlags, NETTERMF_TCP,
                        NULL,
                        L"Use TCP protocol instead of UDP"
                    },
                    {
                        L"log", ARGTYPE_STRING,
                        &g_pszLogFile, 0,
                        L"=<LogFile>",
                        L"Write all received data to log file"
                    },
                    {
                        L"team", ARGTYPE_NUMERIC,
                        &g_teamNumber, 10,
                        L"=<TeamNum>",
                        L"Specifies FRC team number"
                    },
                    {
                        L"local", ARGTYPE_STRING,
                        &g_pszLocal, 0,
                        L"=<Port>",
                        L"Specifies local port (default: 6666)"
                    },
                    {
                        L"remote", ARGTYPE_STRING,
                        &g_pszRemote, 0,
                        L"=<Addr>:<Port>",
                        L"Specifies remote IP and port (default: 10.0.0.2:6668)"
                    },
                    {
                        NULL, ARGTYPE_NONE,
                        NULL, 0,
                        NULL, NULL
                    }
                };
CmdArg          g_cmdArg(g_cmdArgs);

/**
 *  This callback handles console event such as ctrl+c and ctrl+break. By
 *  registering this control handler, we prevent the program from terminating
 *  so that it can clean up before exiting. This is mainly for restoring the
 *  console text attributes before this program is run.
 *
 *  @param dwCtrlType Specifies the control event type.
 *
 *  @return Returns TRUE to prevent other console control handler from running
 *          including the default control handler that will terminate the
 *          program.
 */
BOOL
WINAPI
ConsoleCtrlHandler(
    __in DWORD dwCtrlType
    )
{
    BOOL rc = FALSE;

    TLevel(CALLBK);
    TEnterMsg(("ctrlType=%d", dwCtrlType));

    switch (dwCtrlType)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
        //
        // This allows the program to orderly shutdown.
        //
        printf("Shutting down, press <Enter> to exit...\n");
        g_progFlags |= NETTERMF_SHUTDOWN;
        rc = TRUE;
        break;
    }

    TExitMsg(("=%d", rc));
    return rc;
}   //ConsoleCtrlHandler

/**
 *  This program provides the console access to the cRIO over the network.
 *
 *  @param icArgc Specifies the number of command line arguments.
 *  @param apszArgs Points to the array of string argument pointers.
 *
 *  @return Success: Returns ERROR_SUCCESS.
 *  @return Failure: Returns Win32 error code.
 */
int __cdecl
wmain(
    __in                int icArgs,
    __in_ecount(icArgs) LPWSTR *apszArgs
    )
{
    HRESULT hr = S_OK;
    BOOL fParamsChanged = FALSE;
    Console *console = NULL;
    NetConn *netConn = NULL;

    TLevel(INIT);
    TraceInit(TRACE_MODULES, TRACE_LEVEL, MSG_LEVEL);
    TEnterMsg(("icArgs=%d,apszArgs=%p", icArgs, apszArgs));

    g_progName = g_cmdArg.ParseProgramName(apszArgs[0], PROG_NAME);
    icArgs--;
    apszArgs++;

    hr = RegistryGetConfig(&g_configParams);
    if (SUCCEEDED(hr) &&
        ((hr = g_cmdArg.ParseArguments(icArgs, apszArgs, TRUE)) == S_OK))
    {
        if (g_progFlags & NETTERMF_TCP)
        {
            g_configParams.sockType = SOCK_STREAM;
            g_configParams.protocol = IPPROTO_TCP;
        }

        if (g_pszLogFile != NULL)
        {
            if (_wfopen_s(&g_hLogFile, g_pszLogFile, L"wb") != 0)
            {
                hr = HRESULT_FROM_WIN32(ERROR_OPEN_FAILED);
                MsgPrintf(g_progName, MSGTYPE_ERR, hr,
                          L"Failed to create log file <%s>.",
                          g_pszLogFile);
            }
        }

        if (g_teamNumber != 0)
        {
            StringCchPrintfW(g_configParams.szRemoteAddr,
                             ARRAYSIZE(g_configParams.szRemoteAddr),
                             L"10.%d.%d.2",
                             g_teamNumber/100,
                             g_teamNumber%100);
            StringCchCopyW(g_configParams.szRemotePort,
                           ARRAYSIZE(g_configParams.szRemotePort),
                           L"6668");
            StringCchCopyW(g_configParams.szLocalPort,
                           ARRAYSIZE(g_configParams.szLocalPort),
                           L"6666");
            fParamsChanged = TRUE;
        }

        if (g_pszLocal != NULL)
        {
            StringCchCopyW(g_configParams.szLocalPort,
                           ARRAYSIZE(g_configParams.szLocalPort),
                           g_pszLocal);
            fParamsChanged = TRUE;
        }

        if (g_pszRemote != NULL)
        {
            LPWSTR psz = wcschr(g_pszRemote, L':');

            if (psz == NULL)
            {
                hr = E_INVALIDARG;
                MsgPrintf(g_progName, MSGTYPE_ERR, hr,
                          L"Invalid remote address/port <%s>.",
                          g_pszRemote);
            }
            else
            {
                *psz = L'\0';
                StringCchCopyW(g_configParams.szRemoteAddr,
                               ARRAYSIZE(g_configParams.szRemoteAddr),
                               g_pszRemote);
                StringCchCopyW(g_configParams.szRemotePort,
                               ARRAYSIZE(g_configParams.szRemotePort),
                               psz + 1);
                fParamsChanged = TRUE;
            }
        }

        if (SUCCEEDED(hr) && fParamsChanged)
        {
            RegistrySaveConfig(&g_configParams);
        }
    }

    if (SUCCEEDED(hr))
    {
        if (((console = new Console(ConsoleCtrlHandler)) != NULL) &&
            ((netConn = new NetConn()) != NULL))
        {
            PrintTitle();
            printf("Connecting to remote %ws port %ws:%ws...\n",
                   (g_configParams.protocol == IPPROTO_TCP)? L"TCP": L"UDP",
                   g_configParams.szRemoteAddr, g_configParams.szRemotePort);
            if (!(g_progFlags & NETTERMF_NOCLIENT))
            {
                printf("Receiving from local %ws port %ws...\n",
                       (g_configParams.protocol == IPPROTO_TCP)?
                            L"TCP": L"UDP",
                       g_configParams.szLocalPort);
            }
            printf("\nPress <Ctrl+F12> to exit.\n\n");
            hr = netConn->Initialize(&g_configParams, console);
        }
    }

    if (SUCCEEDED(hr))
    {
        if (g_progFlags & NETTERMF_NOCLIENT)
        {
            while (!(g_progFlags & NETTERMF_SHUTDOWN) && (_getch() != 0))
            {
            }
        }
        else
        {
            DWORD dwcb;

            while (!(g_progFlags & NETTERMF_SHUTDOWN))
            {
                if (g_progFlags & NETTERMF_LINEMODE)
                {
                    static char szLineBuff[RECV_BUFF_SIZE];

                    if (fgets(szLineBuff, ARRAYSIZE(szLineBuff), stdin))
                    {
                        hr = netConn->SendData((LPBYTE)szLineBuff,
                                                (DWORD)strlen(szLineBuff),
                                                &dwcb,
                                                INFINITE);
                    }
                }
                else
                {
                    static int idx = 0;
                    BYTE ch[2];

                    ch[idx] = (BYTE)_getch();
                    if (ch[idx] == KEYCODE_EXTENDED)
                    {
                        idx++;
                    }
                    else if (idx == 0)
                    {
                        hr = netConn->SendData(ch,
                                               1,
                                               &dwcb,
                                               INFINITE);
                    }
                    else if (ch[idx] == KEYCODE_CTRL_F12)
                    {
                        g_progFlags |= NETTERMF_SHUTDOWN;
                        idx = 0;
                    }
                    else
                    {
                        hr = netConn->SendData(ch,
                                               2,
                                               &dwcb,
                                               INFINITE);
                        idx = 0;
                    }
                }

                if (FAILED(hr))
                {
                    MsgPrintf(g_progName, MSGTYPE_ERR, hr,
                                L"Failed to write to server.");
                }
            }
        }
    }

    SAFE_DELETE(netConn);
    SAFE_DELETE(console);
    if (g_hLogFile != NULL)
    {
        fclose(g_hLogFile);
    }

    TExitMsg(("=%x", hr));
    return hr;
}   //wmain

