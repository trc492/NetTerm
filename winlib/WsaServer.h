#if 0
/// Copyright (c) Titan Robotics Club. All rights reserved.
///
/// <module name="WsaServer.h" />
///
/// <summary>
///     This module contains the definitions and implementation of the
///     WsaServer class.
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
#define MOD_ID                  MOD_SERVER

/**
 *  This abstract class defines the WsaCallback object. The object is a
 *  callback interface. It is not meant to be created as an object.
 *  Instead, it should be inherited by a subclass who needs to be called
 *  back when the server receives data.
 */
class WsaCallback
{
public:
    virtual
    VOID
    DataReceived(
        __in                 HANDLE connHandle,
        __in_opt             LPVOID context,
        __in_bcount(recvLen) LPBYTE recvBuff,
        __in                 DWORD  recvLen
        ) = 0;

};  //class WsaCallback

class WsaServer
{
private:
    #define LISTENF_MASK                0x0000ffff
    #define LISTENF_TERMINATING         0x80000000
    #define SIG_SERVERCONNECTION        'CvrS'
    #define TERMINATE_TIMEOUT           1000

    typedef struct _conn
    {
        LIST_ENTRY  list;
        DWORD       dwSig;
        SOCKET      socket;
        LPBYTE      dataBuffer;
        DWORD       dataIndex;
        OVERLAPPED  overlapped;
        HANDLE      hClosedEvent;
        SOCKADDR    fromAddr;
        int         fromLen;
    } CONN, *PCONN;

    //
    // Private data.
    //
    BOOL        m_fInitialized;
    int         m_family;
    int         m_sockType;
    int         m_protocol;
    SOCKET      m_socket;
    WSADATA     m_wsaData;
    WCHAR       m_szPort[NI_MAXSERV];

    WsaCallback *m_dataCallback;
    LPVOID      m_callbackContext;
    DWORD       m_dataBufferSize;
    DWORD       m_dwFlags;

    HANDLE      m_hListenerThread;
    HANDLE      m_hConnectionThread;
    HANDLE      m_hChangedEvent;
    DList       m_connectionList;

    friend
    DWORD WINAPI
    ListenerThreadProc(
        __in LPVOID lpParam
        );

