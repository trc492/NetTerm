#if 0
/// Copyright (c) Titan Robotics Club. All rights reserved.
///
/// <module name="Console.h" />
///
/// <summary>
///     This module contains the definitions and implementation of the Console
///     class.
/// </summary>
///
/// <remarks>
///     Author: Michael Tsang (18-Apr-2012)
///     Environment: Windows application.
/// </remarks>
#endif

#ifdef MOD_ID
    #undef MOD_ID
#endif
#define MOD_ID                  MOD_CONSOLE

class Console: public WsaCallback
{
private:
    #define FOREGROUND_MASK     0x000f
    #define BACKGROUND_MASK     0x00f0
    #define FOREGROUND_BLACK    0x0000
    #define FOREGROUND_CYAN     (FOREGROUND_GREEN | FOREGROUND_BLUE)
    #define FOREGROUND_MAGENTA  (FOREGROUND_RED | FOREGROUND_BLUE)
    #define FOREGROUND_YELLOW   (FOREGROUND_RED | FOREGROUND_GREEN)
    #define FOREGROUND_WHITE    (FOREGROUND_RED | \
                                 FOREGROUND_GREEN | \
                                 FOREGROUND_BLUE)
    #define BACKGROUND_BLACK    0x0000
    #define BACKGROUND_CYAN     (BACKGROUND_GREEN | BACKGROUND_BLUE)
    #define BACKGROUND_MAGENTA  (BACKGROUND_RED | BACKGROUND_BLUE)
    #define BACKGROUND_YELLOW   (BACKGROUND_RED | BACKGROUND_GREEN)
    #define BACKGROUND_WHITE    (BACKGROUND_RED | \
                                 BACKGROUND_GREEN | \
                                 BACKGROUND_BLUE)

    #define DEF_TEXT_ATTRIB     FOREGROUND_WHITE

    PHANDLER_ROUTINE    m_ctrlHandler;
    WORD                m_currTextAttrib;
    HANDLE              m_hConOut;
    WORD                m_origTextAttrib;
    char                m_szRecvBuff[RECV_BUFF_SIZE];

    /**
     *  This function translates the ANSI SGR code into console text
     *  attributes.
     *
     *  @param code Specifies the ANSI SGR code string to translate.
     *  @param textAttrib Specifies the current text attribute.
     *
     *  @return Returns the new text attributes.
     */
    WORD
    AnsiCodeToTextAttrib(
        __in LPCSTR code,
        __in WORD textAttrib
        )
    {
        static struct _XlatEntry
        {
            LPSTR code;
            WORD  mask;
            WORD  attrib;
        } AnsiTable[] = {
            {SGR_FG_BLACK,   FOREGROUND_MASK, FOREGROUND_BLACK},
            {SGR_FG_RED,     FOREGROUND_MASK, FOREGROUND_RED},
            {SGR_FG_GREEN,   FOREGROUND_MASK, FOREGROUND_GREEN},
            {SGR_FG_YELLOW,  FOREGROUND_MASK, FOREGROUND_YELLOW},
            {SGR_FG_BLUE,    FOREGROUND_MASK, FOREGROUND_BLUE},
            {SGR_FG_MAGENTA, FOREGROUND_MASK, FOREGROUND_MAGENTA},
            {SGR_FG_CYAN,    FOREGROUND_MASK, FOREGROUND_CYAN},
            {SGR_FG_WHITE,   FOREGROUND_MASK, FOREGROUND_WHITE},
            {SGR_BG_BLACK,   BACKGROUND_MASK, BACKGROUND_BLACK},
            {SGR_BG_RED,     BACKGROUND_MASK, BACKGROUND_RED},
            {SGR_BG_GREEN,   BACKGROUND_MASK, BACKGROUND_GREEN},
            {SGR_BG_YELLOW,  BACKGROUND_MASK, BACKGROUND_YELLOW},
            {SGR_BG_BLUE,    BACKGROUND_MASK, BACKGROUND_BLUE},
            {SGR_BG_MAGENTA, BACKGROUND_MASK, BACKGROUND_MAGENTA},
            {SGR_BG_CYAN,    BACKGROUND_MASK, BACKGROUND_CYAN},
            {SGR_BG_WHITE,   BACKGROUND_MASK, BACKGROUND_WHITE},
        };

        TLevel(FUNC);
        TEnterMsg(("code=%s,attrib=%x", code, textAttrib));

        if ((strlen(code) == 0) || (strcmp(code, SGR_RESET) == 0))
        {
            textAttrib = DEF_TEXT_ATTRIB;
        }
        else if (strcmp(code, SGR_BRIGHT) == 0)
        {
            textAttrib |= FOREGROUND_INTENSITY;
        }
        else if (strcmp(code, SGR_REVERSE) == 0)
        {
            textAttrib = ((textAttrib & FOREGROUND_MASK) << 4) |
                         ((textAttrib & BACKGROUND_MASK) >> 4);
        }
        else
        {
            for (int i = 0; i < ARRAYSIZE(AnsiTable); i++)
            {
                if (strcmp(code, AnsiTable[i].code) == 0)
                {
                    textAttrib &= ~AnsiTable[i].mask;
                    textAttrib |= AnsiTable[i].attrib;
                    break;
                }
            }
        }

        TExitMsg(("=%x", textAttrib));
        return textAttrib;
    }   //AnsiCodeToTextAttrib

    /**
     *  This function parses the string for embedded ANSI escape sequence and
     *  process it. The caller should call this in a loop and passing the
     *  pointer to the character past the end of the previous escape sequence
     *  as the start of the string.
     *
     *  @param string Specifies the string to be parsed.
     *  @param startSeq Points to a variable to hold the pointer to the
     *         beginning of the escape sequence found.
     *  @param endSeq Points to a variable to hold the pointer to the end of
     *         the escape seqence found. The caller should call this function
     *         again with one character past this pointer as the next string
     *         to parse.
     *
     *  @return Returns the new text attributes.
     */
    WORD
    ParseAnsiSeq(
        __in  LPSTR  string,
        __out LPSTR *startSeq,
        __out LPSTR *endSeq
        )
    {
        WORD textAttrib = m_currTextAttrib;

        TLevel(FUNC);
        TEnterMsg(("string=%s,pstartSeq=%p,pendSeq=%p",
                   string, startSeq, endSeq));

        *startSeq = strstr(string, ESC_PREFIX);
        if (*startSeq == NULL)
        {
            *endSeq = NULL;
        }
        else
        {
            *endSeq = strstr(&(*startSeq)[2], ESC_SUFFIX);
            if (*endSeq != NULL)
            {
                LPSTR psz;
                LPSTR pszCtxt = NULL;

                **endSeq = '\0';
                if (strlen(&(*startSeq)[2]) == 0)
                {
                    textAttrib = DEF_TEXT_ATTRIB;
                }
                else
                {
                    psz = strtok_s(&(*startSeq)[2], ESC_SEP, &pszCtxt);
                    while (psz != NULL)
                    {
                        textAttrib = AnsiCodeToTextAttrib(psz, textAttrib);
                        psz = strtok_s(NULL, ESC_SEP, &pszCtxt);
                    }
                }
            }
            else
            {
                *startSeq = NULL;
            }
        }

        TExitMsg(("=%x (start=%p,end=%p,attrib=%x)",
                  textAttrib, *startSeq, *endSeq, m_currTextAttrib));
        return textAttrib;
    }   //ParseAnsiSeq

    /**
     *  This function clears the console screen by writing spaces and default
     *  text attributes to screen buffer.
     *
     *  @param fFullScreen If TRUE, clear the whole screen, otherwise clear
     *         from the cursor to the end of the screen.
     */
    VOID
    ClearScreen(
        __in BOOL fFullScreen
        )
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;

        TLevel(API);
        TEnterMsg(("fFullScreen=%d", fFullScreen));

        if (!GetConsoleScreenBufferInfo(m_hConOut, &csbi))
        {
            TErr(("Failed to get console screen buffer info (err=%d).",
                  GetLastError()));
        }
        else
        {
            COORD coordScreen = {0, 0};
            DWORD dwScreenLen;
            DWORD cchWritten;

            if (fFullScreen)
            {
                dwScreenLen = csbi.dwSize.X*csbi.dwSize.Y;
            }
            else
            {
                coordScreen = csbi.dwCursorPosition;
                dwScreenLen = (csbi.dwSize.Y - csbi.dwCursorPosition.Y)*
                              csbi.dwSize.X - csbi.dwCursorPosition.X;
            }

            if (!FillConsoleOutputCharacterA(m_hConOut,
                                             ' ',
                                             dwScreenLen,
                                             coordScreen,
                                             &cchWritten))
            {
                TErr(("Failed to clear screen (x=%d,y=%d,err=%d).",
                      csbi.dwSize.X, csbi.dwSize.Y, GetLastError()));
            }
            else if (!FillConsoleOutputAttribute(m_hConOut,
                                                 csbi.wAttributes,
                                                 dwScreenLen,
                                                 coordScreen,
                                                 &cchWritten))
            {
                TErr(("Failed to set screen attribute (x=%d,y=%d,err=%d).",
                      csbi.dwSize.X, csbi.dwSize.Y, GetLastError()));
            }
            else if (fFullScreen &&
                     !SetConsoleCursorPosition(m_hConOut, coordScreen))
            {
                TErr(("Failed to set cursor position (err=%d).",
                      GetLastError()));
            }
        }

        TExit();
        return;
    }   //ClearScreen

    /**
     *  This function dumps the data buffer as binary data in hex and char
     *  format.
     *
     *  @param buffer Points to the buffer that contains the data.
     *  @param len Specifies the length of the buffer.
     */
    VOID
    DumpBin(
        __in_bcount(len) LPBYTE buffer,
        __in             DWORD len
        )
    {
        TLevel(FUNC);
        TEnterMsg(("buffer=%p,len=%d", buffer, len));

        for (DWORD i = 0; i < len; i++)
        {
            if ((i % 16) == 0)
            {
                printf("%08x:", i);
            }

            printf(" %02x", buffer[i]);

            if (((i + 1) == len) && ((i + 1)%16 != 0))
            {
                for (DWORD j = 0; j < (16 - ((i + 1)%16)); j++)
                {
                    printf("   ");
                }
            }

            if ((((i + 1)%16) == 0) || ((i + 1) == len))
            {
                ULONG j;

                printf("  ");
                for (j = (i/16)*16; j <= i; j++)
                {
                    printf("%c",
                            (__isascii(buffer[j]) &&
                            !iscntrl(buffer[j]))?
                            buffer[j]: '.');
                }
                printf("\n");
            }
        }
        printf("\n");

        TExit();
        return;
    }   //DumpBin

