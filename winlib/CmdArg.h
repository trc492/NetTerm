#if 0
/// Copyright (c) Titan Robotics Club. All rights reserved.
///
/// <module name="CmdArg.h" />
///
/// <summary>
///     This module contains the definitions and implementation of the
///     CmdArg class.
/// </summary>
///
/// <remarks>
///     Author: Michael Tsang (25-May-2012)
///     Environment: Windows application.
/// </remarks>
#endif

#pragma once

#ifdef MOD_ID
    #undef MOD_ID
#endif
#define MOD_ID                  MOD_CMDARG

#define ARGTYPE_NONE            0
#define ARGTYPE_STRING          1
#define ARGTYPE_NUMERIC         2
#define ARGTYPE_SWITCH          3
#define ARGTYPE_FUNCTION        4

//
//  context:
//      ARGTYPE_STRING - not used
//      ARGTYPE_NUMERIC - number base
//      ARGTYPE_SWITCH - bit mask
//      ARGTYPE_FUNCTION - callback context
//
typedef struct _ArgEntry
{
    LPWSTR      name;
    DWORD       type;
    LPVOID      data;
    ULONG_PTR   context;
    LPWSTR      syntax;
    LPWSTR      help;
} ARG_ENTRY, *PARG_ENTRY;

typedef HRESULT (*PFNARG)(__in PARG_ENTRY argEntry);

class CmdArg
{
private:
    //
    // Private data.
    //
    PARG_ENTRY  m_argsTable;

    /**
     *  This function parses the argument according to the argument table
     *  entry.
     *
     *  @param arg Specifies the argument string.
     *  @param argEntry Points to the argument table entry.
     *
     *  @return Success: Returns S_OK.
     *  @return Failure: Returns HRESULT code.
     */
    HRESULT
    ParseArg(
        __in LPWSTR arg,
        __in PARG_ENTRY argEntry
        )
    {
        HRESULT hr = S_OK;

        TLevel(FUNC);
        TEnterMsg(("arg=%ws,argEntry=%p", arg, argEntry));

        switch (argEntry->type)
        {
            case ARGTYPE_STRING:
                *(LPWSTR *)argEntry->data = arg;
                break;

            case ARGTYPE_NUMERIC:
            {
                LPWSTR psz;

                *((LPDWORD)argEntry->data) = wcstoul(arg,
                                                     &psz,
                                                     (int)argEntry->context);
                if ((psz == arg) || (*psz != L'\0'))
                {
                    hr = E_INVALIDARG;
                    MsgPrintf(g_progName, MSGTYPE_ERR, hr,
                              L"Invalid number argument %s", arg);
                }
                break;
            }

            case ARGTYPE_SWITCH:
                *((LPWORD)argEntry->data) |= (DWORD)argEntry->context;
                break;

            case ARGTYPE_FUNCTION:
                hr = ((PFNARG)argEntry->data)(argEntry);
                break;
        }

        TExitMsg(("=%x", hr));
        return hr;
    }   //ParseArg

    /**
     *  This function parses a named argument option.
     *
     *  @param arg Specifies the argument name string.
     *
     *  @return Success: Returns S_OK.
     *  @return Failure: Returns HRESULT code.
     */
    HRESULT
    ParseNamedArg(
        __in LPWSTR arg
        )
    {
        HRESULT hr;

        TLevel(FUNC);
        TEnterMsg(("arg=%ws", arg));

        if (m_argsTable == NULL)
        {
            hr = HRESULT_FROM_WIN32(ERROR_APP_INIT_FAILURE);
            MsgPrintf(g_progName, MSGTYPE_ERR, hr, L"No arguments table.");
        }
        else
        {
            size_t argLen = wcslen(arg);
            size_t nameLen;

            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
            for (int i = 0; m_argsTable[i].type != ARGTYPE_NONE; i++)
            {
                nameLen = wcslen(m_argsTable[i].name);
                if (_wcsnicmp(arg, m_argsTable[i].name, nameLen) == 0)
                {
                    switch (m_argsTable[i].type)
                    {
                        case ARGTYPE_STRING:
                        case ARGTYPE_NUMERIC:
                            if ((argLen > nameLen) && (arg[nameLen] == L'='))
                            {
                                hr = ParseArg(&arg[nameLen + 1],
                                              &m_argsTable[i]);
                                break;
                            }
                            else
                            {
                                continue;
                            }

                        case ARGTYPE_SWITCH:
                        case ARGTYPE_FUNCTION:
                            if (argLen == nameLen)
                            {
                                hr = ParseArg(arg, &m_argsTable[i]);
                                break;
                            }
                            else
                            {
                                continue;
                            }
                    }
                }
            }

            if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
            {
                hr = E_INVALIDARG;
                MsgPrintf(g_progName, MSGTYPE_ERR, hr,
                          L"Invalid option <%s>.", arg);
            }
        }

        TExitMsg(("=%x", hr));
        return hr;
    }   //ParseNamedArg

