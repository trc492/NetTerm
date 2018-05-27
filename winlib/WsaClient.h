#if 0
/// Copyright (c) Titan Robotics Club. All rights reserved.
///
/// <module name="WsaClient.h" />
///
/// <summary>
///     This module contains the definitions and implementation of the
///     WsaClient class.
/// </summary>
///
/// <remarks>
///     Author: Michael Tsang (22-Apr-2012)
///     Environment: Windows application.
/// </remarks>
#endif

#pragma once

#ifdef MOD_ID
    #undef MOD_ID
#endif
#define MOD_ID                  MOD_CLIENT

class WsaClient
{
private:
    //
    // Private data.
    //
    BOOL        m_fInitialized;
    int         m_family;
    int         m_sockType;
    int         m_protocol;
    SOCKET      m_socket;
    WSADATA     m_wsaData;
    WCHAR       m_szHost[NI_MAXHOST];
    WCHAR       m_szPort[NI_MAXSERV];
    WCHAR       m_szAddrName[NI_MAXHOST];
    WCHAR       m_szPortName[NI_MAXSERV];

    /**
     *  This function initializes a Winsock client connection.
     *
     *  @return Success: Returns S_OK.
     *  @return Failure: Returns HRESULT code.
     */
    HRESULT
    InitConnection(
        VOID
        )
    {
        HRESULT hr = S_OK;

        TLevel(FUNC);
        TEnter();

        if (m_socket == INVALID_SOCKET)
        {
            DWORD dwErr;
            ADDRINFOW hints;
            ADDRINFOW *ai;

            ZeroMemory(&hints, sizeof(hints));
            hints.ai_family = m_family;
            hints.ai_socktype = m_sockType;
            hints.ai_protocol = m_protocol;
            dwErr = GetAddrInfoW(m_szHost, m_szPort, &hints, &ai);
            if (dwErr != NO_ERROR)
            {
                TWarn(("Failed to get address info (err=%d).", dwErr));
            }
            else
            {
                SOCKADDR_STORAGE addr;
                int len = sizeof(addr);

                TInfo(("Flags=%x,Family=%x,SockType=%x,Protocol=%x,"
                       "CanonName=%ws,AddrFamily=%x",
                       ai->ai_flags, ai->ai_family, ai->ai_socktype,
                       ai->ai_protocol,
                       ai->ai_canonname? ai->ai_canonname: L"<null>",
                       ai->ai_addr->sa_family));
                TAssert((ai->ai_family == AF_INET) ||
                        (ai->ai_family == AF_INET6));

                m_socket = WSASocket(ai->ai_family,
                                     ai->ai_socktype,
                                     ai->ai_protocol,
                                     NULL, 0, WSA_FLAG_OVERLAPPED);
                if (m_socket == INVALID_SOCKET)
                {
                    dwErr = WSAGetLastError();
                    TErr(("Failed to create socket (err=%d).", dwErr));
                }
                else
                {
                    while (connect(m_socket, ai->ai_addr, (int)ai->ai_addrlen)
                           == SOCKET_ERROR)
                    {
                        dwErr = WSAGetLastError();
                        if (dwErr == WSAECONNREFUSED)
                        {
                            dwErr = NO_ERROR;
                            Sleep(1000);
                        }
                        else
                        {
                            TErr(("Failed to connect socket (err=%d).",
                                  dwErr));
                            break;
                        }
                    }
                }

                if (SUCCEEDED(hr))
                {
                    if (getpeername(m_socket, (SOCKADDR *)&addr, &len) ==
                        SOCKET_ERROR)
                    {
                        TWarn(("Failed to get peer name of socket (err=%d).",
                               WSAGetLastError()));
                    }
                    else if (GetNameInfoW((SOCKADDR *)&addr,
                                          len,
                                          m_szAddrName,
                                          ARRAYSIZE(m_szAddrName),
                                          m_szPortName,
                                          ARRAYSIZE(m_szPortName),
                                          NI_NUMERICHOST) != NO_ERROR)
                    {
                        TWarn(("Failed to get socket name info (err=%d).",
                               WSAGetLastError()));
                    }
                    else
                    {
                        TInfo(("Connected to %ws:%ws",
                               m_szAddrName, m_szPortName));
                    }
                }

                if ((dwErr != NO_ERROR) && (m_socket != INVALID_SOCKET))
                {
                    closesocket(m_socket);
                    m_socket = INVALID_SOCKET;
                }
                FreeAddrInfoW(ai);
            }

            if (dwErr != NO_ERROR)
            {
                hr = HRESULT_FROM_WIN32(dwErr);
            }
        }

        TExitMsg(("=%x", hr));
        return hr;
    }   //InitConnection

public:
    /**
     *  Constructor of the class object.
     */
    WsaClient(
        VOID
        ): m_fInitialized(FALSE)
         , m_family(AF_INET)
         , m_sockType(SOCK_STREAM)
         , m_protocol(IPPROTO_TCP)
         , m_socket(INVALID_SOCKET)
    {
        TLevel(INIT);
        TEnter();

        ZeroMemory(&m_wsaData, sizeof(m_wsaData));
        m_szHost[0] = L'\0';
        m_szPort[0] = L'\0';
        m_szAddrName[0] = L'\0';
        m_szPortName[0] = L'\0';

        TExit();
        return;
    }   //WsaClient

