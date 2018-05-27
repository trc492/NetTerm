#if 0
/// Copyright (c) Titan Robotics Club. All rights reserved.
///
/// <module name="Util.cpp" />
///
/// <summary>
///     This module contains utility functions.
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
#define MOD_ID                  MOD_UTIL

/**
 *  This function displays a message on the screen.
 *
 *  @param pszTitle Specifies the title of the message box.
 *  @param msgType Specifies the message type in order to display an
 *         appropriate icon. Valid message types could be:
 *          MSGTYPE_INFO
 *          MSGTYPE_WARN
 *          MSGTYPE_ERR
 *  @param errCode Specifies the HRESULT or Win32 code if provided,
 *         otherwise it's 0.
 *  @param pszFormat Specifies the format string.
 *  @param ... Specifies the substitution arguments for the format string.
 *
 *  @return Success: Returns the MessageBox return code.
 *  @return Failure: Returns HRESULT code.
 */
HRESULT
MsgPrintf(
    __in_opt LPCWSTR pszTitle,
    __in     UINT msgType,
    __in     DWORD errCode,
    __in     LPCWSTR pszFormat,
    ...
    )
{
    HRESULT hr = S_OK;
    static WCHAR szMsg[1024];
    va_list argList;

    TLevel(UTIL);
    TEnterMsg(("title=%ws,msgType=%x,err=%x,format=%ws",
               pszTitle? pszTitle: L"<null>", msgType, errCode, pszFormat));

    va_start(argList, pszFormat);
    hr = StringCchVPrintfW(szMsg, ARRAYSIZE(szMsg), pszFormat, argList);
    va_end(argList);
    if (FAILED(hr))
    {
        TErr(("Failed to format the message (hr=%x).", hr));
    }
    else if (errCode != ERROR_SUCCESS)
    {
        size_t len = 0;

        if ((hr = StringCchLengthW(szMsg, ARRAYSIZE(szMsg), &len)) != S_OK)
        {
            TErr(("Failed to determine message length (hr=%x).", hr));
        }
        else if ((hr = StringCchPrintfW(&szMsg[len], ARRAYSIZE(szMsg) - len,
                                        L"\n\nError Code: %d (0x%08x)\n",
                                        HRESULT_CODE(errCode), errCode)) !=
                 S_OK)
        {
            TErr(("Failed to print error code (hr=%x).", hr));
        }
        else if ((hr = StringCchLengthW(szMsg, ARRAYSIZE(szMsg), &len)) !=
                 S_OK)
        {
            TErr(("Failed to determine message length (hr=%x).", hr));
        }
        else if (!FormatMessageW((FORMAT_MESSAGE_FROM_SYSTEM |
                                  FORMAT_MESSAGE_IGNORE_INSERTS),
                                 NULL,
                                 errCode,
                                 0,
                                 &szMsg[len],
                                 ARRAYSIZE(szMsg) - len,
                                 NULL))
        {
            hr = GETLASTHRESULT();
            TErr(("Failed to format message (hr=%x).", hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        switch (msgType)
        {
        case MSGTYPE_INFO:
            msgType = MB_ICONINFORMATION;
            break;

        case MSGTYPE_WARN:
            msgType = MB_ICONWARNING;
            break;

        case MSGTYPE_ERR:
            msgType = MB_ICONERROR;
            break;

        default:
            msgType = MB_ICONQUESTION;
            break;
        }

        hr = (HRESULT)MessageBoxW(NULL, szMsg, pszTitle, msgType);
    }

    TExitMsg(("=%x", hr));
    return hr;
}   //MsgPrintf

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

    rcReg = RegOpenKeyW(HKEY_CURRENT_USER, REGSTR_PATH_WINNETTERM, &hkey);
    if (rcReg != ERROR_SUCCESS)
    {
        //
        // It's okay to fail. It could just mean we did not save the config
        // in the registry.
        //
        TWarn(("Failed to open registry key <%ws> (rc=%d).",
               REGSTR_PATH_WINNETTERM, rcReg));
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
            TErr(("Invalid remote address."));
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
                TErr(("Invalid remote port."));
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
                TErr(("Invalid remote port."));
            }
        }

        if (rcReg == ERROR_SUCCESS)
        {
            dwSize = sizeof(configParams->xPos);
            if ((rcReg = RegQueryValueExW(hkey,
                                          REGSTR_VALUE_XPOS,
                                          NULL,
                                          &dwType,
                                          (LPBYTE)&configParams->xPos,
                                          &dwSize)) != ERROR_SUCCESS)
            {
                //
                // It's okay if the value doesn't exist.
                //
                TWarn(("Failed to get X window position from registry (rc=%d).",
                       rcReg));
                rcReg = ERROR_SUCCESS;
            }
            else if (dwType != REG_DWORD)
            {
                rcReg = ERROR_INVALID_DATA;
                TErr(("Invalid X window position."));
            }
        }

        if (rcReg == ERROR_SUCCESS)
        {
            dwSize = sizeof(configParams->yPos);
            if ((rcReg = RegQueryValueExW(hkey,
                                          REGSTR_VALUE_YPOS,
                                          NULL,
                                          &dwType,
                                          (LPBYTE)&configParams->yPos,
                                          &dwSize)) != ERROR_SUCCESS)
            {
                //
                // It's okay if the value doesn't exist.
                //
                TWarn(("Failed to get Y window position from registry (rc=%d).",
                       rcReg));
                rcReg = ERROR_SUCCESS;
            }
            else if (dwType != REG_DWORD)
            {
                rcReg = ERROR_INVALID_DATA;
                TErr(("Invalid Y window position."));
            }
        }

        if (rcReg == ERROR_SUCCESS)
        {
            dwSize = sizeof(configParams->nWidth);
            if ((rcReg = RegQueryValueExW(hkey,
                                          REGSTR_VALUE_WIDTH,
                                          NULL,
                                          &dwType,
                                          (LPBYTE)&configParams->nWidth,
                                          &dwSize)) != ERROR_SUCCESS)
            {
                //
                // It's okay if the value doesn't exist.
                //
                TWarn(("Failed to get window width from registry (rc=%d).",
                       rcReg));
                rcReg = ERROR_SUCCESS;
            }
            else if (dwType != REG_DWORD)
            {
                rcReg = ERROR_INVALID_DATA;
                TErr(("Invalid window width."));
            }
        }

        if (rcReg == ERROR_SUCCESS)
        {
            dwSize = sizeof(configParams->nHeight);
            if ((rcReg = RegQueryValueExW(hkey,
                                          REGSTR_VALUE_HEIGHT,
                                          NULL,
                                          &dwType,
                                          (LPBYTE)&configParams->nHeight,
                                          &dwSize)) != ERROR_SUCCESS)
            {
                //
                // It's okay if the value doesn't exist.
                //
                TWarn(("Failed to get window height from registry (rc=%d).",
                       rcReg));
                rcReg = ERROR_SUCCESS;
            }
            else if (dwType != REG_DWORD)
            {
                rcReg = ERROR_INVALID_DATA;
                TErr(("Invalid window height."));
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

    rcReg = RegCreateKeyW(HKEY_CURRENT_USER, REGSTR_PATH_WINNETTERM, &hkey);
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
                        (wcslen(configParams->szRemoteAddr) + 1)*
                        sizeof(WCHAR))) != ERROR_SUCCESS)
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
                        (wcslen(configParams->szRemotePort) + 1)*
                        sizeof(WCHAR))) != ERROR_SUCCESS)
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
                        (wcslen(configParams->szLocalPort) + 1)*
                        sizeof(WCHAR))) != ERROR_SUCCESS)
    {
        MsgPrintf(g_progName, MSGTYPE_ERR, rcReg,
                  L"Failed to save local port to the registry.");
    }
    else if ((rcReg = RegSetValueExW(
                        hkey,
                        REGSTR_VALUE_XPOS,
                        0,
                        REG_DWORD,
                        (LPBYTE)&configParams->xPos,
                        sizeof(configParams->xPos))) != ERROR_SUCCESS)
    {
        MsgPrintf(g_progName, MSGTYPE_ERR, rcReg,
                  L"Failed to save X window position to the registry.");
    }
    else if ((rcReg = RegSetValueExW(
                        hkey,
                        REGSTR_VALUE_YPOS,
                        0,
                        REG_DWORD,
                        (LPBYTE)&configParams->yPos,
                        sizeof(configParams->yPos))) != ERROR_SUCCESS)
    {
        MsgPrintf(g_progName, MSGTYPE_ERR, rcReg,
                    L"Failed to save Y window position to the registry.");
    }
    else if ((rcReg = RegSetValueExW(
                        hkey,
                        REGSTR_VALUE_WIDTH,
                        0,
                        REG_DWORD,
                        (LPBYTE)&configParams->nWidth,
                        sizeof(configParams->nWidth))) != ERROR_SUCCESS)
    {
        MsgPrintf(g_progName, MSGTYPE_ERR, rcReg,
                    L"Failed to save window width to the registry.");
    }
    else if ((rcReg = RegSetValueExW(
                        hkey,
                        REGSTR_VALUE_HEIGHT,
                        0,
                        REG_DWORD,
                        (LPBYTE)&configParams->nHeight,
                        sizeof(configParams->nHeight))) != ERROR_SUCCESS)
    {
        MsgPrintf(g_progName, MSGTYPE_ERR, rcReg,
                    L"Failed to save window height to the registry.");
    }

    if (hkey != NULL)
    {
        RegCloseKey(hkey);
    }

    hr = HRESULT_FROM_WIN32(rcReg);

    TExitMsg(("=%x", hr));
    return hr;
}   //RegistrySaveConfig