	friend
	DWORD WINAPI
	ConnectionThreadProc(
		__in LPVOID lpParam
		);

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
            hints.ai_flags = AI_PASSIVE;
            dwErr = GetAddrInfoW(NULL, m_szPort, &hints, &ai);
            if (dwErr != NO_ERROR)
            {
                TWarn(("Failed to get address info."));
            }
            else
            {
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
                else if (bind(m_socket, ai->ai_addr, (int)ai->ai_addrlen) ==
                         SOCKET_ERROR)
                {
                    dwErr = WSAGetLastError();
                    TErr(("Failed to bind socket (err=%d).", dwErr));
                }
                else if ((m_sockType != SOCK_DGRAM) &&
                         (listen(m_socket, 5) == SOCKET_ERROR))
                {
                    hr = HRESULT_FROM_WIN32(WSAGetLastError());
                    TErr(("Failed to put socket in listening state (hr=%x).",
                          hr));
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

    /**
     *  This function implements the listener thread. Listener is only needed
     *  for STREAM type of connection (e.g. TCP) where multiple streams are
     *  possible.
     *
     *  @return Success: Returns S_OK.
     *  @return Failure: Returns HRESULT code.
     */
    HRESULT
    ListenerThread(
        VOID
        )
    {
        HRESULT hr = S_OK;
        SOCKET socket;
        SOCKADDR_STORAGE saClient;
        int iClientSize;

        TLevel(CALLBK);
        TEnter();

        while (SUCCEEDED(hr))
        {
            TInfo(("Waiting for connection..."));
            iClientSize = sizeof(saClient);
            socket = WSAAccept(m_socket,
                               (PSOCKADDR)&saClient,
                               &iClientSize,
                               NULL,
                               0);
            if (socket == INVALID_SOCKET)
            {
                hr = HRESULT_FROM_WIN32(WSAGetLastError());
                if (HRESULT_CODE(hr) == WSAEINTR)
                {
                    TInfo(("Received a termination request."));
                    hr = S_OK;
                    break;
                }
                else
                {
                    //
                    // Ignore it and try again.
                    //
                    TErr(("Failed to accept connection (hr=%x).", hr));
                    hr = S_OK;
                }
            }
            else
            {
                WCHAR szConnAddr[32];
                DWORD dwLen = ARRAYSIZE(szConnAddr);

                TInfo(("Accepted connection (socket=%x).",
                       (unsigned int)socket));
                if (WSAAddressToStringW((PSOCKADDR)&saClient,
                                        iClientSize,
                                        NULL,
                                        szConnAddr,
                                        &dwLen) == NO_ERROR)
                {
                    TInfo(("New connection address: %ws", szConnAddr));
                }

                hr = StartConnection(socket);
            }
        }

        //
        // The listener thread is about to die, let's clean up.
        //
        StopConnection();

        TExitMsg(("=%x", hr));
        return hr;
    }   //ListenerThread

    /**
     *  This function starts a connection. This includes creating a connection
     *  thread to monitor data from the message socket.
     *
     *  @param socket Specifies the socket for the connection to receive
     *         message from.
     *
     *  @return Success: Returns S_OK.
     *  @return Failure: Returns HRESULT code.
     */
    HRESULT
    StartConnection(
        __in SOCKET socket
        )
    {
        HRESULT hr = S_OK;

        TLevel(FUNC);
        TEnterMsg(("socket=%x", socket));

        if (m_hConnectionThread == NULL)
        {
            //
            // If we haven't started the connection thread yet, start it now.
            //
            TInfo(("Creating Changed event..."));
            TAssert(m_hChangedEvent == NULL);
            m_hChangedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (m_hChangedEvent == NULL)
            {
                hr = GETLASTHRESULT();
                TErr(("Failed to create changed event (hr=%x).", hr));
            }
            else
            {
                TInfo(("Creating Connection thread..."));
                m_hConnectionThread = CreateThread(NULL,
                                                   0,
                                                   ConnectionThreadProc,
                                                   this,
                                                   0,
                                                   NULL);
                if (m_hConnectionThread == NULL)
                {
                    CloseHandle(m_hChangedEvent);
                    m_hChangedEvent = NULL;
                    hr = GETLASTHRESULT();
                    TErr(("Failed to create connection thread (hr=%x).", hr));
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            PCONN conn = new CONN;

            if (conn == NULL)
            {
                hr = E_OUTOFMEMORY;
                TErr(("Failed to allocate connection structure (len=%d).",
                      sizeof(CONN)));
            }
            else
            {
                ZeroMemory(conn, sizeof(*conn));
                conn->dwSig = SIG_SERVERCONNECTION;
                conn->socket = socket;

                if ((conn->dataBuffer = new BYTE[m_dataBufferSize]) == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    TErr(("Failed to allocate message buffer (len=%d).",
                          m_dataBufferSize));
                }
                else if ((conn->overlapped.hEvent = CreateEvent(NULL,
                                                                FALSE,
                                                                FALSE,
                                                                NULL)) == NULL)
                {
                    hr = GETLASTHRESULT();
                    TErr(("Failed to create overlapped event for the connection."));
                }
                else if ((conn->hClosedEvent = CreateEvent(NULL,
                                                           FALSE,
                                                           FALSE,
                                                           NULL)) == NULL)
                {
                    hr = GETLASTHRESULT();
                    TErr(("Failed to create closed event (hr=%x).", hr));
                }
                else
                {
                    DWORD dwcb;
                    //
                    // Do an async read for this connection here.
                    //
                    hr = AsyncRead(conn,
                                   conn->dataBuffer,
                                   m_dataBufferSize,
                                   &dwcb,
                                   &conn->overlapped);
                    if (SUCCEEDED(hr))
                    {
                        m_connectionList.InsertTailDList(&conn->list);
                        SetEvent(m_hChangedEvent);
                    }
                }

                if (FAILED(hr))
                {
                    if (conn->hClosedEvent != NULL)
                    {
                        CloseHandle(conn->hClosedEvent);
                        conn->hClosedEvent = NULL;
                    }

                    if (conn->overlapped.hEvent != NULL)
                    {
                        CloseHandle(conn->overlapped.hEvent);
                        conn->overlapped.hEvent = NULL;
                    }

                    SAFE_DELETE(conn->dataBuffer);
                    closesocket(socket);
                    delete conn;
                }
            }
        }

        TExitMsg(("=%x", hr));
        return hr;
    }   //StartConnection

    /**
     *  This function stops the Connection thread and clean up.
     *
     *  @return Success: Returns S_OK.
     *  @return Failure: Returns HRESULT code.
     */
    HRESULT
    StopConnection(
        VOID
        )
    {
        HRESULT hr = S_OK;
        DWORD rcWait;
        PLIST_ENTRY entry;
        PCONN conn;

        TLevel(FUNC);
        TEnter();
        //
        // Close and free all connections.
        //
        while ((entry = m_connectionList.RemoveHeadDList()) != NULL)
        {
            conn = CONTAINING_RECORD(entry, CONN, list);
            CloseConnection(conn);
            CleanupConnection(conn);
        }

        if (m_hChangedEvent != NULL)
        {
            TInfo(("Signaling connection thread to die..."));
            m_dwFlags |= LISTENF_TERMINATING;
            SetEvent(m_hChangedEvent);
            CloseHandle(m_hChangedEvent);
            m_hChangedEvent = NULL;
        }

        if (m_hConnectionThread != NULL)
        {
            //
            // Terminate the connection thread.
            //
            TInfo(("Waiting for the connection thread to die..."));
            rcWait = WaitForSingleObject(m_hConnectionThread,
                                         TERMINATE_TIMEOUT);
            if (rcWait != WAIT_OBJECT_0)
            {
                hr = (rcWait == WAIT_FAILED)? GETLASTHRESULT():
                                              HRESULT_FROM_WIN32(rcWait);
                TErr(("Failed waiting for the connection thread to die (hr=%x).",
                      hr));
            }
            CloseHandle(m_hConnectionThread);
            m_hConnectionThread = NULL;
        }

        TExitMsg(("=%x", hr));
        return hr;
    }   //StopConnection

    /**
     *  This function closes the connection.
     *
     *  @param conn Points to the CONN structure.
     *
     *  @return Success: Returns S_OK.
     *  @return Failure: Returns HRESULT code.
     */
    HRESULT
    CloseConnection(
        __in PCONN conn
        )
    {
        HRESULT hr = S_OK;

        TLevel(FUNC);
        TEnterMsg(("conn=%p", conn));

        if (conn->socket != INVALID_SOCKET)
        {
            TInfo(("Shutting down connection socket %x.",
                   (unsigned int)conn->socket));
            shutdown(conn->socket, SD_BOTH);
            closesocket(conn->socket);
            conn->socket = INVALID_SOCKET;
            if (conn->hClosedEvent != NULL)
            {
                DWORD rcWait = WaitForSingleObject(conn->hClosedEvent,
                                                   TERMINATE_TIMEOUT);
                if (rcWait != WAIT_OBJECT_0)
                {
                    hr = (rcWait == WAIT_FAILED)? GETLASTHRESULT():
                                                  HRESULT_FROM_WIN32(rcWait);
                    TErr(("Failed waiting for connection close (hr=%x).", hr));
                }
                CloseHandle(conn->hClosedEvent);
                conn->hClosedEvent = NULL;
            }
        }

        TExitMsg(("=%x", hr));
        return hr;
    }   //CloseConnection

    /**
     *  This function deallocates the connection.
     *
     *  @param conn Points to the CONN structure.
     *
     *  @return Success: Returns S_OK.
     *  @return Failure: Returns HRESULT code.
     */
    HRESULT
    CleanupConnection(
        __in PCONN conn
        )
    {
        HRESULT hr = S_OK;

        TLevel(FUNC);
        TEnterMsg(("conn=%p", conn));

        if (conn->socket != INVALID_SOCKET)
        {
            //
            // If we come here, the connection must have been aborted from
            // the client side, so we close the socket here.
            //
            TInfo(("Shutting down connection socket %x.",
                   (unsigned int)conn->socket));
            closesocket(conn->socket);
            conn->socket = INVALID_SOCKET;
        }

        if (conn->dataBuffer != NULL)
        {
            TInfo(("Deallocating receive buffer."));
            delete [] conn->dataBuffer;
            conn->dataBuffer = NULL;
        }

        if (conn->hClosedEvent != NULL)
        {
            TInfo(("Closing closed event handle %p.", conn->hClosedEvent));
            CloseHandle(conn->hClosedEvent);
            conn->hClosedEvent = NULL;
        }

        if (conn->overlapped.hEvent != NULL)
        {
            TInfo(("Closing overlappedRead event handle %p.",
                   conn->overlapped.hEvent));
            CloseHandle(conn->overlapped.hEvent);
            conn->overlapped.hEvent = NULL;
        }

        delete conn;

        TExitMsg(("=%x", hr));
        return hr;
    }   //CleanupConnection

    /**
     *  This function implements the connection thread.
     *
     *  @return Success: Returns S_OK.
     *  @return Failure: Returns HRESULT code.
     */
    HRESULT
    ConnectionThread(
        VOID
        )
    {
        HRESULT hr = S_OK;
        BOOL fChanged = TRUE;
        int n = 0;
        PHANDLE ahWaits = NULL;
        PCONN *aConns = NULL;

        TLevel(CALLBK);
        TEnter();

        while (hr == S_OK)
        {
            if (fChanged == TRUE)
            {
                fChanged = FALSE;
                n = 0;
                //
                // Deallocate old handles and CONN pointers.
                //
                if (ahWaits != NULL)
                {
                    //
                    // Deallocating old wait handles array.
                    //
                    delete [] ahWaits;
                    ahWaits = NULL;
                }
                if (aConns != NULL)
                {
                    //
                    // Deallocating old Conn pointers array.
                    //
                    delete [] aConns;
                    aConns = NULL;
                }
                //
                // Determine the number of CONN entries to monitor.
                //
                m_connectionList.EnterCritSect();
                n += m_connectionList.QueryEntriesDList();
                TInfo(("Number of connections = %d.", n));
                //
                // Create new wait handles and CONN pointer array.
                //
                ahWaits = new HANDLE[n + 1];
                if (ahWaits == NULL)
                {
                    TErr(("Failed to allocate %d wait handles.", n + 1));
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    ahWaits[n] = m_hChangedEvent;
                    if (n > 0)
                    {
                        aConns = new PCONN[n];
                        if (aConns == NULL)
                        {
                            TErr(("Failed to allocate Conn pointer array (n=%d).",
                                  n));
                            hr = E_OUTOFMEMORY;
                        }
                    }
                }
                //
                // Walk the Conn entry list and fill the handle array and
                // initiate an async recv on the connection.
                //
                if ((hr == S_OK) && (n > 0))
                {
                    PLIST_ENTRY entry;
                    PCONN conn;
                    int i;

                    entry = m_connectionList.GetHeadDList();
                    for (i = 0;
                         (i < n) && (entry != NULL);
                         entry = m_connectionList.GetNextDList(entry), i++)
                    {
                        conn = CONTAINING_RECORD(entry, CONN, list);
                        aConns[i] = conn;
                        ahWaits[i] = conn->overlapped.hEvent;
                    }
                    TAssert((entry == NULL) && (i == n));
                }
                m_connectionList.LeaveCritSect();
            }

            if (SUCCEEDED(hr))
            {
                DWORD rcWait;

                TInfo(("Waiting for connection data..."));
                rcWait = WaitForMultipleObjects(n + 1,
                                                ahWaits,
                                                FALSE,
                                                INFINITE);
                if (rcWait == WAIT_OBJECT_0 + n)
                {
                    if (m_dwFlags & LISTENF_TERMINATING)
                    {
                        TInfo(("Received a termination event."));
                        break;
                    }
                    else
                    {
                        //
                        // Something has changed, either a new CONN was added
                        // or an existing CONN was removed.  Let's re-evaluate
                        // the link list.
                        //
                        fChanged = TRUE;
                        TInfo(("Received changed event."));
                    }
                }
                else if (rcWait < WAIT_OBJECT_0 + n)
                {
                    PCONN conn = aConns[rcWait - WAIT_OBJECT_0];

                    TInfo(("Received data for connection %p.", conn));
                    hr = ProcessConnectionData(conn);
                    if (hr != S_OK)
                    {
                        //
                        // If the connection is aborted from the client side,
                        // we get WSAECONNRESET or ERROR_OPERATION_ABORTED.
                        // If the socket is closed from our side, we get
                        // WSAENOTSOCK.
                        //
                        TInfo(("Connection %p is closed.", conn));
                        if (HRESULT_CODE(hr) == WSAENOTSOCK)
                        {
                            //
                            // The connection is shutting down from our side.
                            // Somebody is waiting for the connection to close.
                            // So let's signal it.
                            //
                            if (conn->hClosedEvent != NULL)
                            {
                                SetEvent(conn->hClosedEvent);
                            }
                        }
                        else
                        {
                            //
                            // The connection is shutting down by the client,
                            // so we need to clean up here.
                            //
                            m_connectionList.RemoveEntryDList(&conn->list);
                            CleanupConnection(conn);
                        }
                        fChanged = TRUE;
                        hr = S_OK;
                    }
                }
                else
                {
                    hr = (rcWait == WAIT_FAILED)? GETLASTHRESULT():
                                                  HRESULT_FROM_WIN32(rcWait);
                    TErr(("Received unexpected event (hr=%x).", hr));
                }
            }
        }

        if (ahWaits != NULL)
        {
            delete [] ahWaits;
        }

        if (aConns != NULL)
        {
            delete [] aConns;
        }

        TExitMsg(("=%x", hr));
        return hr;
    }   //ConnectionThread

    /**
     *  This function processes the data received from a connection.
     *
     *  @param conn Points to the CONN structure.
     *
     *  @return Success: Returns S_OK.
     *  @return Failure: Returns HRESULT code.
     */
    HRESULT
    ProcessConnectionData(
        __in PCONN conn
        )
    {
        HRESULT hr = S_OK;
        DWORD dwFlags = 0;
        DWORD dwcb;

        TLevel(FUNC);
        TEnterMsg(("conn=%p", conn));

        if (!WSAGetOverlappedResult(conn->socket,
                                    &conn->overlapped,
                                    &dwcb,
                                    FALSE,
                                    &dwFlags))
        {
            hr = HRESULT_FROM_WIN32(WSAGetLastError());
            if (HRESULT_CODE(hr) == WSAENOTSOCK)
            {
                TInfo(("Socket is shutting down."));
            }
            else if (HRESULT_CODE(hr) == WSAECONNRESET)
            {
                TInfo(("Client has died unexpectedly."));
            }
            else
            {
                TErr(("Failed to get overlappedRead result (hr=%x).", hr));
            }
        }
        else if (dwcb > 0)
        {
            TInfo(("Got a data packet (Len=%d).", dwcb));
            //
            // Note: If the callback is going to take substantial amount
            // of time to process, the callback function should process
            // the request with a different thread and return this thread
            // immediately.  The callback function is responsible for
            // deallocating the buffer.
            //
            m_dataCallback->DataReceived((HANDLE)conn,
                                         m_callbackContext,
                                         conn->dataBuffer,
                                         dwcb);
            hr = AsyncRead((HANDLE)conn,
                           conn->dataBuffer,
                           m_dataBufferSize,
                           &dwcb,
                           &conn->overlapped);
        }
        else
        {
            //
            // Connection has been terminated from the client side.
            //
            TInfo(("Connection is shutdown by client."));
            hr = HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED);
        }

        TExitMsg(("=%x", hr));
        return hr;
    }   //ProcessConnectionData

public:
    //
    // Public constants.
    //
    #define LISTENF_ASYNC               0x00000001

    /**
     *  Constructor of the class object.
     */
    WsaServer(
        VOID
        ): m_fInitialized(FALSE)
         , m_family(AF_UNSPEC)
         , m_sockType(SOCK_STREAM)
         , m_protocol(IPPROTO_TCP)
         , m_socket(INVALID_SOCKET)
         , m_dataCallback(NULL)
         , m_callbackContext(NULL)
         , m_dataBufferSize(0)
         , m_dwFlags(0)
         , m_hListenerThread(NULL)
         , m_hConnectionThread(NULL)
         , m_hChangedEvent(NULL)
         , m_connectionList()
    {
        TLevel(INIT);
        TEnter();

        ZeroMemory(&m_wsaData, sizeof(m_wsaData));
        m_szPort[0] = L'\0';

        TExit();
        return;
    }   //WsaServer

    /**
     *  Desctructor of the class object.
     */
    ~WsaServer(
        VOID
        )
    {
        TLevel(INIT);
        TEnter();

        StopListener();
        if (m_fInitialized)
        {
            WSACleanup();
        }

        TExit();
        return;
    }   //~WsaServer

    /**
     *  This function initializes the WsaServer object.
     *
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
        __in LPCWSTR pszPort,
        __in int family,
        __in int sockType,
        __in int protocol
        )
    {
        HRESULT hr;

        TLevel(INIT);
        TEnterMsg(("Port=%ws,family=%d,sockType=%d,protocol=%d",
                   pszPort, family, sockType, protocol));

        if (m_fInitialized)
        {
            TErr(("WsaServer has already been initialized."));
            hr = HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
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
     *  This function starts a listener.
     *
     *  @param dataCallback Points to the data callback interface.
     *  @param callbackContext Specifies the callback context.
     *  @param dataBufferSize Specifies the read/write buffer size.
     *  @param dwFlags Specifies option flags.
     *
     *  @return Success: Returns S_OK.
     *  @return Failure: Returns HRESULT code.
     */
    HRESULT
    StartListener(
        __in     WsaCallback *dataCallback,
        __in_opt LPVOID callbackContext,
        __in     DWORD dataBufferSize,
        __in     DWORD dwFlags
        )
    {
        HRESULT hr = S_OK;

        TLevel(API);
        TEnterMsg(("callbk=%p,ctxt=%p,buffSize=%d,flags=%x",
                   dataCallback, callbackContext, dataBufferSize, dwFlags));

        if ((dataCallback == NULL) || (dataBufferSize == 0))
        {
            TWarn(("Invalid parameter."));
            hr = E_INVALIDARG;
        }
        else if (!m_fInitialized)
        {
            TErr(("WsaServer was not initialized."));
            hr = HRESULT_FROM_WIN32(ERROR_NOT_READY);
        }
        else
        {
            m_dataCallback = dataCallback;
            m_callbackContext = callbackContext;
            m_dataBufferSize = dataBufferSize;
            m_dwFlags = dwFlags & LISTENF_MASK;

            if (m_sockType == SOCK_DGRAM)
            {
                //
                // No need for listener when using datagram.
                //
                hr = StartConnection(m_socket);
                if (SUCCEEDED(hr))
                {
                    //
                    // The socket becomes the message socket for the
                    // connection.
                    //
                    m_socket = INVALID_SOCKET;
                }
            }
            else if (dwFlags & LISTENF_ASYNC)
            {
                m_hListenerThread = CreateThread(NULL,
                                                 0,
                                                 ListenerThreadProc,
                                                 this,
                                                 0,
                                                 NULL);
                if (m_hListenerThread == NULL)
                {
                    hr = GETLASTHRESULT();
                    TErr(("Failed to create listener thread (hr=%x).", hr));
                }
            }
            else
            {
                //
                // Since this is synchronous, we are using the caller thread
                // as the listener thread.
                //
                hr = ListenerThread();
            }
        }

        TExitMsg(("=%x", hr));
        return hr;
    }   //StartListener

    /**
     *  This function stops the listener.
     *
     *  @return Success: Returns S_OK.
     *  @return Failure: Returns HRESULT code.
     */
    HRESULT
    StopListener(
        VOID
        )
    {
        HRESULT hr = S_OK;

        TLevel(API);
        TEnter();

        if (m_socket != INVALID_SOCKET)
        {
            //
            // Close the listener socket will terminate the listener thread.
            //
            TInfo(("Shutting down socket %x.", (unsigned int)m_socket));
            shutdown(m_socket, SD_BOTH);
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
        }

        if (m_hListenerThread != NULL)
        {
            TInfo(("Waiting for the listener thread to die..."));
            DWORD rcWait = WaitForSingleObject(m_hListenerThread,
                                               TERMINATE_TIMEOUT);
            if (rcWait != WAIT_OBJECT_0)
            {
                hr = (rcWait == WAIT_FAILED)? GETLASTHRESULT():
                                              HRESULT_FROM_WIN32(rcWait);
                TErr(("Failed waiting for the listener thread to die (hr=%x).",
                      hr));
            }
            CloseHandle(m_hListenerThread);
            m_hListenerThread = NULL;
        }

        TExit();
        return hr;
    }   //StopListener

    /**
     *  This function does an asynchronous read from the socket.
     *
     *  @param hConn Specifies the handle of the connection.
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
        __in                  HANDLE hConn,
        __out_bcount(dwcbLen) LPBYTE pbBuff,
        __in                  DWORD dwcbLen,
        __out                 LPDWORD lpdwcb,
        __inout               LPWSAOVERLAPPED overlapped
        )
    {
        HRESULT hr = S_OK;
        PCONN conn = (PCONN)hConn;

        TLevel(API);
        TEnterMsg(("conn=%p,buff=%p,len=%d,lpdwcb=%p,overlapped=%p",
                   conn, pbBuff, dwcbLen, lpdwcb, overlapped));

        *lpdwcb = 0;
        if ((conn == NULL) ||
            (conn->dwSig != SIG_SERVERCONNECTION) ||
            (conn->socket == INVALID_SOCKET))
        {
            TErr(("Invalid connection handle."));
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
        }
        else
        {
            DWORD dwErr;
            WSABUF WSABuff[1];
            DWORD dwFlags = 0;

            conn->fromLen = sizeof(conn->fromAddr);
            WSABuff[0].len = dwcbLen;
            WSABuff[0].buf = (LPSTR)pbBuff;
            if (m_sockType == SOCK_DGRAM)
            {
                dwErr = WSARecvFrom(conn->socket,
                                    WSABuff,
                                    1,
                                    lpdwcb,
                                    &dwFlags,
                                    &conn->fromAddr,
                                    &conn->fromLen,
                                    overlapped,
                                    NULL);
            }
            else
            {
                dwErr = WSARecv(conn->socket,
                                WSABuff,
                                1,
                                lpdwcb,
                                &dwFlags,
                                overlapped,
                                NULL);
            }

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
     *  @param hConn Specifies the handle of the connection.
     *  @param pbBuff Points to the buffer.
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
        __in                 HANDLE hConn,
        __in_bcount(dwcbLen) LPBYTE pbBuff,
        __in                 DWORD dwcbLen,
        __out                LPDWORD lpdwcb,
        __inout              LPWSAOVERLAPPED overlapped
        )
    {
        HRESULT hr = S_OK;
        PCONN conn = (PCONN)hConn;

        TLevel(API);
        TEnterMsg(("conn=%p,buff=%p,len=%d,lpdwcb=%p,overlapped=%p",
                   conn, pbBuff, dwcbLen, lpdwcb, overlapped));

        *lpdwcb = 0;
        if ((conn == NULL) ||
            (conn->dwSig != SIG_SERVERCONNECTION) ||
            (conn->socket == INVALID_SOCKET))
        {
            TErr(("Invalid connection handle."));
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
        }
        else
        {
            WSABUF WSABuff[1];
            DWORD dwErr;

            WSABuff[0].len = dwcbLen;
            WSABuff[0].buf = (LPSTR)pbBuff;
            if (m_sockType == SOCK_DGRAM)
            {
                dwErr = WSASendTo(conn->socket,
                                  WSABuff,
                                  1,
                                  lpdwcb,
                                  0,
                                  &conn->fromAddr,
                                  conn->fromLen,
                                  overlapped,
                                  NULL);
            }
            else
            {
                dwErr = WSASend(conn->socket,
                                WSABuff,
                                1,
                                lpdwcb,
                                0,
                                overlapped,
                                NULL);
            }

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
     *  @param hConn Specifies the handle of the connection.
     *  @param pbBuff Points to the buffer.
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
        __in                  HANDLE hConn,
        __out_bcount(dwcbLen) LPBYTE pbBuff,
        __in                  DWORD dwcbLen,
        __out                 LPDWORD lpdwcb,
        __in                  DWORD dwTimeout
        )
    {
        HRESULT hr = S_OK;
        PCONN conn = (PCONN)hConn;
        OVERLAPPED overlapped;

        TLevel(API);
        TEnterMsg(("conn=%p,Socket=<%ws>,pbBuff=%p,dwcbLen=%d,lpdwcb=%p,Timeout=%d",
                   conn, m_szPort, pbBuff, dwcbLen, lpdwcb, dwTimeout));

        ZeroMemory(&overlapped, sizeof(overlapped));
        overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (overlapped.hEvent == NULL)
        {
            hr = GETLASTHRESULT();
            TErr(("Failed to create overlapped read event (hr=%x).", hr));
        }
        else
        {
            hr = AsyncRead(hConn, pbBuff, dwcbLen, lpdwcb, &overlapped);
            if (SUCCEEDED(hr))
            {
                DWORD dwErr = WaitForSingleObject(overlapped.hEvent,
                                                  dwTimeout);

                if (dwErr == WAIT_OBJECT_0)
                {
                    DWORD dwFlags = 0;

                    dwErr = ERROR_SUCCESS;
                    if (!WSAGetOverlappedResult(conn->socket,
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
                            TErr(("Failed to get overlapped read result (err=%d).",
                                  dwErr));
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
     *  @param hConn Specifies the handle of the connection.
     *  @param pbBuff Points to the buffer.
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
        __in                 HANDLE hConn,
        __in_bcount(dwcbLen) LPBYTE pbBuff,
        __in                 DWORD dwcbLen,
        __out                LPDWORD lpdwcb,
        __in                 DWORD dwTimeout
        )
    {
        HRESULT hr = S_OK;
        PCONN conn = (PCONN)hConn;
        OVERLAPPED overlapped;

        TLevel(API);
        TEnterMsg(("conn=%p,Socket=<%ws>,pbBuff=%p,dwcbLen=%d,lpdwcb=%p,Timeout=%d",
                   conn, m_szPort, pbBuff, dwcbLen, lpdwcb, dwTimeout));

        ZeroMemory(&overlapped, sizeof(overlapped));
        overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (overlapped.hEvent == NULL)
        {
            hr = GETLASTHRESULT();
            TErr(("Failed to create overlapped write event (hr=%x).", hr));
        }
        else
        {
            hr = AsyncWrite(hConn, pbBuff, dwcbLen, lpdwcb, &overlapped);
            if (SUCCEEDED(hr))
            {
                DWORD dwErr = WaitForSingleObject(overlapped.hEvent,
                                                  dwTimeout);

                if (dwErr == WAIT_OBJECT_0)
                {
                    DWORD dwFlags = 0;

                    dwErr = ERROR_SUCCESS;
                    if (!WSAGetOverlappedResult(conn->socket,
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
                            TErr(("Failed to get overlapped read result (err=%d).",
                                  dwErr));
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

};  //class WsaServer

#ifdef _MAIN_FILE

/**
 *  This function implements the listener thread.
 *
 *  @param lpParam Points to thread data passed to the function.
 *
 *  @return Success: Returns S_OK
 *  @return Failure: Returns HRESULT code.
 */
DWORD WINAPI
ListenerThreadProc(
    __in LPVOID lpParam
    )
{
    DWORD rc;
    WsaServer *server = (WsaServer *)lpParam;

    TLevel(CALLBK);
    TEnterMsg(("param=%p", lpParam));

    rc = (DWORD)server->ListenerThread();

    TExitMsg(("=%x", rc));
    return rc;
}   //ListenerThreadProc

/**
 *  This function implements the connection thread.
 *
 *  @param lpParam Points to thread data passed to the function.
 *
 *  @return Success: Returns ERROR_SUCCESS.
 *  @return Failure: Returns Win32 error code.
 */
DWORD WINAPI
ConnectionThreadProc(
    __in LPVOID lpParam
    )
{
    DWORD rc;
    WsaServer *server = (WsaServer *)lpParam;

    TLevel(CALLBK);
    TEnterMsg(("param=%p", lpParam));

    rc = (DWORD)server->ConnectionThread();

    TExitMsg(("=%x", rc));
    return rc;
}   //ConnectionThreadProc

#endif