    /**
     *  Desctructor of the class object.
     */
    ~WsaClient(
        VOID
        )
    {
        TLevel(INIT);
        TEnter();

        if (m_socket != INVALID_SOCKET)
        {
            TInfo(("Shutdown socket <%ws:%ws>", m_szHost, m_szPort));
            shutdown(m_socket, SD_BOTH);
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
        }

        if (m_fInitialized)
        {
            WSACleanup();
        }

        TExit();
        return;
    }   //~WsaClient

    /**
     *  This function initializes the WsaClient object.
     *
     *  @param pszHost Specifies the host name.
     *  @param pszPort Specifies the port number or service name.
     *  @param family Specifies the address family.
     *  @param sockType Specifies the socket type.
     *  @param protocol Specifies the protocol type.
     *
     *  @return Success: Returns S_OK.
     *  @return Failure: Returns HRESULT code.
     */
    HRESULT
    Initialize(
        __in LPCWSTR pszHost,
        __in LPCWSTR pszPort,
        __in int family,
        __in int sockType,
        __in int protocol
        )
    {
        HRESULT hr;

        TLevel(INIT);
        TEnterMsg(("Host=%ws,Port=%ws,family=%d,sockType=%d,protocol=%d",
                   pszHost, pszPort, family, sockType, protocol));

        if (m_fInitialized)
        {
            TErr(("WsaClient has already been initialized."));
            hr = HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
        }
        else if ((hr = StringCchCopyW(m_szHost, ARRAYSIZE(m_szHost), pszHost))
                 != S_OK)
        {
            TErr(("Failed to copy host string <%ws> (hr=%x).", pszHost, hr));
        }
        else if ((hr = StringCchCopyW(m_szPort, ARRAYSIZE(m_szPort), pszPort))
                 != S_OK)
        {
            TErr(("Failed to copy port string <%ws> (hr=%x).", pszPort, hr));
        }
        else if ((hr = HRESULT_FROM_WIN32(WSAStartup(MAKEWORD(2, 2),
                                                     &m_wsaData))) != S_OK)
        {
            TErr(("Failed to initialize WinSock (hr=%x).", hr));
        }
        else
        {
            m_fInitialized = TRUE;
            m_family = family;
            m_sockType = sockType;
            m_protocol = protocol;
            TInfo(("Version=%x-%x",
                   m_wsaData.wVersion, m_wsaData.wHighVersion));
            TInfo(("Description=<%s>", m_wsaData.szDescription));
            TInfo(("SystemStatus=<%s>", m_wsaData.szSystemStatus));
            TInfo(("MaxSockets=%d, MaxUdpDg=%d",
                   m_wsaData.iMaxSockets, m_wsaData.iMaxUdpDg));
            TInfo(("VendorInfo=%p", m_wsaData.lpVendorInfo));

            hr = InitConnection();
        }

        TExitMsg(("=%x", hr));
        return hr;
    }   //Initialize