/**
 *  This function parses the command line parameters.
 *
 *  @param configParams Points to the config parameters to be filled out.
 *
 *  @return Success: Returns S_OK.
 *  @return Failure: Returns HRESULT code.
 */
HRESULT
ParseCmdParams(
    __inout PCONFIG_PARAMS configParams
    )
{
    HRESULT hr = S_OK;
    LPWSTR *apszArgs;
    int icArgs;
    BOOL fParamsChanged = FALSE;

    TLevel(FUNC);
    TEnterMsg(("configParams=%p", configParams));

    apszArgs = CommandLineToArgvW(GetCommandLineW(), &icArgs);
    //
    // Skip the program name.
    //
    apszArgs++;
    icArgs--;

    while (SUCCEEDED(hr) && (icArgs > 0))
    {
        if ((apszArgs[0][0] == L'/') || (apszArgs[0][0] == L'-'))
        {
            if (_wcsicmp(&apszArgs[0][1], L"l") == 0)
            {
                if (icArgs < 2)
                {
                    hr = E_INVALIDARG;
                }
                else if ((hr = StringCchCopyW(
                                configParams->szLocalPort,
                                ARRAYSIZE(configParams->szLocalPort),
                                apszArgs[1])) == S_OK)
                {
                    icArgs--;
                    apszArgs++;
                    fParamsChanged = TRUE;
                }
            }
            else if (_wcsicmp(&apszArgs[0][1], L"r") == 0)
            {
                LPWSTR psz;

                if ((icArgs < 2) ||
                    ((psz = wcschr(apszArgs[1], L':')) == NULL))
                {
                    hr = E_INVALIDARG;
                }
                else
                {
                    *psz = L'\0';
                    if (((hr = StringCchCopyW(
                                configParams->szRemoteAddr,
                                ARRAYSIZE(configParams->szRemoteAddr),
                                apszArgs[1])) == S_OK) &&
                        ((hr = StringCchCopyW(
                                configParams->szRemotePort,
                                ARRAYSIZE(configParams->szRemotePort),
                                psz + 1)) == S_OK))
                    {
                        icArgs--;
                        apszArgs++;
                        fParamsChanged = TRUE;
                    }
                }
            }
            else if (_wcsicmp(&apszArgs[0][1], L"t") == 0)
            {
                DWORD teamNumber = 0;
                LPWSTR psz = NULL;

                if ((icArgs < 2) ||
                    ((teamNumber = wcstoul(apszArgs[1], &psz, 10)) == 0) ||
                    (*psz != '\0'))
                {
                    hr = E_INVALIDARG;
                }
                else
                {
                    hr = StringCchPrintfW(configParams->szRemoteAddr,
                                          ARRAYSIZE(configParams->szRemoteAddr),
                                          L"10.%d.%d.2",
                                          teamNumber/100,
                                          teamNumber%100);
                    if (SUCCEEDED(hr))
                    {
                        icArgs--;
                        apszArgs++;
                        fParamsChanged = TRUE;
                    }
                }
            }
            else
            {
                hr = E_INVALIDARG;
            }
            icArgs--;
            apszArgs++;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    if (icArgs != 0)
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr) && fParamsChanged)
    {
        RegistrySaveConfig(configParams);
    }

    TExitMsg(("=%x (remoteAddr=%ws,remotePort=%ws,localPort=%ws)",
              hr, configParams->szRemoteAddr, configParams->szRemotePort,
              configParams->szLocalPort));
    return hr;
}   //ParseCmdParams