public:
    /**
     *  Constructor for the NetConn class.
     *
     *  @param ctrlHandler Specifies the console control handler.
     */
    Console(
        __in_opt PHANDLER_ROUTINE ctrlHandler = NULL
        ): m_ctrlHandler(ctrlHandler)
         , m_currTextAttrib(DEF_TEXT_ATTRIB)
         , m_origTextAttrib(0)
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;

        TLevel(INIT);
        TEnterMsg(("ctrlHandler=%p", ctrlHandler));

        m_hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if ((m_hConOut == INVALID_HANDLE_VALUE) || (m_hConOut == NULL))
        {
            MsgPrintf(g_progName, MSGTYPE_ERR, GetLastError(),
                      L"Failed to get stdout handle.");
        }
        else if ((ctrlHandler != NULL) &&
                 !SetConsoleCtrlHandler(ctrlHandler, TRUE))
        {
            MsgPrintf(g_progName, MSGTYPE_ERR, GetLastError(),
                      L"Failed to set console control handler.");
            m_ctrlHandler = NULL;
        }
        else if (!GetConsoleScreenBufferInfo(m_hConOut, &csbi))
        {
            MsgPrintf(g_progName, MSGTYPE_ERR, GetLastError(),
                      L"Failed to get console buffer info.");
        }
        else
        {
            m_szRecvBuff[0] = '\0';
            m_origTextAttrib = csbi.wAttributes;
            SetConsoleTextAttribute(m_hConOut, m_currTextAttrib);
            ClearScreen(TRUE);
        }

        TExit();
        return;
    }   //Console

    /**
     *  Destructor for the NetConn class.
     */
    ~Console(
        VOID
        )
    {
        TLevel(INIT);
        TEnter();

        if (m_origTextAttrib != 0)
        {
            SetConsoleTextAttribute(m_hConOut, m_origTextAttrib);
            ClearScreen(FALSE);
        }

        if (m_ctrlHandler != NULL)
        {
            SetConsoleCtrlHandler(m_ctrlHandler, FALSE);
        }

        TExit();
        return;
    }   //~Console

    /**
     *  This is a callback from the server when a buffer of data is received
     *  so that the buffer can be processed.
     *
     *  @param connHandle Specifies the handle of the connection receiving
     *         the data.
     *  @param context Specifies the callback context.
     *  @param recvBuff Points to the buffer containing the received data.
     *  @param recvLen Specifies the length of the received data.
     */
    VOID
    DataReceived(
        __in                 HANDLE connHandle,
        __in_opt             LPVOID context,
        __in_bcount(recvLen) LPBYTE recvBuff,
        __in                 DWORD recvLen
        )
    {
        TLevel(CALLBK);
        TEnterMsg(("hConn=%p,ctxt=%p,buff=%p,len=%d",
                   connHandle, context, recvBuff, recvLen));

        UNREFERENCED_PARAMETER(connHandle);
        UNREFERENCED_PARAMETER(context);

        if (g_hLogFile != NULL)
        {
            fwrite(recvBuff, recvLen, 1, g_hLogFile);
        }

        if (g_progFlags & NETTERMF_DUMPBIN)
        {
            DumpBin(recvBuff, recvLen);
        }
        else if (recvLen + 1 > sizeof(m_szRecvBuff))
        {
            TErr(("Receive length exceeds buffer size (len=%d).", recvLen));
        }
        else
        {
            WORD textAttrib;
            LPSTR pszLine;
            LPSTR pszStart;
            LPSTR pszEnd;

            pszLine = m_szRecvBuff;
            RtlCopyMemory(pszLine, recvBuff, recvLen);
            pszLine[recvLen] = '\0';
            for (;;)
            {
                textAttrib = ParseAnsiSeq(pszLine, &pszStart, &pszEnd);
                if (pszStart != NULL)
                {
                    *pszStart = '\0';
                    printf(pszLine);
                    pszLine = pszEnd + 1;
                    SetConsoleTextAttribute(m_hConOut, textAttrib);
                    m_currTextAttrib = textAttrib;
                }
                else
                {
                    printf(pszLine);
                    if ((g_progFlags & NETTERMF_APPENDLF) &&
                        (pszLine[strlen(pszLine) - 1] == '\r'))
                    {
                        printf("\n");
                    }
                    break;
                }
            }
        }

        TExit();
        return;
    }   //DataReceived

};  //class Console