    /**
     *  This function prints the syntax help message of the argument entry.
     *
     *  @param argEntry Points to the argument entry.
     *  @param buffer Points to the caller allocated buffer for the syntax
     *         string.
     *  @param buffLen Specifies the length of the syntax buffer.
     */
    VOID
    PrintSyntaxHelp(
        __in                  PARG_ENTRY argEntry,
        __out_ecount(buffLen) WCHAR *buffer,
        __in                  DWORD buffLen
        )
    {
        DWORD len;

        TLevel(FUNC);
        TEnterMsg(("argEntry=%p,buffer=%p,len=%d", argEntry, buffer, buffLen));

        StringCchPrintfW(buffer, buffLen,
                         L"%s%s",
                         argEntry->name? argEntry->name: L"",
                         argEntry->syntax? argEntry->syntax: L"");
        len = (DWORD)wcslen(buffer);
        for (DWORD i = len; i < buffLen - 1; i++)
        {
            buffer[i] = L' ';
        }
        buffer[buffLen - 1] = L'\0';
        wprintf(buffer);

        if (argEntry->help != NULL)
        {
            printf(" - %ws", argEntry->help);
        }
        printf("\n");

        TExit();
        return;
    }   //PrintSyntaxHelp

public:
    /**
     *  Constructor of the class object.
     *
     *  @param argsTable Points to the arguments table.
     */
    CmdArg(
        __in PARG_ENTRY argsTable
        ): m_argsTable(argsTable)
    {
        TLevel(INIT);
        TEnterMsg(("argsTable=%p", argsTable));
        TExit();
        return;
    }   //CmdArg

    /**
     *  Desctructor of the class object.
     */
    ~CmdArg(
        VOID
        )
    {
        TLevel(INIT);
        TEnter();
        TExit();
        return;
    }   //~CmdArg

    /**
     *  This function parses the program name.
     *
     *  @param progArg Points to the program name argument string.
     *  @param defPorgName Points to a default program name string if failed
     *         to parse the program name argument.
     *
     *  @return Success: Returns a pointer to the program name string.
     *  @return Failure: Returns a pointer to the default program name string.
     */
    LPWSTR
    ParseProgramName(
        __in LPWSTR progArg,
        __in LPWSTR defProgName
        )
    {
        LPWSTR progName;

        TLevel(API);
        TEnterMsg(("progArg=%ws,defProgName=%ws", progArg, defProgName));

        progName = (LPWSTR)wcsrchr(progArg, L'\\');
        if (progName == NULL)
        {
            progName = defProgName;
        }
        else
        {
            LPWSTR psz;

            progName++;
            psz = wcschr((wchar_t *)progName, L'.');
            if (psz != NULL)
            {
                *psz = L'\0';
            }
        }

        TExitMsg(("=%ws", progName));
        return progName;
    }   //ParseProgramName

