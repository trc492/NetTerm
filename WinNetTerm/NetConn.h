#if 0
/// Copyright (c) Titan Robotics Club. All rights reserved.
///
/// <module name="NetConn.h" />
///
/// <summary>
///     This module contains definitions and implementation of the NetConn
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
#define MOD_ID                  MOD_NETCONN

class NetConn: public WsaCallback
{
private:
    WsaServer  *m_server;
    WsaClient  *m_client;
    char        m_szRecvBuff[RECV_BUFF_SIZE];

public:
    /**
     *  Constructor for the NetConn class.
     */
    NetConn(
        VOID
        ): m_server(NULL)
         , m_client(NULL)
    {
        TLevel(INIT);
        TEnter();

        m_szRecvBuff[0] = '\0';

        TExit();
    }   //NetConn

    /**
     *  Destructor for the NetConn clas.
     */
    ~NetConn(
        VOID
        )
    {
        TLevel(INIT);
        TEnter();

        SAFE_DELETE(m_client);
        SAFE_DELETE(m_server);

        TExit();
    }   //~NetConn

    /**
     *  This function initializes the network by creating the server and
     *  client pair.
     *
     *  @param configParams Specifies the config parameters.
     *  @param hwndCallback Specifies the window handle for the callback.
     *
     *  @return Success: Returns S_OK.
     *  @return Failure: Returns HRESULT code.
     */
    HRESULT
    Initialize(
        __in PCONFIG_PARAMS configParams,
        __in HWND hwndCallback
        )
    {
        HRESULT hr = S_OK;

        TLevel(API);
        TEnterMsg(("configParams=%p,hwndCallback=%p",
                   configParams, hwndCallback));

        if ((m_server = new WsaServer()) == NULL)
        {
            hr = E_OUTOFMEMORY;
            MsgPrintf(g_progName, MSGTYPE_ERR, hr,
                      L"Failed to create server.");
        }
        else if ((m_client = new WsaClient()) == NULL)
        {
            hr = E_OUTOFMEMORY;
            MsgPrintf(g_progName, MSGTYPE_ERR, hr,
                      L"Failed to create client.");
        }
        else if ((hr = m_server->Initialize(configParams->szLocalPort,
                                            AF_INET,
                                            SOCKTYPE,
                                            PROTOCOL)) != S_OK)
        {
            MsgPrintf(g_progName, MSGTYPE_ERR, hr,
                      L"Failed to initialize server.");
        }
        else if ((hr = m_server->StartListener(this,
                                               hwndCallback,
                                               RECV_BUFF_SIZE,
                                               LISTENF_ASYNC)) != S_OK)
        {
            MsgPrintf(g_progName, MSGTYPE_ERR, hr,
                      L"Failed to start server listener.");
        }
        else if ((hr = m_client->Initialize(configParams->szRemoteAddr,
                                            configParams->szRemotePort,
                                            AF_INET,
                                            SOCKTYPE,
                                            PROTOCOL)) != S_OK)
        {
            MsgPrintf(g_progName, MSGTYPE_ERR, hr,
                      L"Failed to initialize client.");
        }

        TExitMsg(("=%x", hr));
        return hr;
    }   //Initialize

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

        if (recvLen + 1 > sizeof(m_szRecvBuff))
        {
            TErr(("Receive length exceeds buffer size (len=%d).", dwRecvLen));
        }
        else
        {
            RtlCopyMemory(m_szRecvBuff, recvBuff, recvLen);
            m_szRecvBuff[recvLen] = '\0';
            SendMessageA((HWND)context,
                         EM_REPLACESEL,
                         FALSE,
                         (LPARAM)m_szRecvBuff);
        }

        TExit();
        return;
    }   //DataReceived

    /**
     *  This function calls the client interface to send the data.
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
    SendData(
        __in_bcount(dwcbLen) LPBYTE  pbBuff,
        __in                 DWORD   dwcbLen,
        __out                LPDWORD lpdwcb,
        __in                 DWORD   dwTimeout
        )
    {
        HRESULT hr;

        TLevel(API);
        TEnterMsg(("pbBuff=%p,dwcbLen=%d,lpdwcb=%p,Timeout=%d",
                   pbBuff, dwcbLen, lpdwcb, dwTimeout));

        hr = m_client->SyncWrite(pbBuff, dwcbLen, lpdwcb, dwTimeout);

        TExitMsg(("=%x", hr));
        return hr;
    }   //SendData

};  //class NetConn