    /**
     *  This function does an asynchronous read from the socket.
     *
     *  @param pbBuff Points to the buffer.
     *  @param dwcbLen Specifies the buffer size in bytes.
     *  @param lpdwcb Points to a variable to hold the number of characters
     *         read.
     *  @param overlapped Points to the overlapped structure.
     *
     *  @return Success: Returns S_OK.
     *  @return Failure: Returns HRESULT code.
     */
    HRESULT
    AsyncRead(
        __out_bcount(dwcbLen) LPBYTE pbBuff,
        __in                  DWORD dwcbLen,
        __out                 LPDWORD lpdwcb,
        __inout               LPWSAOVERLAPPED overlapped
        )
    {
        HRESULT hr = S_OK;

        TLevel(API);
        TEnterMsg(("buff=%p,len=%d,lpdwcb=%p,overlapped=%p",
                   pbBuff, dwcbLen, lpdwcb, overlapped));

        *lpdwcb = 0;
        if (m_socket == INVALID_SOCKET)
        {
            hr = InitConnection();
        }

        if (SUCCEEDED(hr))
        {
            DWORD dwErr;
            WSABUF WSABuff[1];
            DWORD dwFlags = 0;

            WSABuff[0].len = dwcbLen;
            WSABuff[0].buf = (LPSTR)pbBuff;
            dwErr = WSARecv(m_socket,
                            WSABuff,
                            1,
                            lpdwcb,
                            &dwFlags,
                            overlapped,
                            NULL);
            if (dwErr == ERROR_SUCCESS)
            {
                SetEvent(overlapped->hEvent);
            }
            else
            {
                dwErr = WSAGetLastError();
                if (dwErr == WSA_IO_PENDING)
                {
                    //
                    // The read is successfully queued.
                    //
                    dwErr = ERROR_SUCCESS;
                }
                else
                {
                    TWarn(("Failed to receive data from the socket (err=%d).",
                           dwErr));
                    closesocket(m_socket);
                    m_socket = INVALID_SOCKET;
                }
            }

            if (dwErr != ERROR_SUCCESS)
            {
                hr = HRESULT_FROM_WIN32(dwErr);
            }
        }

        TExitMsg(("=%x (len=%d)", hr, *lpdwcb));
        return hr;
    }   //AsyncRead

    /**
     *  This function does an asynchronous write to the socket.
     *
     *  @param pbBuff Points to the buffer.
     *  @param dwcbLen Specifies the buffer size in bytes.
     *  @param lpdwcb Points to a variable to hold the number of characters
     *         written.
     *  @param overlapped Points to the overlapped structure.
     *
     *  @return Success: Returns S_OK.
     *  @return Failure: Returns HRESULT code.
     */
    HRESULT
    AsyncWrite(
        __in_bcount(dwcbLen) LPBYTE pbBuff,
        __in                 DWORD dwcbLen,
        __out                LPDWORD lpdwcb,
        __inout              LPWSAOVERLAPPED overlapped
        )
    {
        HRESULT hr = S_OK;

        TLevel(API);
        TEnterMsg(("buff=%p,len=%d,lpdwcb=%p,overlapped=%p",
                   pbBuff, dwcbLen, lpdwcb, overlapped));

        *lpdwcb = 0;
        if (m_socket == INVALID_SOCKET)
        {
            hr = InitConnection();
        }

        if (SUCCEEDED(hr))
        {
            WSABUF WSABuff[1];
            DWORD dwErr;

            WSABuff[0].len = dwcbLen;
            WSABuff[0].buf = (LPSTR)pbBuff;
            dwErr = WSASend(m_socket,
                            WSABuff,
                            1,
                            lpdwcb,
                            0,
                            overlapped,
                            NULL);
            if (dwErr == ERROR_SUCCESS)
            {
                SetEvent(overlapped->hEvent);
            }
            else
            {
                dwErr = WSAGetLastError();
                if (dwErr == WSA_IO_PENDING)
                {
                    //
                    // The write is successfully queued.
                    //
                    dwErr = ERROR_SUCCESS;
                }
                else
                {
                    TWarn(("Failed to send data to the socket (err=%d).",
                           dwErr));
                    closesocket(m_socket);
                    m_socket = INVALID_SOCKET;
                }
            }

            if (dwErr != ERROR_SUCCESS)
            {
                hr = HRESULT_FROM_WIN32(dwErr);
            }
        }

        TExitMsg(("=%x (len=%d)", hr, *lpdwcb));
        return hr;
    }   //AsyncWrite

