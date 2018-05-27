#if 0
/// Copyright (c) Titan Robotics Club. All rights reserved.
///
/// <module name="Misc.cpp" />
///
/// <summary>
///     This module contains miscellaneous functions.
/// </summary>
///
/// <remarks>
///     Author: Michael Tsang (01-May-2012)
///     Environment: Windows application.
/// </remarks>
#endif

#include "StdAfx.h"

#ifdef MOD_ID
    #undef MOD_ID
#endif
#define MOD_ID                  MOD_MISC

/**
 *  This function prints the usage help message.
 *
 *  @param argEntry Points to the argument table entry.
 *
 *  @return Success: Returns S_OK.
 *  @return Failure: Returns HRESULT_CODE.
 */
HRESULT
PrintHelp(
    __in PARG_ENTRY argEntry
    )
{
    HRESULT hr = E_ABORT;

    TLevel(FUNC);
    TEnterMsg(("argEntry=%p", argEntry));

    PrintTitle();
    printf("Usage:\n");
    if (argEntry->name[0] == L'?')
    {
        g_cmdArg.PrintCmdHelp(g_progName, FALSE);
    }
    else
    {
        g_cmdArg.PrintCmdHelp(g_progName, TRUE);
    }

    TExitMsg(("=%x", hr));
    return hr;
}   //PrintHelp

/**
 *  This function reads the configuration from the registry if it exists.
 *  It is not an error if it doesn't exist. We will just use the default
 *  values.
 *
 *  @param configParams Points to the CONFIG_PARAMS structure to be filled
 *         in.
 *
 *  @return Success: Returns S_OK.
 *  @return Failure: Returns HRESULT_CODE.
 */
HRESULT
RegistryGetConfig(
    __inout PCONFIG_PARAMS configParams
    )
{
    HRESULT hr;
    LONG rcReg;
    HKEY hkey;

    TLevel(FUNC);
    TEnterMsg(("configParams=%p", configParams));

    rcReg = RegOpenKeyW(HKEY_CURRENT_USER, REGSTR_PATH_NETTERM, &hkey);
    if (rcReg != ERROR_SUCCESS)
    {
        //
        // It's okay to fail. It could just mean we did not save the config
        // in the registry.
        //
        TWarn(("Failed to open registry key <%ws> (rc=%d).",
               REGSTR_PATH_NETTERM, rcReg));
        rcReg = ERROR_SUCCESS;
    }
    else
    {
        DWORD dwType;
        DWORD dwSize;

        dwSize = sizeof(configParams->szRemoteAddr);
        if ((rcReg = RegQueryValueExW(hkey,
                                      REGSTR_VALUE_REMOTEADDR,
                                      NULL,
                                      &dwType,
                                      (LPBYTE)configParams->szRemoteAddr,
                                      &dwSize)) != ERROR_SUCCESS)
        {
            //
            // It's okay if the value doesn't exist.
            //
            TWarn(("Failed to get remote address from registry (rc=%d).",
                   rcReg));
            rcReg = ERROR_SUCCESS;
        }
        else if (dwType != REG_SZ)
        {
            rcReg = ERROR_INVALID_DATA;
            MsgPrintf(g_progName, MSGTYPE_ERR, rcReg,
                      L"Invalid remote IP address.");
        }

        if (rcReg == ERROR_SUCCESS)
        {
            dwSize = sizeof(configParams->szRemotePort);
            if ((rcReg = RegQueryValueExW(hkey,
                                          REGSTR_VALUE_REMOTEPORT,
                                          NULL,
                                          &dwType,
                                          (LPBYTE)configParams->szRemotePort,
                                          &dwSize)) != ERROR_SUCCESS)
            {
                //
                // It's okay if the value doesn't exist.
                //
                TWarn(("Failed to get remote port from registry (rc=%d).",
                       rcReg));
                rcReg = ERROR_SUCCESS;
            }
            else if (dwType != REG_SZ)
            {
                rcReg = ERROR_INVALID_DATA;
                MsgPrintf(g_progName, MSGTYPE_ERR, rcReg,
                          L"Invalid remote port.");
            }
        }

        if (rcReg == ERROR_SUCCESS)
        {
            dwSize = sizeof(configParams->szLocalPort);
            if ((rcReg = RegQueryValueExW(hkey,
                                          REGSTR_VALUE_LOCALPORT,
                                          NULL,
                                          &dwType,
                                          (LPBYTE)configParams->szLocalPort,
                                          &dwSize)) != ERROR_SUCCESS)
            {
                //
                // It's okay if the value doesn't exist.
                //
                TWarn(("Failed to get local port from registry (rc=%d).",
                       rcReg));
                rcReg = ERROR_SUCCESS;
            }
            else if (dwType != REG_SZ)
            {
                rcReg = ERROR_INVALID_DATA;
                MsgPrintf(g_progName, MSGTYPE_ERR, rcReg,
                          L"Invalid local port.");
            }
        }

        RegCloseKey(hkey);
    }

    hr = HRESULT_FROM_WIN32(rcReg);

    TExitMsg(("=%x (remoteAddr=%ws,remotePort=%ws,localPort=%ws)",
              hr, configParams->szRemoteAddr, configParams->szRemotePort,
              configParams->szLocalPort));
    return hr;
}   //RegistryGetConfig

