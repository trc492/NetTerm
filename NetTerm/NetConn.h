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

class NetConn
{
private:
    WsaServer  *m_server;
    WsaClient  *m_client;

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
     *  @param callback Specifies the callback interface to call.
     *
     *  @return Success: Returns S_OK.
     *  @return Failure: Returns HRESULT code.
     */
    HRESULT
    Initialize(
        __in PCONFIG_PARAMS configParams,
        __in WsaCallback *callback
        )
    {
        HRESULT hr = S_OK;

        TLevel(API);
        TEnterMsg(("configParams=%p,callback=%p", configParams, callback));

        if ((m_server = new WsaServer()) == NULL)
        {
            hr = E_OUTOFMEMORY;
            MsgPrintf(g_progName, MSGTYPE_ERR, hr,
                      L"Failed to create server.");
        }
        else if (!(g_progFlags & NETTERMF_NOCLIENT) &&
                 ((m_client = new WsaClient()) == NULL))
        {
            hr = E_OUTOFMEMORY;
            MsgPrintf(g_progName, MSGTYPE_ERR, hr,
                      L"Failed to create client.");
        }
        else if ((hr = m_server->Initialize(configParams->szLocalPort,
                                            AF_INET,
                                            configParams->sockType,
                                            configParams->protocol)) != S_OK)
        {
            MsgPrintf(g_progName, MSGTYPE_ERR, hr,
                      L"Failed to initialize server.");
        }
        else if ((hr = m_server->StartListener(callback,
                                               NULL,
                                               RECV_BUFF_SIZE,
                                               LISTENF_ASYNC)) != S_OK)
        {
            MsgPrintf(g_progName, MSGTYPE_ERR, hr,
                      L"Failed to start server listener.");
        }
        else if ((m_client != NULL) &&
                 ((hr = m_client->Initialize(configParams->szRemoteAddr,
                                             configParams->szRemotePort,
                                             AF_INET,
                                             configParams->sockType,
                                             configParams->protocol)) != S_OK))
        {
            MsgPrintf(g_progName, MSGTYPE_ERR, hr,
                      L"Failed to initialize client.");
        }

        TExitMsg(("=%x", hr));
        return hr;
    }   //Initialize

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

        if (m_client != NULL)
        {
            hr = m_client->SyncWrite(pbBuff, dwcbLen, lpdwcb, dwTimeout);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_TARGET_HANDLE);
        }

        TExitMsg(("=%x", hr));
        return hr;
    }   //SendData

};  //class NetConn