    /**
     *  This function does an synchronous read from the socket.
     *
     *  @param pbBuff Points to the buffer.
     *  @param dwcbLen Specifies the buffer size in bytes.
     *  @param lpdwcb Points to a variable to hold the number of characters
     *         read.
     *  @param dwTimeout Specifies the timeout value in milli-seconds, can be
     *         INFINITE.
     *
     *  @return Success: Returns S_OK.
     *  @return Failure: Returns HRESULT code.
     */
    HRESULT
    SyncRead(
        __out_bcount(dwcbLen) LPBYTE pbBuff,
        __in                  DWORD dwcbLen,
        __out                 LPDWORD lpdwcb,
        __in                  DWORD dwTimeout
        )
    {
        HRESULT hr = S_OK;
        OVERLAPPED overlapped;

        TLevel(API);
        TEnterMsg(("Socket=<%ws:%ws>,pbBuff=%p,dwcbLen=%d,lpdwcb=%p,Timeout=%d",
                   m_szHost, m_szPort, pbBuff, dwcbLen, lpdwcb, dwTimeout));

        ZeroMemory(&overlapped, sizeof(overlapped));
        overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (overlapped.hEvent == NULL)
        {
            hr = GETLASTHRESULT();
            TErr(("Failed to create overlapped read event (hr=%x).", hr));
        }
        else
        {
            hr = AsyncRead(pbBuff, dwcbLen, lpdwcb, &overlapped);
            if (SUCCEEDED(hr))
            {
                DWORD dwErr = WaitForSingleObject(overlapped.hEvent,
                                                  dwTimeout);

                if (dwErr == WAIT_OBJECT_0)
                {
                    DWORD dwFlags = 0;

                    dwErr = ERROR_SUCCESS;
                    if (!WSAGetOverlappedResult(m_socket,
                                                &overlapped,
                                                lpdwcb,
                                                FALSE,
                                                &dwFlags))
                    {
                        dwErr = WSAGetLastError();
                        if (dwErr == WSAECONNRESET)
                        {
                            TInfo(("The connection is closed at the server side."));
                        }
                        else
                        {
                            TErr(("Failed to get overlapped read result."));
                        }
                    }
                }
                else
                {
                    if (dwErr == WAIT_FAILED)
                    {
                        dwErr = GetLastError();
                    }
                    TErr(("Failed to wait for overlapped read (err=%d).",
                          dwErr));
                }

                if (dwErr != ERROR_SUCCESS)
                {
                    hr = HRESULT_FROM_WIN32(dwErr);
                }
            }
            CloseHandle(overlapped.hEvent);
        }

        TExitMsg(("=%x (len=%d)", hr, *lpdwcb));
        return hr;
    }   //SyncRead

    /**
     *  This function does an synchronous write to the socket.
     *
     *  @param pbBuff Points to the buffer.
     *  @param dwcbLen Specifies the buffer size in bytes.
     *  @param lpdwcb Points to a variable to hold the number of characters
     *         written.
     *  @param dwTimeout Specifies the timeout value in milli-seconds, can be
     *         INFINITE.
     *
     *  @return Success: Returns S_OK.
     *  @return Failure: Returns HRESULT code.
     */
    HRESULT
    SyncWrite(
        __in_bcount(dwcbLen) LPBYTE  pbBuff,
        __in                 DWORD   dwcbLen,
        __out                LPDWORD lpdwcb,
        __in                 DWORD   dwTimeout
        )
    {
        HRESULT hr = S_OK;
        OVERLAPPED overlapped;

        TLevel(API);
        TEnterMsg(("Socket=<%ws:%ws>,pbBuff=%p,dwcbLen=%d,lpdwcb=%p,Timeout=%d",
                   m_szHost, m_szPort, pbBuff, dwcbLen, lpdwcb, dwTimeout));

        ZeroMemory(&overlapped, sizeof(overlapped));
        overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (overlapped.hEvent == NULL)
        {
            hr = GETLASTHRESULT();
            TErr(("Failed to create overlapped write event (hr=%x).", hr));
        }
        else
        {
            hr = AsyncWrite(pbBuff, dwcbLen, lpdwcb, &overlapped);
            if (SUCCEEDED(hr))
            {
                DWORD dwErr = WaitForSingleObject(overlapped.hEvent,
                                                  dwTimeout);

                if (dwErr == WAIT_OBJECT_0)
                {
                    DWORD dwFlags = 0;

                    dwErr = ERROR_SUCCESS;
                    if (!WSAGetOverlappedResult(m_socket,
                                                &overlapped,
                                                lpdwcb,
                                                FALSE,
                                                &dwFlags))
                    {
                        dwErr = WSAGetLastError();
                        if (dwErr == WSAECONNRESET)
                        {
                            TInfo(("The connection is closed at the server side."));
                        }
                        else
                        {
                            TErr(("Failed to get overlapped read result."));
                        }
                    }
                }
                else
                {
                    if (dwErr == WAIT_FAILED)
                    {
                        dwErr = GetLastError();
                    }
                    TErr(("Failed to wait for overlapped write (err=%d).",
                          dwErr));
                }

                if (dwErr != ERROR_SUCCESS)
                {
                    hr = HRESULT_FROM_WIN32(dwErr);
                }
            }
            CloseHandle(overlapped.hEvent);
        }

        TExitMsg(("=%x (len=%d)", hr, *lpdwcb));
        return hr;
    }   //SyncWrite

};  //class WsaClient