    /**
     *  This function parses the arguments.
     *
     *  @param icArgs Specifies the number of arguments.
     *  @param apszArgs Points to the argument pointers array.
     *  @param fMatchArgCount If TRUE, check for zero icArgs at the end.
     *
     *  @return Success: Returns S_OK.
     *  @return Failure: Returns HRESULT code.
     */
    HRESULT
    ParseArguments(
        __in                int icArgs,
        __in_ecount(icArgs) LPWSTR *apszArgs,
        __in                BOOL fMatchArgCount
        )
    {
        HRESULT hr = S_OK;

        TLevel(API);
        TEnterMsg(("icArgs=%d,apszArgs=%p,fMatchArgCount=%d",
                   icArgs, apszArgs, fMatchArgCount));

        if (m_argsTable == NULL)
        {
            hr = HRESULT_FROM_WIN32(ERROR_APP_INIT_FAILURE);
            MsgPrintf(g_progName, MSGTYPE_ERR, hr, L"No arguments table.");
        }
        else
        {
            //
            // Parse options if any.
            //
            while (SUCCEEDED(hr) && (icArgs > 0))
            {
                if ((apszArgs[0][0] == L'/') || (apszArgs[0][0] == L'-'))
                {
                    hr = ParseNamedArg(&apszArgs[0][1]);
                    icArgs--;
                    apszArgs++;
                }
                else
                {
                    break;
                }
            }
            //
            // Parse arguments if any.
            //
            if (SUCCEEDED(hr))
            {
                int i = 0;
                //
                // Skip all options in the table.
                //
                while (m_argsTable[i].name != NULL)
                {
                    i++;
                }

                while (SUCCEEDED(hr) && (icArgs > 0) &&
                       (m_argsTable[i].type != ARGTYPE_NONE))
                {
                    hr = ParseArg(apszArgs[0], &m_argsTable[i]);
                    icArgs--;
                    apszArgs++;
                    i++;
                }

                if (SUCCEEDED(hr))
                {
                    if (m_argsTable[i].type != ARGTYPE_NONE)
                    {
                        hr = E_INVALIDARG;
                        MsgPrintf(g_progName, MSGTYPE_ERR, hr,
                                  L"Missing arguments.");
                    }
                    else if (fMatchArgCount && (icArgs > 0))
                    {
                        hr = E_INVALIDARG;
                        MsgPrintf(g_progName, MSGTYPE_ERR, hr,
                                  L"Too many arguments.");
                    }
                }
            }
        }

        TExitMsg(("=%x", hr));
        return hr;
    }   //ParseArguments

    /**
     *  This function prints the command usage help message.
     *
     *  @param cmdName Specifies the command name string.
     *  @param fVerbose If TRUE, prints the verbose help message.
     */
    VOID
    PrintCmdHelp(
        __in LPCWSTR cmdName,
        __in BOOL fVerbose
        )
    {
        DWORD maxLen = 0;

        TLevel(API);
        TEnterMsg(("cmdName=%ws,fVerbose=%d", cmdName, fVerbose));

        printf("%ws", cmdName);
        if (m_argsTable != NULL)
        {
            DWORD len;

            for (int i = 0; m_argsTable[i].type != ARGTYPE_NONE; i++)
            {
                if (m_argsTable[i].name != NULL)
                {
                    printf(" [/%ws%ws]",
                           m_argsTable[i].name,
                           m_argsTable[i].syntax? m_argsTable[i].syntax: L"");
                }
                else
                {
                    printf(" %ws",
                           m_argsTable[i].syntax? m_argsTable[i].syntax: L"");
                }

                len = (DWORD)
                      ((m_argsTable[i].name? wcslen(m_argsTable[i].name): 0) +
                       (m_argsTable[i].syntax? wcslen(m_argsTable[i].syntax):
                                               0));
                if (len > maxLen)
                {
                    maxLen = len;
                }
            }
        }
        printf("\n");

        if (fVerbose && (m_argsTable != NULL))
        {
            LPWSTR pszSyntax = (LPWSTR)new WCHAR[maxLen + 1];

            if (pszSyntax != NULL)
            {
                printf("\n");
                for (int i = 0; m_argsTable[i].type != ARGTYPE_NONE; i++)
                {
                    PrintSyntaxHelp(&m_argsTable[i], pszSyntax, maxLen + 1);
                }
                delete[] pszSyntax;
            }
            else
            {
                MsgPrintf(g_progName, MSGTYPE_ERR, 0,
                          L"Failed to allocate syntax string (len=%d).",
                          maxLen + 1);
            }
        }

        TExit();
        return;
    }   //PrintCmdHelp

};  //class CmdArg

