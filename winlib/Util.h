#if 0
/// Copyright (c) Titan Robotics Club. All rights reserved.
///
/// <module name="Util.h" />
///
/// <summary>
///     This module contains the definitions and implementation of utility
///     functions.
/// </summary>
///
/// <remarks>
///     Author: Michael Tsang (25-Apr-2012)
///     Environment: Windows application.
/// </remarks>
#endif

#pragma once

#ifdef MOD_ID
    #undef MOD_ID
#endif
#define MOD_ID                  MOD_CMDARG

//
// Constants.
//
#define MSGTYPE_INFO            0
#define MSGTYPE_WARN            1
#define MSGTYPE_ERR             2

//
// Macros.
//
#define GETLASTHRESULT()        HRESULT_FROM_WIN32(GetLastError())
#define SAFE_DELETE(p)          if ((p) != NULL)            \
                                {                           \
                                    delete(p);              \
                                    (p) = NULL;             \
                                }
#define PrintTitle()            printf("\n%ws. %ws [%s, %s]\n%ws\n\n",      \
                                       PROG_TITLE, PROG_VERSION, __DATE__,  \
                                       __TIME__, PROG_COPYRIGHT)

#ifdef _MAIN_FILE

/**
 *  This function displays a message on the screen.
 *
 *  @param pszTitle Specifies the title of the message box.
 *  @param msgType Specifies the message type. Valid types are:
 *          MSGTYPE_INFO
 *          MSGTYPE_WARN
 *          MSGTYPE_ERR
 *  @param errCode Specifies the HRESULT or Win32 code if provided,
 *         otherwise it's 0.
 *  @param pszFormat Specifies the format string.
 *  @param ... Specifies the substitution arguments for the format string.
 */
VOID
MsgPrintf(
    __in_opt LPCWSTR pszTitle,
    __in     UINT msgType,
    __in     DWORD errCode,
    __in     LPCWSTR pszFormat,
    ...
    )
{
    HRESULT hr = S_OK;
    static WCHAR szMsg[256];
    va_list argList;

    if (pszTitle != NULL)
    {
        printf("%ws: ", pszTitle);
    }

    switch (msgType)
    {
    case MSGTYPE_INFO:
        printf("Info: ");
        break;

    case MSGTYPE_WARN:
        printf("Warn: ");
        break;

    case MSGTYPE_ERR:
        printf("Err: ");
        break;

    default:
        printf("Unknown: ");
        break;
    }

    va_start(argList, pszFormat);
    hr = StringCchVPrintfW(szMsg, ARRAYSIZE(szMsg), pszFormat, argList);
    va_end(argList);
    if (FAILED(hr))
    {
        printf("\n");
    }
    else
    {
        printf("%ws\n", szMsg);
    }

    if (errCode != ERROR_SUCCESS)
    {
        if ((hr = StringCchPrintfW(szMsg, ARRAYSIZE(szMsg),
                                   L"\nError Code: %d (0x%08x)\n",
                                   HRESULT_CODE(errCode), errCode)) ==
            S_OK)
        {
            printf("%ws", szMsg);
        }

        if (FormatMessageW((FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_IGNORE_INSERTS),
                           NULL,
                           errCode,
                           0,
                           szMsg,
                           ARRAYSIZE(szMsg),
                           NULL))
        {
            printf("%ws\n", szMsg);
        }
    }

    return;
}   //MsgPrintf

#else

VOID
MsgPrintf(
    __in_opt LPCWSTR pszTitle,
    __in     UINT msgType,
    __in     DWORD errCode,
    __in     LPCWSTR pszFormat,
    ...
    );

#endif