/**
 *  This function saves the configuration to the registry.
 *
 *  @param configParams Points to the CONFIG_PARAMS structure.
 *
 *  @return Success: Returns S_OK.
 *  @return Failure: Returns HRESULT_CODE.
 */
HRESULT
RegistrySaveConfig(
    __in PCONFIG_PARAMS configParams
    )
{
    HRESULT hr;
    LONG rcReg;
    HKEY hkey = NULL;

    TLevel(FUNC);
    TEnterMsg(("remoteAddr=%ws,remotePort=%ws,localPort=%ws",
               configParams->szRemoteAddr, configParams->szRemotePort,
               configParams->szLocalPort));

    rcReg = RegCreateKeyW(HKEY_CURRENT_USER, REGSTR_PATH_NETTERM, &hkey);
    if (rcReg != ERROR_SUCCESS)
    {
        MsgPrintf(g_progName, MSGTYPE_ERR, rcReg,
                  L"Failed to create configuration registry key.");
    }
    else if ((rcReg = RegSetValueExW(
                        hkey,
                        REGSTR_VALUE_REMOTEADDR,
                        0,
                        REG_SZ,
                        (LPBYTE)configParams->szRemoteAddr,
                        (DWORD)((wcslen(configParams->szRemoteAddr) + 1)*
                                sizeof(WCHAR)))) != ERROR_SUCCESS)
    {
        MsgPrintf(g_progName, MSGTYPE_ERR, rcReg,
                  L"Failed to save remote address to the registry.");
    }
    else if ((rcReg = RegSetValueExW(
                        hkey,
                        REGSTR_VALUE_REMOTEPORT,
                        0,
                        REG_SZ,
                        (LPBYTE)configParams->szRemotePort,
                        (DWORD)((wcslen(configParams->szRemotePort) + 1)*
                                sizeof(WCHAR)))) != ERROR_SUCCESS)
    {
        MsgPrintf(g_progName, MSGTYPE_ERR, rcReg,
                  L"Failed to save remote port to the registry.");
    }
    else if ((rcReg = RegSetValueExW(
                        hkey,
                        REGSTR_VALUE_LOCALPORT,
                        0,
                        REG_SZ,
                        (LPBYTE)configParams->szLocalPort,
                        (DWORD)((wcslen(configParams->szLocalPort) + 1)*
                                sizeof(WCHAR)))) != ERROR_SUCCESS)
    {
        MsgPrintf(g_progName, MSGTYPE_ERR, rcReg,
                  L"Failed to save local port to the registry.");
    }

    if (hkey != NULL)
    {
        RegCloseKey(hkey);
    }

    hr = HRESULT_FROM_WIN32(rcReg);

    TExitMsg(("=%x", hr));
    return hr;
}   //RegistrySaveConfig

