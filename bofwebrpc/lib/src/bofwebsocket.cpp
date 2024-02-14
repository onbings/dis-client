/*!
  Copyright (c) 2008, EVS. All rights reserved.

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND,  EITHER EXPRESSED OR IMPLIED,  INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  History:
    V 1.00  August 24 2013  BHA : Original version
*/
#include <bofstd/bofenum.h>
#include <bofstd/boffs.h>
#include <bofstd/bofsocket.h>

#include "bofwebrpc/bofwebsocket.h"

// https://apidog.com/blog/how-to-implement-websocket-_pInput-c/
// https://github.com/iamscottmoyers/simple-libwebsockets-example/blob/master/server.c
// https://github.com/iamscottmoyers/simple-libwebsockets-example/blob/master/client.c

BEGIN_WEBRPC_NAMESPACE()
constexpr const char *WS_CLT_KEY_ARG = "bof_ws_clt_key=";
constexpr const char *WS_CLT_KEY_VAL_FMT = "BofWsClt_%08X";
// #define WS_CLT_KEY_ARG "bof_ws_clt_key="
#define DBG_PROTO
#if defined(_WIN32)
#define BOF_LOG_TO_DBG(pFormat,...)  {std::string Dbg;Dbg=BOF::Bof_Sprintf(pFormat, __VA_ARGS__);OutputDebugString(Dbg.c_str());}
#else
#define BOF_LOG_TO_DBG(pFormat,...)  {printf(pFormat, __VA_ARGS__);}
#endif
#define DBG_LOG lwsl_info

#if defined(DBG_PROTO)
#define BOF_LWS_WRITE(Sts, Wsi, pBuf, Len, Proto)                       \
  {                                                                 \
    Sts = lws_write(Wsi, pBuf, Len, Proto);                         \
    /*BOF_LOG_TO_DBG("Write %zx:%p %s", Len, pBuf, pBuf ? (char *)pBuf : ""); */ \
  }
#else
#define BOF_LWS_WRITE(Sts, Wsi, pBuf, Len, Proto) \
  {                                           \
    Sts = lws_write(Wsi, pBuf, Len, Proto);   \
  }
#endif
constexpr uint32_t PUSH_POP_TIMEOUT = 150; // Global To for getting command out of incoming queue, in ListeningMode_B it is half of the To specified for listen
#define BOF_LWS_CLIENT_DISCONNECT()            \
  {                                        \
    if (mpLwsContext_X)                    \
    {                                      \
      lws_context_destroy(mpLwsContext_X); \
      mpLwsContext_X = nullptr;            \
    }                                      \
    mWebSocketState_X.Reset();             \
    mpuRxBufferCollection->Reset();        \
  }
#define BOF_LWS_WAKEUP_SERVICE()              \
  {                                       \
    if (mpLwsContext_X)                   \
    {                                     \
      lws_cancel_service(mpLwsContext_X); \
    }                                     \
  }

#define BOF_WEB_RPC_PROGRAM_OPERATION(Field, Operation)                                              \
  BOFERR Rts_E;                                                                                  \
  BOF_WEB_RPC_SOCKET_OPERATION_PARAM Param_X;                                                        \
  Param_X.Ticket_U32 = mTicket_U32;                                                              \
  BOF_INC_TICKET_NUMBER(mTicket_U32);                                                            \
  Param_X.TimeOut_U32 = _TimeOut_U32;                                                            \
  Param_X.Timer_U32 = BOF::Bof_GetMsTickCount();                                                 \
  Param_X.Operation_E = Operation;                                                               \
  Param_X.Field = _rParam_X;                                                                     \
  Rts_E = mpuSocketOperationParamCollection->Push(&Param_X, PUSH_POP_TIMEOUT, nullptr, nullptr); \
  if (Rts_E == BOF_ERR_NO_ERROR)                                                                 \
  {                                                                                              \
    BOF_LWS_WAKEUP_SERVICE();                                                                        \
    _rOpTicket_U32 = Param_X.Ticket_U32;                                                         \
    mOperationPending_B = true;                                                                  \
  }                                                                                              \
  else                                                                                           \
  {                                                                                              \
    _rOpTicket_U32 = 0;                                                                          \
  }                                                                                              \
  return Rts_E;

static BOF::BofEnum<BOF_WEB_RPC_SOCKET_OPERATION> S_WebRpcSocketOpEnumConverter(
    {
        {BOF_WEB_RPC_SOCKET_OPERATION_EXIT, "EXIT"},
        {BOF_WEB_RPC_SOCKET_OPERATION_LISTEN, "LISTEN"},
        {BOF_WEB_RPC_SOCKET_OPERATION_CONNECT, "CONNECT"},
        {BOF_WEB_RPC_SOCKET_OPERATION_READ, "READ"},
        {BOF_WEB_RPC_SOCKET_OPERATION_WRITE, "WRITE"},
        {BOF_WEB_RPC_SOCKET_OPERATION_DISCONNECT, "DISCONNECT"},
        {BOF_WEB_RPC_SOCKET_OPERATION_MAX, "MAX"},
    },
    BOF_WEB_RPC_SOCKET_OPERATION_MAX);

static BOF::BofEnum<BOF_WEB_SOCKET_LWS_CALLBACK_REASON> S_LwsCallbackReasonEnumConverter(
    {
        {LWS_CALLBACK_PROTOCOL_INIT, "LWS_CALLBACK_PROTOCOL_INIT"},
        {LWS_CALLBACK_PROTOCOL_DESTROY, "LWS_CALLBACK_PROTOCOL_DESTROY"},
        {LWS_CALLBACK_WSI_CREATE, "LWS_CALLBACK_WSI_CREATE"},
        {LWS_CALLBACK_WSI_DESTROY, "LWS_CALLBACK_WSI_DESTROY"},
        {LWS_CALLBACK_WSI_TX_CREDIT_GET, "LWS_CALLBACK_WSI_TX_CREDIT_GET"},
        {LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS, "LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS"},
        {LWS_CALLBACK_OPENSSL_LOAD_EXTRA_SERVER_VERIFY_CERTS, "LWS_CALLBACK_OPENSSL_LOAD_EXTRA_SERVER_VERIFY_CERTS"},
        {LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION, "LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION"},
        {LWS_CALLBACK_OPENSSL_CONTEXT_REQUIRES_PRIVATE_KEY, "LWS_CALLBACK_OPENSSL_CONTEXT_REQUIRES_PRIVATE_KEY"},
        {LWS_CALLBACK_SSL_INFO, "LWS_CALLBACK_SSL_INFO"},
        {LWS_CALLBACK_OPENSSL_PERFORM_SERVER_CERT_VERIFICATION, "LWS_CALLBACK_OPENSSL_PERFORM_SERVER_CERT_VERIFICATION"},
        {LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED, "LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED"},
        {LWS_CALLBACK_HTTP, "LWS_CALLBACK_HTTP"},
        {LWS_CALLBACK_HTTP_BODY, "LWS_CALLBACK_HTTP_BODY"},
        {LWS_CALLBACK_HTTP_BODY_COMPLETION, "LWS_CALLBACK_HTTP_BODY_COMPLETION"},
        {LWS_CALLBACK_HTTP_FILE_COMPLETION, "LWS_CALLBACK_HTTP_FILE_COMPLETION"},
        {LWS_CALLBACK_HTTP_WRITEABLE, "LWS_CALLBACK_HTTP_WRITEABLE"},
        {LWS_CALLBACK_CLOSED_HTTP, "LWS_CALLBACK_CLOSED_HTTP"},
        {LWS_CALLBACK_FILTER_HTTP_CONNECTION, "LWS_CALLBACK_FILTER_HTTP_CONNECTION"},
        {LWS_CALLBACK_ADD_HEADERS, "LWS_CALLBACK_ADD_HEADERS"},
        {LWS_CALLBACK_VERIFY_BASIC_AUTHORIZATION, "LWS_CALLBACK_VERIFY_BASIC_AUTHORIZATION"},
        {LWS_CALLBACK_CHECK_ACCESS_RIGHTS, "LWS_CALLBACK_CHECK_ACCESS_RIGHTS"},
        {LWS_CALLBACK_PROCESS_HTML, "LWS_CALLBACK_PROCESS_HTML"},
        {LWS_CALLBACK_HTTP_BIND_PROTOCOL, "LWS_CALLBACK_HTTP_BIND_PROTOCOL"},
        {LWS_CALLBACK_HTTP_DROP_PROTOCOL, "LWS_CALLBACK_HTTP_DROP_PROTOCOL"},
        {LWS_CALLBACK_HTTP_CONFIRM_UPGRADE, "LWS_CALLBACK_HTTP_CONFIRM_UPGRADE"},
        {LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP, "LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP"},
        {LWS_CALLBACK_CLOSED_CLIENT_HTTP, "LWS_CALLBACK_CLOSED_CLIENT_HTTP"},
        {LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ, "LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ"},
        {LWS_CALLBACK_RECEIVE_CLIENT_HTTP, "LWS_CALLBACK_RECEIVE_CLIENT_HTTP"},
        {LWS_CALLBACK_COMPLETED_CLIENT_HTTP, "LWS_CALLBACK_COMPLETED_CLIENT_HTTP"},
        {LWS_CALLBACK_CLIENT_HTTP_WRITEABLE, "LWS_CALLBACK_CLIENT_HTTP_WRITEABLE"},
        {LWS_CALLBACK_CLIENT_HTTP_REDIRECT, "LWS_CALLBACK_CLIENT_HTTP_REDIRECT"},
        {LWS_CALLBACK_CLIENT_HTTP_BIND_PROTOCOL, "LWS_CALLBACK_CLIENT_HTTP_BIND_PROTOCOL"},
        {LWS_CALLBACK_CLIENT_HTTP_DROP_PROTOCOL, "LWS_CALLBACK_CLIENT_HTTP_DROP_PROTOCOL"},
        {LWS_CALLBACK_ESTABLISHED, "LWS_CALLBACK_ESTABLISHED"},
        {LWS_CALLBACK_CLOSED, "LWS_CALLBACK_CLOSED"},
        {LWS_CALLBACK_SERVER_WRITEABLE, "LWS_CALLBACK_SERVER_WRITEABLE"},
        {LWS_CALLBACK_RECEIVE, "LWS_CALLBACK_RECEIVE"},
        {LWS_CALLBACK_RECEIVE_PONG, "LWS_CALLBACK_RECEIVE_PONG"},
        {LWS_CALLBACK_WS_PEER_INITIATED_CLOSE, "LWS_CALLBACK_WS_PEER_INITIATED_CLOSE"},
        {LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION, "LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION"},
        {LWS_CALLBACK_CONFIRM_EXTENSION_OKAY, "LWS_CALLBACK_CONFIRM_EXTENSION_OKAY"},
        {LWS_CALLBACK_WS_SERVER_BIND_PROTOCOL, "LWS_CALLBACK_WS_SERVER_BIND_PROTOCOL"},
        {LWS_CALLBACK_WS_SERVER_DROP_PROTOCOL, "LWS_CALLBACK_WS_SERVER_DROP_PROTOCOL"},
        {LWS_CALLBACK_CLIENT_CONNECTION_ERROR, "LWS_CALLBACK_CLIENT_CONNECTION_ERROR"},
        {LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH, "LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH"},
        {LWS_CALLBACK_CLIENT_ESTABLISHED, "LWS_CALLBACK_CLIENT_ESTABLISHED"},
        {LWS_CALLBACK_CLIENT_CLOSED, "LWS_CALLBACK_CLIENT_CLOSED"},
        {LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER, "LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER"},
        {LWS_CALLBACK_CLIENT_RECEIVE, "LWS_CALLBACK_CLIENT_RECEIVE"},
        {LWS_CALLBACK_CLIENT_RECEIVE_PONG, "LWS_CALLBACK_CLIENT_RECEIVE_PONG"},
        {LWS_CALLBACK_CLIENT_WRITEABLE, "LWS_CALLBACK_CLIENT_WRITEABLE"},
        {LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED, "LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED"},
        {LWS_CALLBACK_WS_EXT_DEFAULTS, "LWS_CALLBACK_WS_EXT_DEFAULTS"},
        {LWS_CALLBACK_FILTER_NETWORK_CONNECTION, "LWS_CALLBACK_FILTER_NETWORK_CONNECTION"},
        {LWS_CALLBACK_WS_CLIENT_BIND_PROTOCOL, "LWS_CALLBACK_WS_CLIENT_BIND_PROTOCOL"},
        {LWS_CALLBACK_WS_CLIENT_DROP_PROTOCOL, "LWS_CALLBACK_WS_CLIENT_DROP_PROTOCOL"},
        {LWS_CALLBACK_GET_THREAD_ID, "LWS_CALLBACK_GET_THREAD_ID"},
        {LWS_CALLBACK_ADD_POLL_FD, "LWS_CALLBACK_ADD_POLL_FD"},
        {LWS_CALLBACK_DEL_POLL_FD, "LWS_CALLBACK_DEL_POLL_FD"},
        {LWS_CALLBACK_CHANGE_MODE_POLL_FD, "LWS_CALLBACK_CHANGE_MODE_POLL_FD"},
        {LWS_CALLBACK_LOCK_POLL, "LWS_CALLBACK_LOCK_POLL"},
        {LWS_CALLBACK_UNLOCK_POLL, "LWS_CALLBACK_UNLOCK_POLL"},
        {LWS_CALLBACK_CGI, "LWS_CALLBACK_CGI"},
        {LWS_CALLBACK_CGI_TERMINATED, "LWS_CALLBACK_CGI_TERMINATED"},
        {LWS_CALLBACK_CGI_STDIN_DATA, "LWS_CALLBACK_CGI_STDIN_DATA"},
        {LWS_CALLBACK_CGI_STDIN_COMPLETED, "LWS_CALLBACK_CGI_STDIN_COMPLETED"},
        {LWS_CALLBACK_CGI_PROCESS_ATTACH, "LWS_CALLBACK_CGI_PROCESS_ATTACH"},
        {LWS_CALLBACK_SESSION_INFO, "LWS_CALLBACK_SESSION_INFO"},
        {LWS_CALLBACK_GS_EVENT, "LWS_CALLBACK_GS_EVENT"},
        {LWS_CALLBACK_HTTP_PMO, "LWS_CALLBACK_HTTP_PMO"},
        {LWS_CALLBACK_RAW_PROXY_CLI_RX, "LWS_CALLBACK_RAW_PROXY_CLI_RX"},
        {LWS_CALLBACK_RAW_PROXY_SRV_RX, "LWS_CALLBACK_RAW_PROXY_SRV_RX"},
        {LWS_CALLBACK_RAW_PROXY_CLI_CLOSE, "LWS_CALLBACK_RAW_PROXY_CLI_CLOSE"},
        {LWS_CALLBACK_RAW_PROXY_SRV_CLOSE, "LWS_CALLBACK_RAW_PROXY_SRV_CLOSE"},
        {LWS_CALLBACK_RAW_PROXY_CLI_WRITEABLE, "LWS_CALLBACK_RAW_PROXY_CLI_WRITEABLE"},
        {LWS_CALLBACK_RAW_PROXY_SRV_WRITEABLE, "LWS_CALLBACK_RAW_PROXY_SRV_WRITEABLE"},
        {LWS_CALLBACK_RAW_PROXY_CLI_ADOPT, "LWS_CALLBACK_RAW_PROXY_CLI_ADOPT"},
        {LWS_CALLBACK_RAW_PROXY_SRV_ADOPT, "LWS_CALLBACK_RAW_PROXY_SRV_ADOPT"},
        {LWS_CALLBACK_RAW_PROXY_CLI_BIND_PROTOCOL, "LWS_CALLBACK_RAW_PROXY_CLI_BIND_PROTOCOL"},
        {LWS_CALLBACK_RAW_PROXY_SRV_BIND_PROTOCOL, "LWS_CALLBACK_RAW_PROXY_SRV_BIND_PROTOCOL"},
        {LWS_CALLBACK_RAW_PROXY_CLI_DROP_PROTOCOL, "LWS_CALLBACK_RAW_PROXY_CLI_DROP_PROTOCOL"},
        {LWS_CALLBACK_RAW_PROXY_SRV_DROP_PROTOCOL, "LWS_CALLBACK_RAW_PROXY_SRV_DROP_PROTOCOL"},
        {LWS_CALLBACK_RAW_RX, "LWS_CALLBACK_RAW_RX"},
        {LWS_CALLBACK_RAW_CLOSE, "LWS_CALLBACK_RAW_CLOSE"},
        {LWS_CALLBACK_RAW_WRITEABLE, "LWS_CALLBACK_RAW_WRITEABLE"},
        {LWS_CALLBACK_RAW_ADOPT, "LWS_CALLBACK_RAW_ADOPT"},
        {LWS_CALLBACK_RAW_CONNECTED, "LWS_CALLBACK_RAW_CONNECTED"},
        {LWS_CALLBACK_RAW_SKT_BIND_PROTOCOL, "LWS_CALLBACK_RAW_SKT_BIND_PROTOCOL"},
        {LWS_CALLBACK_RAW_SKT_DROP_PROTOCOL, "LWS_CALLBACK_RAW_SKT_DROP_PROTOCOL"},
        {LWS_CALLBACK_RAW_ADOPT_FILE, "LWS_CALLBACK_RAW_ADOPT_FILE"},
        {LWS_CALLBACK_RAW_RX_FILE, "LWS_CALLBACK_RAW_RX_FILE"},
        {LWS_CALLBACK_RAW_WRITEABLE_FILE, "LWS_CALLBACK_RAW_WRITEABLE_FILE"},
        {LWS_CALLBACK_RAW_CLOSE_FILE, "LWS_CALLBACK_RAW_CLOSE_FILE"},
        {LWS_CALLBACK_RAW_FILE_BIND_PROTOCOL, "LWS_CALLBACK_RAW_FILE_BIND_PROTOCOL"},
        {LWS_CALLBACK_RAW_FILE_DROP_PROTOCOL, "LWS_CALLBACK_RAW_FILE_DROP_PROTOCOL"},
        {LWS_CALLBACK_TIMER, "LWS_CALLBACK_TIMER"},
        {LWS_CALLBACK_EVENT_WAIT_CANCELLED, "LWS_CALLBACK_EVENT_WAIT_CANCELLED"},
        {LWS_CALLBACK_CHILD_CLOSING, "LWS_CALLBACK_CHILD_CLOSING"},
        {LWS_CALLBACK_CONNECTING, "LWS_CALLBACK_CONNECTING"},
        {LWS_CALLBACK_VHOST_CERT_AGING, "LWS_CALLBACK_VHOST_CERT_AGING"},
        {LWS_CALLBACK_VHOST_CERT_UPDATE, "LWS_CALLBACK_VHOST_CERT_UPDATE"},
        {LWS_CALLBACK_MQTT_NEW_CLIENT_INSTANTIATED, "LWS_CALLBACK_MQTT_NEW_CLIENT_INSTANTIATED"},
        {LWS_CALLBACK_MQTT_IDLE, "LWS_CALLBACK_MQTT_IDLE"},
        {LWS_CALLBACK_MQTT_CLIENT_ESTABLISHED, "LWS_CALLBACK_MQTT_CLIENT_ESTABLISHED"},
        {LWS_CALLBACK_MQTT_SUBSCRIBED, "LWS_CALLBACK_MQTT_SUBSCRIBED"},
        {LWS_CALLBACK_MQTT_CLIENT_WRITEABLE, "LWS_CALLBACK_MQTT_CLIENT_WRITEABLE"},
        {LWS_CALLBACK_MQTT_CLIENT_RX, "LWS_CALLBACK_MQTT_CLIENT_RX"},
        {LWS_CALLBACK_MQTT_UNSUBSCRIBED, "LWS_CALLBACK_MQTT_UNSUBSCRIBED"},
        {LWS_CALLBACK_MQTT_DROP_PROTOCOL, "LWS_CALLBACK_MQTT_DROP_PROTOCOL"},
        {LWS_CALLBACK_MQTT_CLIENT_CLOSED, "LWS_CALLBACK_MQTT_CLIENT_CLOSED"},
        {LWS_CALLBACK_MQTT_ACK, "LWS_CALLBACK_MQTT_ACK"},
        {LWS_CALLBACK_MQTT_RESEND, "LWS_CALLBACK_MQTT_RESEND"},
        {LWS_CALLBACK_MQTT_UNSUBSCRIBE_TIMEOUT, "LWS_CALLBACK_MQTT_UNSUBSCRIBE_TIMEOUT"},
        {LWS_CALLBACK_USER, "LWS_CALLBACK_USER"},
    },
    LWS_CALLBACK_USER);

uint32_t BofWebSocket::S_mWsCltId_U32 = 1;

static int S_Lws_Callback_Http(struct lws *_pWsi_X, enum lws_callback_reasons _LwsReason_E, void *_pLwsUser, void *_pInput, size_t _Len)
{
  int Rts_i;
  struct lws_context *pContext_X;
  BofWebSocket *pWebSocket;

  /* This will be same for every connected peer */
  pContext_X = lws_get_context(_pWsi_X);
  pWebSocket = (BofWebSocket *)lws_context_user(pContext_X);
  Rts_i = pWebSocket ? pWebSocket->LwsCallback(_pWsi_X, _LwsReason_E, _pLwsUser, _pInput, _Len) : -1;

  return Rts_i;
}

// Set up HTTP mount for serving static files
struct lws_http_mount S_MountPoint_X = {
    nullptr,
    "/",
    "./",         // Serve files from the current directory
    "index.html", // Default file to serve
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    0,
    0,
    0,
    0,
    0,
    0,
    LWSMPRO_FILE,
    1,
    nullptr,
};

// This array of struct will be completed before calling lws_create_context to setup per_session_data_size and user
/* The first protocol must always be the HTTP handler */
static struct lws_protocols S_pLwsServerProtocol_X[] = {
    {"http",
     S_Lws_Callback_Http,
     0, // per session data size
     0,
     0,       // user defined ID, not used by LWS
     nullptr, // user defined pointer
     0},
    {"webrpc",
     S_Lws_Callback_Http,
     0, // per session data size
     0,
     0,       // user defined ID, not used by LWS
     nullptr, // user defined pointer
     0},
    {NULL, NULL, 0, 0} // End of list
};
static struct lws_protocols S_pLwsClientProtocol_X[] = {
    {"webrpc",
     S_Lws_Callback_Http,
     0, // per session data size
     0,
     0,       // user defined ID, not used by LWS
     nullptr, // user defined pointer
     0},

    {NULL, NULL, 0, 0} // End of list
};
BofWebSocket::BofWebSocket(const BOF_WEB_SOCKET_PARAM &_rWebSocketParam_X)
    : BOF::BofThread()
{
  uint32_t Start_U32;
  BOF::BOF_CIRCULAR_BUFFER_PARAM CircularBufferParam_X;
  BOF::BOF_RAW_CIRCULAR_BUFFER_PARAM RawCircularBufferParam_X;
  BOFERR Sts_E;
  bool Sts_B;

  mWebSocketParam_X = _rWebSocketParam_X;
  // lws_set_log_level(LLLF_LOG_TIMESTAMP | LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG | LLL_PARSER | LLL_HEADER | LLL_EXT | LLL_CLIENT | LLL_LATENCY | LLL_USER | LLL_THREAD, nullptr);
  lws_set_log_level(LLLF_LOG_TIMESTAMP | LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_HEADER | LLL_EXT | LLL_CLIENT | LLL_LATENCY | LLL_USER | LLL_THREAD, nullptr);
  //                    LLL_DEBUG | LLL_INFO | LLL_NOTICE | LLL_WARN | LLL_ERR | LLL_CLIENT | LLL_LATENCY, nullptr);  //);
  lws_set_log_level(0, nullptr);
  Sts_E = Bof_CreateEvent(mWebSocketParam_X.WebSocketThreadParam_X.Name_S, false, 1, false, false, mCancelEvent_X);
  BOF_ASSERT(Sts_E == BOF_ERR_NO_ERROR);

  CircularBufferParam_X.MultiThreadAware_B = true;
  CircularBufferParam_X.NbMaxElement_U32 = mWebSocketParam_X.NbMaxOperationPending_U32;
  CircularBufferParam_X.pData = nullptr;
  CircularBufferParam_X.Overwrite_B = false;
  CircularBufferParam_X.Blocking_B = true;
  CircularBufferParam_X.PopLockMode_B = false;
  mpuSocketOperationParamCollection = std::make_unique<BOF::BofCircularBuffer<BOF_WEB_RPC_SOCKET_OPERATION_PARAM>>(CircularBufferParam_X);
  if ((mpuSocketOperationParamCollection) && (mpuSocketOperationParamCollection->LastErrorCode() == BOF_ERR_NO_ERROR))
  {
    CircularBufferParam_X.MultiThreadAware_B = true;
    CircularBufferParam_X.NbMaxElement_U32 = mWebSocketParam_X.NbMaxOperationPending_U32 + 1;
    CircularBufferParam_X.pData = nullptr;
    CircularBufferParam_X.Overwrite_B = false;
    CircularBufferParam_X.Blocking_B = true;
    CircularBufferParam_X.PopLockMode_B = false;
    mpuSocketOperationWriteOpCollection = std::make_unique<BOF::BofCircularBuffer<BOF_WEB_RPC_SOCKET_WRITE_OP>>(CircularBufferParam_X);
    if ((mpuSocketOperationWriteOpCollection) && (mpuSocketOperationWriteOpCollection->LastErrorCode() == BOF_ERR_NO_ERROR))
    {
      mpuSocketOperationResultCollection = std::make_unique<BOF::BofCircularBuffer<BOF_WEB_RPC_SOCKET_OPERATION_RESULT>>(CircularBufferParam_X);
      if ((mpuSocketOperationResultCollection) && (mpuSocketOperationParamCollection->LastErrorCode() == BOF_ERR_NO_ERROR))
      {
        RawCircularBufferParam_X.MultiThreadAware_B = true;
        RawCircularBufferParam_X.BufferSizeInByte_U32 = mWebSocketParam_X.RxBufferSize_U32;
        RawCircularBufferParam_X.SlotMode_B = false;
        RawCircularBufferParam_X.AlwaysContiguous_B = true; // false;
        RawCircularBufferParam_X.NbMaxBufferEntry_U32 = mWebSocketParam_X.NbMaxBufferEntry_U32;
        RawCircularBufferParam_X.pData_U8 = nullptr;
        RawCircularBufferParam_X.Overwrite_B = false;
        RawCircularBufferParam_X.Blocking_B = true;
        mpuRxBufferCollection = std::make_unique<BOF::BofRawCircularBuffer>(RawCircularBufferParam_X);
        if ((mpuRxBufferCollection) && (mpuRxBufferCollection->LastErrorCode() == BOF_ERR_NO_ERROR))
        {
        }
      }
    }
  }
}
BofWebSocket::~BofWebSocket()
{
  Stop();
}
BOFERR BofWebSocket::ProgramWebSocketOperation(uint32_t _TimeOut_U32, BOF_WEB_RPC_SOCKET_EXIT_PARAM &_rParam_X, uint32_t &_rOpTicket_U32)
{
  BOF_WEB_RPC_PROGRAM_OPERATION(OpParam.Exit_X, BOF_WEB_RPC_SOCKET_OPERATION::BOF_WEB_RPC_SOCKET_OPERATION_EXIT);
}
BOFERR BofWebSocket::ProgramWebSocketOperation(uint32_t _TimeOut_U32, BOF_WEB_RPC_SOCKET_LISTEN_PARAM &_rParam_X, uint32_t &_rOpTicket_U32)
{
  BOF_WEB_RPC_PROGRAM_OPERATION(OpParam.Listen_X, BOF_WEB_RPC_SOCKET_OPERATION::BOF_WEB_RPC_SOCKET_OPERATION_LISTEN);
}
BOFERR BofWebSocket::ProgramWebSocketOperation(uint32_t _TimeOut_U32, BOF_WEB_RPC_SOCKET_CONNECT_PARAM &_rParam_X, uint32_t &_rOpTicket_U32)
{
  BOF_WEB_RPC_PROGRAM_OPERATION(OpParam.Connect_X, BOF_WEB_RPC_SOCKET_OPERATION::BOF_WEB_RPC_SOCKET_OPERATION_CONNECT);
}
BOFERR BofWebSocket::ProgramWebSocketOperation(uint32_t _TimeOut_U32, BOF_WEB_RPC_SOCKET_READ_PARAM &_rParam_X, uint32_t &_rOpTicket_U32)
{
  BOF_WEB_RPC_PROGRAM_OPERATION(OpParam.Read_X, BOF_WEB_RPC_SOCKET_OPERATION::BOF_WEB_RPC_SOCKET_OPERATION_READ);
}
BOFERR BofWebSocket::ProgramWebSocketOperation(uint32_t _TimeOut_U32, BOF_WEB_RPC_SOCKET_WRITE_PARAM &_rParam_X, uint32_t &_rOpTicket_U32)
{
  BOF_WEB_RPC_PROGRAM_OPERATION(OpParam.Write_X, BOF_WEB_RPC_SOCKET_OPERATION::BOF_WEB_RPC_SOCKET_OPERATION_WRITE);
}
BOFERR BofWebSocket::ProgramWebSocketOperation(uint32_t _TimeOut_U32, BOF_WEB_RPC_SOCKET_DISCONNECT_PARAM &_rParam_X, uint32_t &_rOpTicket_U32)
{
  BOF_WEB_RPC_PROGRAM_OPERATION(OpParam.Disconnect_X, BOF_WEB_RPC_SOCKET_OPERATION::BOF_WEB_RPC_SOCKET_OPERATION_DISCONNECT);
}
BOFERR BofWebSocket::GetWebSocketOperationResult(uint32_t _TimeOut_U32, BOF_WEB_RPC_SOCKET_OPERATION_RESULT &_rOperationResult_X)
{
  BOFERR Rts_E;

  _rOperationResult_X.Reset();
  Rts_E = mpuSocketOperationResultCollection->Pop(&_rOperationResult_X, _TimeOut_U32, nullptr, nullptr);
  // BOF_LOG_TO_DBG("%d WEB_RPC_SOCKET_OPERATION_GET_RESULT %s Err %s Ticket %d Op %s Sts %s\n", BOF::Bof_GetMsTickCount(), mWebSocketParam_X.WebSocketThreadParam_X.Name_S.c_str(), BOF::Bof_ErrorCode(Rts_E), _rOperationResult_X.OpTicket_U32, S_WebRpcSocketOpEnumConverter.ToString(_rOperationResult_X.Operation_E).c_str(), BOF::Bof_ErrorCode(_rOperationResult_X.Sts_E));
  return Rts_E;
}

uint32_t BofWebSocket::NumberOfWebSocketOperationWaiting()
{
  // DBGLOG("NumberOfOperationPending %d\n", mpuSocketOperationParamCollection->GetNbElement());
  return mpuSocketOperationParamCollection->GetNbElement();
}
bool BofWebSocket::IsWebSocketOperationPending()
{
  // DBGLOG("NumberOfOperationPending %d\n", mpuSocketOperationParamCollection->GetNbElement());
  return mOperationPending_B; // only cleared by ClearSocketOperation or CancelSocketOperation
}
uint32_t BofWebSocket::NumberOfWebSocketResultPending()
{
  // DBGLOG("NumberOfResultPending %d\n", mpuSocketOperationResultCollection->GetNbElement());
  return mpuSocketOperationResultCollection->GetNbElement();
}
BOFERR BofWebSocket::CancelWebSocketOperation(uint32_t _TimeOut_U32)
{
  BOFERR Rts_E = BOF_ERR_PENDING;

  if (!mCancel_B)
  {
    mCancel_B = true;
    if (_TimeOut_U32)
    {
      Rts_E = BOF::Bof_WaitForEvent(mCancelEvent_X, _TimeOut_U32, 0);
    }
    else
    {
      Rts_E = BOF_ERR_NO_ERROR;
    }
    if (Rts_E == BOF_ERR_NO_ERROR)
    {
      mOperationPending_B = false;
    }
  }
  return Rts_E;
}

BOFERR BofWebSocket::ClearWebSocketOperation()
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;

  mOperationPending_B = false;
  mCancel_B = false;
  Bof_SetEventMask(mCancelEvent_X, 0);
  return Rts_E;
}

BOFERR BofWebSocket::ResetWebSocketOperation()
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;

  mpuSocketOperationParamCollection->Reset();
  mpuSocketOperationResultCollection->Reset();
  ClearWebSocketOperation();

  return Rts_E;
}
BOFERR BofWebSocket::Connect(uint32_t TimeoutInMs_U32, const std::string &_rWsEndpoint_S, const std::string &_rWsCltKey_S)  // "ws ://10.129.171.112:8080";
{
  BOFERR Rts_E;
  BOF::BOF_SOCKET_ADDRESS WsEndpoint_X(_rWsEndpoint_S);
  uint32_t OpTicket_U32;
  BOFWEBRPC::BOF_WEB_RPC_SOCKET_OPERATION_RESULT OperationResult_X;
  BOFWEBRPC::BOF_WEB_RPC_SOCKET_CONNECT_PARAM ConnectParam_X;

  ConnectParam_X.DstIpAddr_X = BOF::BOF_IPV4_ADDR_U32(WsEndpoint_X);
  ConnectParam_X.DstPort_U16 = WsEndpoint_X.Port();
  BOF_SNPRINTF_NULL_CLIPPED(ConnectParam_X.pWsCltKey_c, sizeof(ConnectParam_X.pWsCltKey_c), "%s", _rWsCltKey_S.c_str());
  Rts_E = ProgramWebSocketOperation(TimeoutInMs_U32, ConnectParam_X, OpTicket_U32);
  if (Rts_E == BOF_ERR_NO_ERROR)
  {
    do
    {
      Rts_E = GetWebSocketOperationResult(TimeoutInMs_U32, OperationResult_X);
      if (Rts_E == BOF_ERR_NO_ERROR)
      {
        if (OperationResult_X.OpTicket_U32 == OpTicket_U32)
        {
          break;
        }
      }
    }
    while (Rts_E == BOF_ERR_NO_ERROR);
  }
  return Rts_E;
}
BOFERR BofWebSocket::Write(uint32_t TimeoutInMs_U32, uint32_t &_rNbToWrite_U32, uint8_t *_pData_U8)
{
  BOFERR Rts_E;
  uint32_t OpTicket_U32;
  BOFWEBRPC::BOF_WEB_RPC_SOCKET_OPERATION_RESULT OperationResult_X;
  BOFWEBRPC::BOF_WEB_RPC_SOCKET_WRITE_PARAM WriteParam_X;

  WriteParam_X.pBuffer_U8 = _pData_U8;
  WriteParam_X.Nb_U32 = _rNbToWrite_U32;
  _rNbToWrite_U32 = 0;
  Rts_E = ProgramWebSocketOperation(TimeoutInMs_U32, WriteParam_X, OpTicket_U32);
  if (Rts_E == BOF_ERR_NO_ERROR)
  {
    do
    {
      Rts_E = GetWebSocketOperationResult(TimeoutInMs_U32, OperationResult_X);
      if (Rts_E == BOF_ERR_NO_ERROR)
      {
        if (OperationResult_X.OpTicket_U32 == OpTicket_U32)
        {
          _rNbToWrite_U32 = OperationResult_X.Size_U32;
          break;
        }
      }
    }
    while (Rts_E == BOF_ERR_NO_ERROR);
  }
  return Rts_E;
}

BOFERR BofWebSocket::Read(uint32_t TimeoutInMs_U32, uint32_t &_rNbMaxToRead_U32, uint8_t *_pData_U8)
{
  BOFERR Rts_E;
  uint32_t OpTicket_U32;
  BOFWEBRPC::BOF_WEB_RPC_SOCKET_OPERATION_RESULT OperationResult_X;
  BOFWEBRPC::BOF_WEB_RPC_SOCKET_READ_PARAM ReadParam_X;

  ReadParam_X.pBuffer_U8 = _pData_U8;
  ReadParam_X.Nb_U32 = _rNbMaxToRead_U32;
  _rNbMaxToRead_U32 = 0;
  Rts_E = ProgramWebSocketOperation(TimeoutInMs_U32, ReadParam_X, OpTicket_U32);
  if (Rts_E == BOF_ERR_NO_ERROR)
  {
    do
    {
      Rts_E = GetWebSocketOperationResult(TimeoutInMs_U32, OperationResult_X);
      if (Rts_E == BOF_ERR_NO_ERROR)
      {
        if (OperationResult_X.OpTicket_U32 == OpTicket_U32)
        {
          _rNbMaxToRead_U32 = OperationResult_X.Size_U32;
          break;
        }
      }
    }
    while (Rts_E == BOF_ERR_NO_ERROR);
  }
  return Rts_E;
}
BOFERR BofWebSocket::Disconnect(uint32_t TimeoutInMs_U32)
{
  BOFERR Rts_E;
  uint32_t OpTicket_U32;
  BOFWEBRPC::BOF_WEB_RPC_SOCKET_OPERATION_RESULT OperationResult_X;
  BOFWEBRPC::BOF_WEB_RPC_SOCKET_DISCONNECT_PARAM DisconnectParam_X;

  DisconnectParam_X.Unused_U32 = 0;
  Rts_E = ProgramWebSocketOperation(TimeoutInMs_U32, DisconnectParam_X, OpTicket_U32);
  if (Rts_E == BOF_ERR_NO_ERROR)
  {
    do
    {
      Rts_E = GetWebSocketOperationResult(TimeoutInMs_U32, OperationResult_X);
      if (Rts_E == BOF_ERR_NO_ERROR)
      {
        if (OperationResult_X.OpTicket_U32 == OpTicket_U32)
        {
          break;
        }
      }
    }
    while (Rts_E == BOF_ERR_NO_ERROR);
  }
  return Rts_E;
}

BOFERR BofWebSocket::Run()
{
  BOFERR Rts_E;
  bool Sts_B;
  uint32_t Start_U32, OpTicket_U32;
  BOFWEBRPC::BOF_WEB_RPC_SOCKET_LISTEN_PARAM ListenParam_X;

  Rts_E = LaunchBofProcessingThread(mWebSocketParam_X.WebSocketThreadParam_X.Name_S, false, 0, mWebSocketParam_X.WebSocketThreadParam_X.ThreadSchedulerPolicy_E, mWebSocketParam_X.WebSocketThreadParam_X.ThreadPriority_E, 0, 2000, 0);
  BOF_ASSERT(Rts_E == BOF_ERR_NO_ERROR);
  Sts_B = IsThreadRunning(100);
  BOF_ASSERT(Sts_B);
  Start_U32 = BOF::Bof_GetMsTickCount();
  while (1)
  {
    if (mpLwsContext_X)
    {
      break;
    }
    if (BOF::Bof_ElapsedMsTime(Start_U32) >= 500)
    {
      break;
    }
    BOF::Bof_MsSleep(0);
  }
  if (Rts_E == BOF_ERR_NO_ERROR)
  {
    if (mWebSocketParam_X.NbMaxClient_U32)
    {
      ListenParam_X.NbMaxClient_U32 = mWebSocketParam_X.NbMaxClient_U32;
      ListenParam_X.SrcIpAddr_X = mWebSocketParam_X.ServerIp_X.ToBinary();
      ListenParam_X.SrcPort_U16 = mWebSocketParam_X.ServerIp_X.Port();
      Rts_E = ProgramWebSocketOperation(2000, ListenParam_X, OpTicket_U32);
    }
  }
  return Rts_E;
}
BOFERR BofWebSocket::Stop()
{
  BOFERR Rts_E;
  BOF_WEB_RPC_SOCKET_EXIT_PARAM Param_X;
  uint32_t OpTicket_U32;
  // First this and after destructor of BofThread
  // DBGLOG("%d: ProgramSocketOperation exit cmd\n", BOF::Bof_GetMsTickCount());
  Rts_E = ProgramWebSocketOperation(PUSH_POP_TIMEOUT, Param_X, OpTicket_U32);
  if (Rts_E != BOF_ERR_NO_ERROR)
  {
    // DBGLOG("%d: ProgramSocketOperation rts %d\n", BOF::Bof_GetMsTickCount(), e);
  }
  // Must kill the thread from here and not in the BofThread destructor because between both destructor, all mem var of BofSocketThread will disappear (unique pointer)
  DestroyBofProcessingThread("~WebSocket");
  BOF_LWS_CLIENT_DISCONNECT();
  return Rts_E;
}
bool BofWebSocket::IsClientExist(const std::string &_rWsCltKey_S)
{
  bool Rts_B = false;

  // BOF_LOG_TO_DBG("IsClient %s exist in \n%s\n", _rWsCltKey_S.c_str(), GetClientList().c_str());

  std::lock_guard<std::mutex> Lock(mMtxClientCollection);
  auto It = mClientCollection.find(_rWsCltKey_S);
  // BOF_LOG_TO_DBG("%s EXIST %s\n", _rWsCltKey_S.c_str(), (It == mClientCollection.end()) ? "No" : "Yes");
  if (It != mClientCollection.end())
  {
    Rts_B = true;
  }
  return Rts_B;
}

BOFERR BofWebSocket::AddClient(const std::string &_rWsCltKey_S, BOF_WEB_SOCKET_DATA_PER_SESSION *_pWebSocketDataPerSession_X)
{
  BOFERR Rts_E = BOF_ERR_EINVAL;

  if (_pWebSocketDataPerSession_X)
  {
    BOF_ASSERT(_pWebSocketDataPerSession_X->pWebSocket_X != nullptr);
    Rts_E = BOF_ERR_NO_MORE;
    // BOF_LOG_TO_DBG("Check if %s exist in \n%s\n", _rWsCltKey_S.c_str(), GetClientList().c_str());

    std::lock_guard<std::mutex> Lock(mMtxClientCollection);
    if (mClientCollection.size() < mWebSocketParam_X.NbMaxClient_U32)
    {
      auto It = mClientCollection.find(_rWsCltKey_S);
      // BOF_LOG_TO_DBG("%s EXIST %s\n", _rWsCltKey_S.c_str(), (It == mClientCollection.end()) ? "No" : "Yes");
      if (It == mClientCollection.end())
      {
        _pWebSocketDataPerSession_X->ConnectDone_B = true;
        mClientCollection[_rWsCltKey_S] = *_pWebSocketDataPerSession_X;
        Rts_E = BOF_ERR_NO_ERROR;
      }
      else
      {
        _pWebSocketDataPerSession_X->ConnectDone_B = false;
        Rts_E = BOF_ERR_ALREADY_OPENED;
      }
    }
  }
  return Rts_E;
}
BOFERR BofWebSocket::RemoveClient(const std::string &_rWsCltKey_S)
{
  BOFERR Rts_E = BOF_ERR_NOT_FOUND;

  std::lock_guard<std::mutex> Lock(mMtxClientCollection);
  auto It = mClientCollection.find(_rWsCltKey_S);
  if (It != mClientCollection.end())
  {
    mClientCollection.erase(It);
    Rts_E = BOF_ERR_NO_ERROR;
  }
  return Rts_E;
}

std::string BofWebSocket::GetClientList()
{
  std::string Rts_S = "";

  std::lock_guard<std::mutex> Lock(mMtxClientCollection);
  for (auto &rIt : mClientCollection)
  {
    Rts_S = Rts_S + rIt.first + ": Cnt " + std::to_string(rIt.second.ConnectDone_B) + '\n';
  }
  return Rts_S;
}
BOFERR BofWebSocket::RemoveClient(BOF_WEB_SOCKET_DATA_PER_SESSION *_pWebSocketDataPerSession_X)
{
  BOFERR Rts_E = BOF_ERR_EINVAL;

  if (_pWebSocketDataPerSession_X)
  {
    Rts_E = BOF_ERR_NOT_FOUND;
    std::lock_guard<std::mutex> Lock(mMtxClientCollection);
    for (auto &rIt : mClientCollection)
    {
      if (rIt.second.pWebSocket_X == _pWebSocketDataPerSession_X->pWebSocket_X)
      {
        _pWebSocketDataPerSession_X->ConnectDone_B = false;
        mClientCollection.erase(rIt.first);
        Rts_E = BOF_ERR_NO_ERROR;
        break;
      }
    }
  }
  return Rts_E;
}
uint32_t BofWebSocket::GetNumberOfClient()
{
  return mClientCollection.size();
}

BOFERR BofWebSocket::GetCloseInfo(struct lws *_pWsi_X, uint16_t &_rCloseStatus_U16, uint32_t &_rMaxSize_U32, char *_pCloseInfo_c)
{
  BOFERR Rts_E = BOF_ERR_NOT_AVAILABLE;
  const unsigned char *pCloseInfo_c;
  size_t CloseInfoLen;

  // Retrieve the close information (reason) if available
  // It's important to note that the close payload returned by lws_get_close_payload includes the status code as the first two bytes.
  pCloseInfo_c = lws_get_close_payload(_pWsi_X);
  _rCloseStatus_U16 = 0;
  if (pCloseInfo_c)
  {
    CloseInfoLen = lws_get_close_length(_pWsi_X);
    // Process or log the close information
    if (CloseInfoLen >= 2)
    {
      _rCloseStatus_U16 = *(uint16_t *)pCloseInfo_c;
      pCloseInfo_c += sizeof(uint16_t);
      CloseInfoLen -= sizeof(uint16_t);
      if (CloseInfoLen <= _rMaxSize_U32)
      {
        strcpy(_pCloseInfo_c, (const char *)pCloseInfo_c);
        Rts_E = BOF_ERR_NO_ERROR;
      }
      else
      {
        Rts_E = BOF_ERR_TOO_SMALL;
      }
      _rMaxSize_U32 = CloseInfoLen;
    }
  }
  return Rts_E;
}
BOFERR BofWebSocket::SetSocketBufferSize(struct lws *_pWsi_X, uint32_t &_rRcvBufferSize_U32, uint32_t &_rSndBufferSize_U32)
{
  BOFERR Rts_E = BOF_ERR_NOT_AVAILABLE;
  BOF::BOFSOCKET WebSocketHandle;
  uint32_t RcvBufferSize_U32, SndBufferSize_U32;

  WebSocketHandle = lws_get_socket_fd(_pWsi_X);
  if (WebSocketHandle >= 0)
  {
    RcvBufferSize_U32 = 0;
    SndBufferSize_U32 = 0;
    BOF::BofSocket::S_SetSocketBufferSize(WebSocketHandle, RcvBufferSize_U32, SndBufferSize_U32);
    if (_rRcvBufferSize_U32)
    {
      RcvBufferSize_U32 = _rRcvBufferSize_U32;
    }
    if (_rSndBufferSize_U32)
    {
      SndBufferSize_U32 = _rSndBufferSize_U32;
    }
    Rts_E = BOF::BofSocket::S_SetSocketBufferSize(WebSocketHandle, RcvBufferSize_U32, SndBufferSize_U32);
#if defined(DBG_PROTO)
    BOF_LOG_TO_DBG("SetSocketBufferSize pWsi %p hndl %zd rcv %x snd %X sts %d\n", _pWsi_X, (uint64_t)WebSocketHandle, RcvBufferSize_U32, SndBufferSize_U32, Rts_E);
#endif
  }
  return Rts_E;
}
/*
In libwebsockets, the return value from your protocol callback function (LWS_CALLBACK_PROTOCOL) depends on the specific callback reason. The protocol callback is responsible for handling various events in the WebSocket lifecycle. Here are some common return values for different reasons:

LWS_CALLBACK_ESTABLISHED:

Return 0 to continue processing the connection.
You might perform any setup related to the newly established connection in this callback.
LWS_CALLBACK_RECEIVE:

Return 0 to continue processing the connection.
This is where you handle incoming data from the WebSocket connection.
LWS_CALLBACK_CLOSED:

Return 0 to continue processing the connection.
Cleanup or perform any necessary actions when the connection is closed.
LWS_CALLBACK_HTTP:

Return 0 to indicate that the HTTP processing is complete.
You may return a value other than zero to signal libwebsockets to not complete the HTTP processing, allowing you to handle it further. For example, you might return 1 to indicate that your application has responded to an HTTP request, and libwebsockets should not send a default response.
LWS_CALLBACK_SERVER_WRITEABLE:

Return 0 to continue processing the connection.
This callback is invoked when the socket is ready for writing.
LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:

Return 0 to accept the connection.
Return 1 to reject the connection.
LWS_CALLBACK_PROTOCOL_DESTROY:

Return 0 to continue processing the destruction of the protocol.
Other Callback Reasons:

For other callback reasons, refer to the libwebsockets documentation for the specific return values expected for each reason.
*/

int BofWebSocket::LwsCallback(struct lws *_pWsi_X, enum lws_callback_reasons _LwsReason_E, void *_pLwsUser, void *_pInput, size_t _Len)
{
  BOFERR Sts_E;
  int Rts_i = -1, Sts_i;
  struct lws_context *pContext_X;
  BOF_WEB_SOCKET_DATA_PER_SESSION *pWebSocketDataPerSession_X;
  uint64_t NbWritten_U64, Remain_U64;
  bool FinalFragment_B, IsLocked_B, DoReleaseBuffer_B;
  BOF::BOF_BUFFER Reply_X;
  uint32_t OpTicket_U32, Nb_U32, SizeOfFirst_U32, MaxSize_U32;
  uint16_t CloseStatus_U16;
  BOF::BOF_RAW_BUFFER *pRawBuffer_X;
  BOF_WEB_RPC_SOCKET_WRITE_OP SocketOperationWriteOp_X;
  std::string WsCltKey_S;
  char pWsCltKey_c[0x200], pCloseInfo_c[0x200];

  if (_pWsi_X)
  {
    Rts_i = 0;
    /* This will be same for every connected peer */
    pContext_X = lws_get_context(_pWsi_X);
    /* This will be different for every connected peer */
    pWebSocketDataPerSession_X = (BOF_WEB_SOCKET_DATA_PER_SESSION *)_pLwsUser;
    if (pWebSocketDataPerSession_X)
    {
    }
#if defined(DBG_PROTO)
    BOF_LOG_TO_DBG("%d [[[LwsCallback]]] %36.36s pWsi %p Usr %p Buf %08zx:%p\n", BOF::Bof_GetMsTickCount(), S_LwsCallbackReasonEnumConverter.ToString(_LwsReason_E).c_str(), _pWsi_X, _pLwsUser, _Len, _pInput);
#endif
    FinalFragment_B = false;
    switch (_LwsReason_E)
    {
      case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
        printf("jj");
        break;

      case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED:
        break;

      case LWS_CALLBACK_HTTP_BIND_PROTOCOL:
        mWebSocketState_X.pWebSocket_X = _pWsi_X;
        break;

      case LWS_CALLBACK_HTTP:
        {
#if 0
        // Get the HTTP method (GET, POST, etc.)
        const char *http_method = nullptr;
        //http_method  = lws_hdr_simple(_pWsi_X, WSI_TOKEN_GET_URI);
        char p[256];
        //lws_serve_http_file();
        //lws_get_mimetype();
        const char *url = (const char *)_pInput;
        Sts_i = lws_hdr_total_length(_pWsi_X, WSI_TOKEN_GET_URI);
        Sts_i = lws_hdr_total_length(_pWsi_X, WSI_TOKEN_POST_URI);
        Sts_i = lws_hdr_total_length(_pWsi_X, WSI_TOKEN_PUT_URI);
        Sts_i = lws_hdr_total_length(_pWsi_X, WSI_TOKEN_DELETE_URI);
        Sts_i = lws_hdr_total_length(_pWsi_X, WSI_TOKEN_OPTIONS_URI);

        Sts_i = lws_hdr_copy(_pWsi_X, p, sizeof(p), WSI_TOKEN_GET_URI);
        Sts_i = lws_hdr_copy(_pWsi_X, p, sizeof(p), WSI_TOKEN_POST_URI);

        if (lws_get_urlarg_by_name(_pWsi_X, WS_CLT_KEY_ARG, pWsCltKey_c, sizeof(pWsCltKey_c)) > 0)
        or lws_hdr_copy_fragment()

        Sts_i=lws_serve_http_file(_pWsi_X, "C:/www/www.bha.org/sdl.html", "text/html", NULL, 0);

        if (!http_method)
        {
          lwsl_err("Failed to get HTTP method\n");
          return -1;
        }

        lwsl_notice("Received HTTP %s request\n", http_method);

        // Get the URI
        const char *uri = nullptr;
        //uri = lws_hdr_simple_ptr(_pWsi_X, WSI_TOKEN_GET_URI);
        if (!uri)
        {
          lwsl_err("Failed to get URI\n");
          return -1;
        }

        lwsl_notice("URI: %s\n", uri);

        // Handle different HTTP methods and URIs as needed

        // Example: Serve a file based on the URI
        if (strcmp(http_method, "GET") == 0)
        {
          if (strcmp(uri, "/") == 0)
          {
            // Serve index.html for the root URI
            lws_serve_http_file(_pWsi_X, "index.html", "text/html", NULL, 0);
          }
          else if (strcmp(uri, "/example") == 0)
          {
            // Serve example.html for the /example URI
            lws_serve_http_file(_pWsi_X, "example.html", "text/html", NULL, 0);
          }
          else
          {
            // Handle other URIs as needed
            // ...
          }
        }
        else if (strcmp(http_method, "POST") == 0)
        {
          // Handle POST requests
          // ...
        }
        else
        {
          // Handle other HTTP methods
          // ...
        }
#endif
        }
        break;

      case LWS_CALLBACK_ADD_HEADERS:
        Rts_i = -1;
        if (lws_get_urlarg_by_name(_pWsi_X, WS_CLT_KEY_ARG, pWsCltKey_c, sizeof(pWsCltKey_c)) <= 0)
        {
          snprintf(pWsCltKey_c, sizeof(pWsCltKey_c), WS_CLT_KEY_VAL_FMT, S_mWsCltId_U32);
        }
        Rts_i = (mClientCollection.size() < mWebSocketParam_X.NbMaxClient_U32) ? 0 : 1; // 1 Reject con
        if (Rts_i == 0)
        {
          Rts_i = (IsClientExist(pWsCltKey_c)) ? 1 : 0; // 1 Reject con
          if (Rts_i == 1)
          {
            lws_close_reason(_pWsi_X, LWS_CLOSE_STATUS_POLICY_VIOLATION, (uint8_t *)pWsCltKey_c, strlen(pWsCltKey_c));
          }
        }
        // BOF_LOG_TO_DBG("LWS_CALLBACK_ADD_HEADERS Rts %d Nb %zd/%d Key %s\n", Rts_i, mClientCollection.size(), mWebSocketParam_X.NbMaxClient_U32, pWsCltKey_c);
#if 0
        // Add custom headers to the response
        lws_return_http_status(_pWsi_X, HTTP_STATUS_UNAUTHORIZED, nullptr);
 //       lws_add_http_header_by_name(_pWsi_X, (const unsigned char *)"Custom-Header:", (const unsigned char *)"Some-Value", 10, nullptr, nullptr);
        
        // Signal that the response is complete
        Sts_i = lws_http_transaction_completed(_pWsi_X);
#endif
        break;

      case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
        break;

      case LWS_CALLBACK_ESTABLISHED:
        Rts_i = -2;
        if (SetSocketBufferSize(_pWsi_X, mWebSocketParam_X.SocketRcvBufferSize_U32, mWebSocketParam_X.SocketSndBufferSize_U32) == BOF_ERR_NO_ERROR)
        {
          Rts_i = -3;
          if (mWebSocketState_X.pWebSocket_X)
          {
            BOF_ASSERT(mWebSocketState_X.pWebSocket_X == _pWsi_X);
          }

          if (lws_get_urlarg_by_name(_pWsi_X, WS_CLT_KEY_ARG, pWsCltKey_c, sizeof(pWsCltKey_c)) <= 0)
          {
            snprintf(pWsCltKey_c, sizeof(pWsCltKey_c), WS_CLT_KEY_VAL_FMT, S_mWsCltId_U32++);
          }
          // BOF_LOG_TO_DBG("LWS_CALLBACK_ESTABLISHED WsCltKey %s Nb %zd/%d\n", pWsCltKey_c, mClientCollection.size(), mWebSocketParam_X.NbMaxClient_U32);
          mWebSocketState_X.pWebSocket_X = _pWsi_X;
          pWebSocketDataPerSession_X->pWebSocket_X = _pWsi_X;
          // WsCltKey_S = std::string(pWebSocketDataPerSession_X->pWsCltKey_c);
          // BOF_LOG_TO_DBG("==1=>this: %p nb clt %zd inst %p pWsi_X %p pWebSocketDataPerSession_X %p WsCltKey %s\n", this, mClientCollection.size(), &mWebSocketState_X, _pWsi_X, pWebSocketDataPerSession_X, pWsCltKey_c);
          Rts_i = (AddClient(pWsCltKey_c, pWebSocketDataPerSession_X) == BOF_ERR_NO_ERROR) ? 0 : 1; // 1 Reject con
          if (Rts_i == 0)
          {
            mWebSocketState_X.ConnectDone_B = true;
            if (mWebSocketParam_X.OnOpen)
            {
              mWebSocketParam_X.OnOpen(mWebSocketParam_X.pUser);  // mWebSocketState_X.pWebSocket_X);
            }
          }
          else
          {
            lws_close_reason(_pWsi_X, LWS_CLOSE_STATUS_POLICY_VIOLATION, (uint8_t *)pWsCltKey_c, strlen(pWsCltKey_c));
          }
          // BOF_LOG_TO_DBG("==2=>this: %p nb clt %zd inst %p Val %p Rts %d Con %d\n", this, mClientCollection.size(), &mWebSocketState_X, _pWsi_X, Rts_i, mWebSocketState_X.ConnectDone_B);
        }
        break;

      case LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH:
        break;

      case LWS_CALLBACK_CLIENT_ESTABLISHED:
        Rts_i = -4;
        if (SetSocketBufferSize(_pWsi_X, mWebSocketParam_X.SocketRcvBufferSize_U32, mWebSocketParam_X.SocketSndBufferSize_U32) == BOF_ERR_NO_ERROR)
        {
          BOF_ASSERT(mWebSocketState_X.pWebSocket_X == _pWsi_X);
          mWebSocketState_X.ConnectDone_B = true;
          Rts_i = 0;
          if (mWebSocketParam_X.OnOpen)
          {
            mWebSocketParam_X.OnOpen(mWebSocketParam_X.pUser);  //mWebSocketState_X.pWebSocket_X);
          }
        }
        break;

      case LWS_CALLBACK_SERVER_WRITEABLE:
      case LWS_CALLBACK_CLIENT_WRITEABLE:
        Sts_E = mpuSocketOperationWriteOpCollection->Peek(&SocketOperationWriteOp_X, 0, nullptr, nullptr);
        if (Sts_E == BOF_ERR_NO_ERROR)
        {
          if (SocketOperationWriteOp_X.Buffer_X.RemainToRead())
          {
            // Data is ENCODED/MASKED/COMPRESSED
            //  lws_set_timeout(Wsi, NO_PENDING_TIMEOUT, 0);
            // BOF_LOG_TO_DBG("-->WRITEIO buf %zx:%p sz %zx WRITE %zx:%p\n", SocketOperationWriteOp_X.Buffer_X.Capacity_U64, SocketOperationWriteOp_X.Buffer_X.pData_U8, SocketOperationWriteOp_X.Buffer_X.Size_U64, SocketOperationWriteOp_X.Buffer_X.RemainToRead(), &SocketOperationWriteOp_X.Buffer_X.pData_U8[SocketOperationWriteOp_X.Buffer_X.Offset_U64]);
            BOF_LWS_WRITE(Sts_i,_pWsi_X, &SocketOperationWriteOp_X.Buffer_X.pData_U8[SocketOperationWriteOp_X.Buffer_X.Offset_U64], SocketOperationWriteOp_X.Buffer_X.RemainToRead(), LWS_WRITE_BINARY); // LWS_WRITE_HTTP);
            NbWritten_U64 = (uint64_t)Sts_i;
            // TODO: All LWS_WRITE_BUFFER macro call have this assert, so in sometime we can simplify the code below
            BOF_ASSERT(NbWritten_U64 == SocketOperationWriteOp_X.Buffer_X.RemainToRead());

            if (_LwsReason_E == LWS_CALLBACK_SERVER_WRITEABLE)
            {
              mNbTxByServer_U64 += NbWritten_U64;
            }
            if (_LwsReason_E == LWS_CALLBACK_CLIENT_WRITEABLE)
            {
              mNbTxByClient_U64 += NbWritten_U64;
            }
            // BOF_LOG_TO_DBG("%-24.24s Tx wsi %p write %zx:%p -> NbWritten %zx Data Masked\n", S_LwsCallbackReasonEnumConverter.ToString(_LwsReason_E).c_str(), _pWsi_X, SocketOperationWriteOp_X.Buffer_X.RemainToRead(), &SocketOperationWriteOp_X.Buffer_X.pData_U8[SocketOperationWriteOp_X.Buffer_X.Offset_U64], NbWritten_U64);
            Remain_U64 = 0;
            SocketOperationWriteOp_X.Result_X.Time_U32 = BOF::Bof_ElapsedMsTime(SocketOperationWriteOp_X.Timer_U32);
            SocketOperationWriteOp_X.Result_X.pBuffer_U8 = &SocketOperationWriteOp_X.Buffer_X.pData_U8[LWS_PRE];

            if ((Sts_i < 0) || (NbWritten_U64 > SocketOperationWriteOp_X.Buffer_X.RemainToRead()))
            {
              SocketOperationWriteOp_X.Result_X.Sts_E = BOF_ERR_WRITE;
              SocketOperationWriteOp_X.Result_X.Size_U32 = SocketOperationWriteOp_X.Buffer_X.Size_U64 - LWS_PRE;
              Sts_E = mpuSocketOperationResultCollection->Push(&SocketOperationWriteOp_X.Result_X, PUSH_POP_TIMEOUT, nullptr, nullptr);
              // BOF_LOG_TO_DBG("%d PUSH_RESULT1 Err %s Ticket %d Op %s Sts %s\n", BOF::Bof_GetMsTickCount(), BOF::Bof_ErrorCode(Sts_E), SocketOperationWriteOp_X.Result_X.OpTicket_U32, S_WebRpcSocketOpEnumConverter.ToString(SocketOperationWriteOp_X.Result_X.Operation_E).c_str(), BOF::Bof_ErrorCode(SocketOperationWriteOp_X.Result_X.Sts_E));

              if (mWebSocketParam_X.OnOperation)
              {
                Sts_E = mWebSocketParam_X.OnOperation(mWebSocketParam_X.pUser, SocketOperationWriteOp_X.Result_X.Operation_E, SocketOperationWriteOp_X.Result_X.OpTicket_U32, SocketOperationWriteOp_X.Result_X.Size_U32, SocketOperationWriteOp_X.Result_X.pBuffer_U8, Sts_E);
              }
              mpuSocketOperationWriteOpCollection->Skip(nullptr, true, nullptr, nullptr, nullptr);
              SocketOperationWriteOp_X.Buffer_X.ReleaseStorage();
              SocketOperationWriteOp_X.Buffer_X.Reset();
            }
            else if (NbWritten_U64 == SocketOperationWriteOp_X.Buffer_X.RemainToRead())
            {
              SocketOperationWriteOp_X.Buffer_X.SeekRel(NbWritten_U64, Remain_U64);
              SocketOperationWriteOp_X.Result_X.Sts_E = BOF_ERR_NO_ERROR;
              SocketOperationWriteOp_X.Result_X.Size_U32 = SocketOperationWriteOp_X.Buffer_X.Size_U64 - LWS_PRE;
              Sts_E = mpuSocketOperationResultCollection->Push(&SocketOperationWriteOp_X.Result_X, PUSH_POP_TIMEOUT, nullptr, nullptr);
              // BOF_LOG_TO_DBG("%d PUSH_RESULT2 Err %s Ticket %d Op %s Sts %s\n", BOF::Bof_GetMsTickCount(), BOF::Bof_ErrorCode(Sts_E), SocketOperationWriteOp_X.Result_X.OpTicket_U32, S_WebRpcSocketOpEnumConverter.ToString(SocketOperationWriteOp_X.Result_X.Operation_E).c_str(), BOF::Bof_ErrorCode(SocketOperationWriteOp_X.Result_X.Sts_E));
              if (mWebSocketParam_X.OnOperation)
              {
                Sts_E = mWebSocketParam_X.OnOperation(mWebSocketParam_X.pUser, SocketOperationWriteOp_X.Result_X.Operation_E, SocketOperationWriteOp_X.Result_X.OpTicket_U32, SocketOperationWriteOp_X.Result_X.Size_U32, SocketOperationWriteOp_X.Result_X.pBuffer_U8, Sts_E);
              }
              mpuSocketOperationWriteOpCollection->Skip(nullptr, true, nullptr, nullptr, nullptr);
              SocketOperationWriteOp_X.Buffer_X.ReleaseStorage();
              SocketOperationWriteOp_X.Buffer_X.Reset();
            }
            else
            {
              SocketOperationWriteOp_X.Buffer_X.SeekRel(NbWritten_U64, Remain_U64);
              if (Remain_U64)
              {
                if (SocketOperationWriteOp_X.Result_X.Time_U32 > SocketOperationWriteOp_X.TimeOut_U32)
                {
                  SocketOperationWriteOp_X.Result_X.Sts_E = BOF_ERR_ETIMEDOUT;
                  SocketOperationWriteOp_X.Result_X.Size_U32 = SocketOperationWriteOp_X.Buffer_X.Size_U64 - LWS_PRE;
                  Sts_E = mpuSocketOperationResultCollection->Push(&SocketOperationWriteOp_X.Result_X, PUSH_POP_TIMEOUT, nullptr, nullptr);
                  // BOF_LOG_TO_DBG("%d PUSH_RESULT3 Err %s Ticket %d Op %s Sts %s\n", BOF::Bof_GetMsTickCount(), BOF::Bof_ErrorCode(Sts_E), SocketOperationWriteOp_X.Result_X.OpTicket_U32, S_WebRpcSocketOpEnumConverter.ToString(SocketOperationWriteOp_X.Result_X.Operation_E).c_str(), BOF::Bof_ErrorCode(SocketOperationWriteOp_X.Result_X.Sts_E));
                  if (mWebSocketParam_X.OnOperation)
                  {
                    Sts_E = mWebSocketParam_X.OnOperation(mWebSocketParam_X.pUser, SocketOperationWriteOp_X.Result_X.Operation_E, SocketOperationWriteOp_X.Result_X.OpTicket_U32, SocketOperationWriteOp_X.Result_X.Size_U32, SocketOperationWriteOp_X.Result_X.pBuffer_U8, Sts_E);
                  }
                  mpuSocketOperationWriteOpCollection->Skip(nullptr, true, nullptr, nullptr, nullptr);
                  SocketOperationWriteOp_X.Buffer_X.ReleaseStorage();
                  SocketOperationWriteOp_X.Buffer_X.Reset();
                }
                else
                {
                  Sts_i = lws_callback_on_writable(mWebSocketState_X.pWebSocket_X);
                  //No because if client disconnect BOF_ASSERT(Sts_i > 0);
                }
              }
            }
            // BOF_LOG_TO_DBG("LwsCallback TxData>>> 2:%zd Rem %zd/%zd Sent %d Status %d\n", NbWritten_U64, SocketOperationWriteOp_X.Buffer_X.RemainToRead(), Remain_U64, Sts_i, mWebSocketState_X.IoSts_E);
          }
        }
        break;

      case LWS_CALLBACK_RECEIVE:
      case LWS_CALLBACK_CLIENT_RECEIVE:
        // Access the 'fin' parameter to check if the [FIN] bit is set
        FinalFragment_B = lws_is_final_fragment(_pWsi_X);

        if (_LwsReason_E == LWS_CALLBACK_RECEIVE)
        {
          mNbRxByServer_U64 += _Len;
#if defined(DBG_PROTO)
          char pDbg_c[0x2000];
          sprintf(pDbg_c, "%d [[[RCV]]] Srv Len %08zx Tot %zd Data:\n", BOF::Bof_GetMsTickCount(), _Len, mNbRxByServer_U64);
          OutputDebugString(pDbg_c);
#endif
        }
        if (_LwsReason_E == LWS_CALLBACK_CLIENT_RECEIVE)
        {
          mNbRxByClient_U64 += _Len;
#if defined(DBG_PROTO)
          char pDbg_c[0x2000];
          sprintf(pDbg_c, "%d [[[RCV]]] Clt Len %08zx Tot %zd Data:\n", BOF::Bof_GetMsTickCount(), _Len, mNbRxByClient_U64);
          OutputDebugString(pDbg_c);
          OutputDebugString((char *)_pInput);
#endif
        }
        mpuRxBufferCollection->SetAppendMode(0, true, nullptr); // By default we append
        Sts_E = mpuRxBufferCollection->PushBuffer(0, _Len, (const uint8_t *)_pInput, &pRawBuffer_X);
        mpuRxBufferCollection->SetAppendMode(0, !FinalFragment_B, &pRawBuffer_X); // If it was the final one, we stop append->Result is pushed and committed
        break;

      default:
        break;
    }

    if ((_LwsReason_E == LWS_CALLBACK_RECEIVE) || (_LwsReason_E == LWS_CALLBACK_CLIENT_RECEIVE))
    {
      // Data is NOT encoded/masked/compressed
#if defined(DBG_PROTO)
      char pDbg_c[0x2000];
      sprintf(pDbg_c, "%-24.24s FinalRx %d wsi %p\n", S_LwsCallbackReasonEnumConverter.ToString(_LwsReason_E).c_str(), FinalFragment_B, _pWsi_X);
      OutputDebugString(pDbg_c);
#endif
      if (FinalFragment_B)
      { 
#if defined(DBG_PROTO)
        char pDbg_c[0x2000];
        sprintf(pDbg_c, "%-24.24s Buff %d:%p Data:\n", S_LwsCallbackReasonEnumConverter.ToString(_LwsReason_E).c_str(), pRawBuffer_X->Size1_U32, pRawBuffer_X->pData1_U8);
        OutputDebugString(pDbg_c);
        OutputDebugString((char *)_pInput);
#endif
        // Data is NOT encoded/masked/compressed
        //BOF_LOG_TO_DBG("%-24.24s FinalRx wsi %p %x:%p Data Masked\n", S_LwsCallbackReasonEnumConverter.ToString(_LwsReason_E).c_str(), _pWsi_X, pRawBuffer_X->Size1_U32, pRawBuffer_X->pData1_U8);
        BOF_ASSERT(pRawBuffer_X->Size2_U32 == 0);
        BOF_ASSERT(pRawBuffer_X->pData2_U8 == nullptr);
        DoReleaseBuffer_B = false;
        if (mWebSocketParam_X.OnMessage)
        {
          if (mWebSocketParam_X.OnMessage(mWebSocketParam_X.pUser, pRawBuffer_X->Size1_U32, pRawBuffer_X->pData1_U8, Reply_X) == BOF_ERR_NO_ERROR)
          {
            DoReleaseBuffer_B = true;
          }
        }
        if (mWebSocketParam_X.OnEvent)
        {
          if (mWebSocketParam_X.OnEvent(mWebSocketParam_X.pUser, _LwsReason_E, pRawBuffer_X->Size1_U32, pRawBuffer_X->pData1_U8, Reply_X) == BOF_ERR_NO_ERROR)
          {
            DoReleaseBuffer_B = true;
            if (Reply_X.Size_U64)
            {
              // BOF_LOG_TO_DBG("-->WRITErp buf %zx:%p sz %zx WRITE %zx:%p\n", Reply_X.Capacity_U64, Reply_X.pData_U8, Reply_X.Size_U64, Reply_X.RemainToRead(), &Reply_X.pData_U8[Reply_X.Offset_U64]);
              BOF_LWS_WRITE(Sts_i, _pWsi_X, &Reply_X.pData_U8[LWS_PRE], Reply_X.Size_U64 - LWS_PRE, LWS_WRITE_BINARY); // LWS_WRITE_HTTP);
              // BOF_LOG_TO_DBG("%-24.24s FinalRx wsi %p SendBack %zx:%p Data %-32.32s\n", S_LwsCallbackReasonEnumConverter.ToString(_LwsReason_E).c_str(), _pWsi_X, Reply_X.Size_U64 - LWS_PRE, &Reply_X.pData_U8[LWS_PRE], &Reply_X.pData_U8[LWS_PRE]);
            }
          }
        }
        // Done in V_OnProcessing by case WEB_RPC_SOCKET_OPERATION_READ in Sts_E = mpuRxBufferCollection->Skip(true, &IsLocked_B);
        if (DoReleaseBuffer_B)
        {
          // If the OnMessage or OnMessage functions have successfully processed the info, we realese the buffer
          // otherwize it will be relesed in V_OnProcessing by case WEB_RPC_SOCKET_OPERATION_READ
          Sts_E = mpuRxBufferCollection->Skip(true, nullptr);
        }
      }
    }
    else
    {
      if (mWebSocketParam_X.OnEvent)
      {
        if (mWebSocketParam_X.OnEvent(mWebSocketParam_X.pUser, _LwsReason_E, _Len, (uint8_t *)_pInput, Reply_X) == BOF_ERR_NO_ERROR)
        {
          if (Reply_X.Size_U64)
          {
            BOF_LWS_WRITE(Sts_i, _pWsi_X, &Reply_X.pData_U8[LWS_PRE], Reply_X.Size_U64 - LWS_PRE, LWS_WRITE_BINARY); // LWS_WRITE_HTTP);
          }
        }
      }
    }

    switch (_LwsReason_E)
    {
      case LWS_CALLBACK_CLIENT_CLOSED:
        mWebSocketState_X.ConnectDone_B = false;
        MaxSize_U32 = sizeof(pCloseInfo_c);
        if (GetCloseInfo(_pWsi_X, CloseStatus_U16, MaxSize_U32, pCloseInfo_c) == BOF_ERR_NO_ERROR)
        {
          BOF_LOG_TO_DBG("LWS_CALLBACK_CLIENT_CLOSED Sts %d SZ %d Info %s\n", CloseStatus_U16, MaxSize_U32, pCloseInfo_c);
        }
        if (mWebSocketParam_X.OnOpen)
        {
          mWebSocketParam_X.OnClose(mWebSocketParam_X.pUser);
        }
        break;

      case LWS_CALLBACK_CLOSED:
        mWebSocketState_X.ConnectDone_B = false;
        RemoveClient(pWebSocketDataPerSession_X);
        MaxSize_U32 = sizeof(pCloseInfo_c);
        if (GetCloseInfo(_pWsi_X, CloseStatus_U16, MaxSize_U32, pCloseInfo_c) == BOF_ERR_NO_ERROR)
        {
          // BOF_LOG_TO_DBG("LWS_CALLBACK_CLOSED Sts %d SZ %d Info %s\n", CloseStatus_U16, MaxSize_U32, pCloseInfo_c);
        }
        if (mWebSocketParam_X.OnOpen)
        {
          mWebSocketParam_X.OnClose(mWebSocketParam_X.pUser);
        }
        break;

      case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        if (mWebSocketParam_X.OnError)
        {
          mWebSocketParam_X.OnError(mWebSocketParam_X.pUser, BOF_ERR_ECONNRESET);
        }
        BOF_LWS_CLIENT_DISCONNECT();
        break;

      default:
        break;
    }

    if (Reply_X.Size_U64)
    {
      Reply_X.ReleaseStorage();
    }

    if ((_LwsReason_E == LWS_CALLBACK_SERVER_WRITEABLE) || (_LwsReason_E == LWS_CALLBACK_CLIENT_WRITEABLE))
    {
      // BOF_LOG_TO_DBG("[[[LwsCallback]]] ===>>> [Rx] Clt %08zx Srv %08zx [Tx] Clt %08zx Srv %08zx\n", mNbRxByClient_U64, mNbRxByServer_U64, mNbTxByClient_U64, mNbTxByServer_U64);
    }
    if ((_LwsReason_E == LWS_CALLBACK_RECEIVE) || (_LwsReason_E == LWS_CALLBACK_CLIENT_RECEIVE))
    {
      // if (FinalFragment_B)
      {
        // BOF_LOG_TO_DBG("[[[LwsCallback]]] ===>>> [Rx] Clt %08zx Srv %08zx [Tx] Clt %08zx Srv %08zx\n", mNbRxByClient_U64, mNbRxByServer_U64, mNbTxByClient_U64, mNbTxByServer_U64);
      }
    }
  }

  // DBGLOG("S_Lws_Client_Callback_Http leave\n");
  return Rts_i;
}

BOFERR BofWebSocket::V_OnProcessing()
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR, Sts_E;
  uint64_t Remain_U64;
  uint32_t i_U32, PollTimeout_U32, Start_U32, Delta_U32, ListenTicketId_U32, Nb_U32, SizeOfFirst_U32;
  int Sts_i, Len_i;
  struct lws_context_creation_info LwsCreationInfo_X;
  BOF_WEB_SOCKET_DATA_PER_SESSION *pWebSocketDataPerSession_X;
  std::string Ip_S, FullIp_S;
  struct lws_client_connect_info ClientConnectInfo_X;
  BOF_WEB_RPC_SOCKET_OPERATION_RESULT Result_X;
  bool NewCommandRcv_B, SendResult_B, IoPending_B, ListeningMode_B;
  BOF_WEB_RPC_SOCKET_WRITE_OP SocketOperationWriteOp_X;
  char pWsCltKey_c[0x200];

  PollTimeout_U32 = PUSH_POP_TIMEOUT;
  NewCommandRcv_B = false;
  SendResult_B = false;
  IoPending_B = false;
  ListeningMode_B = false;
  do
  {
    if (mpLwsContext_X)
    {
      // Will exit if I call LWS_WAKEUP_SERVICE
      BOF_LOG_TO_DBG("ENTER]]]this %p ENTER lws_service %p\n", this, mpLwsContext_X);
      Sts_i = lws_service(mpLwsContext_X, PollTimeout_U32); // Service the WebSocket context, the timeout arg is ignored and replaced by 30 sec
      BOF_LOG_TO_DBG("LEAVE[[[this %p lws_service %p sts %d\n", this, mpLwsContext_X, Sts_i);

      Sts_E = mpuSocketOperationParamCollection->Pop(&mCurrentOp_X, 0, nullptr, nullptr);
    }
    else
    {
      mCurrentOp_X.Reset();
      Sts_E = mpuSocketOperationParamCollection->Pop(&mCurrentOp_X, PollTimeout_U32, nullptr, nullptr);
    }
    // Check if a new cmd has been received or just command timeout on the queue
    NewCommandRcv_B = (Sts_E == BOF_ERR_NO_ERROR);
    if ((!IoPending_B) || (ListeningMode_B))
    {
      if (NewCommandRcv_B)
      {
        // mCurrentOpParam_X.TimeOut_U32, mpuSocketOperationParamCollection->GetNbElement(), mpuSocketOperationResultCollection->GetNbElement()); First init of operation param
        Result_X.Reset();
        Result_X.Sts_E = BOF_ERR_NO_ERROR;
        Result_X.Operation_E = mCurrentOp_X.Operation_E;
        Result_X.OpTicket_U32 = mCurrentOp_X.Ticket_U32;
        Result_X.Time_U32 = BOF::Bof_ElapsedMsTime(mCurrentOp_X.Timer_U32); // Time to get the command in processing thread
        mCurrentOp_X.Timer_U32 = BOF::Bof_GetMsTickCount();                 // Restart op timer to have the time to execute this one
        // BOF_LOG_TO_DBG("%d ==CMD==> PopCmd rts %d cmd %s Ticket %d NewCommandRcv %d IoPending %d ListeningMode %d FifoSz %d\n", BOF::Bof_GetMsTickCount(), Rts_E, S_WebRpcSocketOpEnumConverter.ToString(mCurrentOp_X.Operation_E).c_str(), mCurrentOp_X.Ticket_U32, NewCommandRcv_B, IoPending_B, ListeningMode_B, mpuSocketOperationParamCollection->GetNbElement());
        //  So ListeningMode_B is perhaps true, but nevertheless a new command must be processed
        mOperationPending_B = true; // can be resetted by canceled
        SendResult_B = true;
        Start_U32 = 0;
#if defined(DBG_PROTO)
        BOF_LOG_TO_DBG("V_OnProcessing Op %d\n", mCurrentOp_X.Operation_E);
#endif
        switch (mCurrentOp_X.Operation_E)
        {
          case BOF_WEB_RPC_SOCKET_OPERATION::BOF_WEB_RPC_SOCKET_OPERATION_LISTEN:
            Result_X.Sts_E = BOF_ERR_WRONG_MODE;
            if (mWebSocketParam_X.NbMaxClient_U32)
            {
              Result_X.Sts_E = BOF_ERR_INVALID_STATE;
              if ((!ListeningMode_B) && (mpLwsContext_X == nullptr))
              {
                mWebSocketState_X.ConnectDone_B = false;
                memset(&LwsCreationInfo_X, 0, sizeof(LwsCreationInfo_X));
                // Does not work !!!!   LwsCreationInfo_X.iface = mWebSocketServerParam_X.Ip_X.ToString(false, false).c_str();
                Ip_S = mWebSocketParam_X.ServerIp_X.ToString(false, false);
                LwsCreationInfo_X.iface = Ip_S.c_str();                       // "192.168.56.1";
                LwsCreationInfo_X.port = mWebSocketParam_X.ServerIp_X.Port(); // 8080; // HTTP port
                for (i_U32 = 0; i_U32 < BOF_NB_ELEM_IN_ARRAY(S_pLwsServerProtocol_X); i_U32++)
                {
                  S_pLwsServerProtocol_X[i_U32].per_session_data_size = sizeof(BOF_WEB_SOCKET_DATA_PER_SESSION);
                  S_pLwsServerProtocol_X[i_U32].user = this;
                }
                LwsCreationInfo_X.protocols = S_pLwsServerProtocol_X;
                LwsCreationInfo_X.user = this;
                LwsCreationInfo_X.mounts = &S_MountPoint_X;
                // Create the libwebsockets context
                mpLwsContext_X = lws_create_context(&LwsCreationInfo_X);
                if (!mpLwsContext_X)
                {
                  BOF_LWS_CLIENT_DISCONNECT();
                  Result_X.Sts_E = BOF_ERR_INIT;
                }
                else
                {
                  // struct lws *p= lws_get_context_wsi(mpLwsContext_X);
                  Result_X.Sts_E = BOF_ERR_NO_ERROR;
                  mCurrentOp_X.TimeOut_U32 = PollTimeout_U32;
                  ListenTicketId_U32 = mCurrentOp_X.Ticket_U32;
                  ListeningMode_B = true;
                }
              }
            }
            break;

          case BOF_WEB_RPC_SOCKET_OPERATION::BOF_WEB_RPC_SOCKET_OPERATION_CONNECT:
            Result_X.Sts_E = BOF_ERR_WRONG_MODE;
            if (!mWebSocketParam_X.NbMaxClient_U32)
            {
              Result_X.Sts_E = BOF_ERR_INVALID_STATE;
              Len_i = strlen(mCurrentOp_X.OpParam.Connect_X.pWsCltKey_c);
              if ((!mWebSocketState_X.ConnectDone_B) && (mpLwsContext_X == nullptr) && (Len_i) && (Len_i < sizeof(mCurrentOp_X.OpParam.Connect_X.pWsCltKey_c)))
              {
                memset(&LwsCreationInfo_X, 0, sizeof(LwsCreationInfo_X));
                LwsCreationInfo_X.port = CONTEXT_PORT_NO_LISTEN;
                LwsCreationInfo_X.gid = -1;
                LwsCreationInfo_X.uid = -1;
                for (i_U32 = 0; i_U32 < BOF_NB_ELEM_IN_ARRAY(S_pLwsClientProtocol_X); i_U32++)
                {
                  S_pLwsClientProtocol_X[i_U32].per_session_data_size = sizeof(BOF_WEB_SOCKET_DATA_PER_SESSION);
                  S_pLwsClientProtocol_X[i_U32].user = this;
                }
                LwsCreationInfo_X.protocols = S_pLwsClientProtocol_X;
                LwsCreationInfo_X.user = this;
                // LwsCreationInfo_X.max_http_header_data = 16384; // Set the maximum header size
                // LwsCreationInfo_X.max_http_header_pool = 1024;  // Set the maximum header pool size
                //  Create the libwebsockets context
                mpLwsContext_X = lws_create_context(&LwsCreationInfo_X);
                if (!mpLwsContext_X)
                {
                  BOF_LWS_CLIENT_DISCONNECT();
                  Result_X.Sts_E = BOF_ERR_INIT;
                }
                else
                {
                  Result_X.Sts_E = BOF_ERR_ENOTCONN;
                  memset(&ClientConnectInfo_X, 0, sizeof(ClientConnectInfo_X));
                  ClientConnectInfo_X.context = mpLwsContext_X;
                  Ip_S = mCurrentOp_X.OpParam.Connect_X.DstIpAddr_X.ToString();
                  FullIp_S = mCurrentOp_X.OpParam.Connect_X.DstIpAddr_X.ToString(mCurrentOp_X.OpParam.Connect_X.DstPort_U16);
                  // The '=' is inside URI_CLIENT_NAME_ARG
                  BOF_SNPRINTF_NULL_CLIPPED(pWsCltKey_c, sizeof(pWsCltKey_c), "/?%s%s", WS_CLT_KEY_ARG, mCurrentOp_X.OpParam.Connect_X.pWsCltKey_c);
                  ClientConnectInfo_X.address = Ip_S.c_str();
                  ClientConnectInfo_X.port = mCurrentOp_X.OpParam.Connect_X.DstPort_U16;
                  ClientConnectInfo_X.path = pWsCltKey_c;
                  ClientConnectInfo_X.host = lws_canonical_hostname(mpLwsContext_X);
                  ClientConnectInfo_X.origin = FullIp_S.c_str();
                  ClientConnectInfo_X.protocol = S_pLwsClientProtocol_X[0].name;
                  ClientConnectInfo_X.ietf_version_or_minus_one = -1; // IETF version

                  mWebSocketState_X.pWebSocket_X = lws_client_connect_via_info(&ClientConnectInfo_X);
#if defined(DBG_PROTO)
                  BOF_LOG_TO_DBG("V_OnProcessing WEB_RPC_SOCKET_OPERATION_CONNECT pWebSocket_X %p on %s:%d %s\n", mWebSocketState_X.pWebSocket_X, Ip_S.c_str(), mCurrentOp_X.OpParam.Connect_X.DstPort_U16, FullIp_S.c_str());
#endif
                  // BOF_LOG_TO_DBG("lws_client_connect_via_info %s pWebSocket_X %p ConnectDone %d\n", pWsCltKey_c, mWebSocketState_X.pWebSocket_X, mWebSocketState_X.ConnectDone_B);
                  if (mWebSocketState_X.pWebSocket_X)
                  {
                    Result_X.Sts_E = BOF_ERR_NO_ERROR;
                  }
                }
              }
            }
            break;

          case BOF_WEB_RPC_SOCKET_OPERATION::BOF_WEB_RPC_SOCKET_OPERATION_DISCONNECT:
            Result_X.Sts_E = BOF_ERR_WRONG_MODE;
            if (!mWebSocketParam_X.NbMaxClient_U32)
            {
              Result_X.Sts_E = BOF_ERR_INVALID_STATE;
              if ((mWebSocketState_X.ConnectDone_B) || (mpLwsContext_X))
              {
                BOF_LWS_CLIENT_DISCONNECT();
                Result_X.Sts_E = BOF_ERR_NO_ERROR;
              }
            }
            break;

          case BOF_WEB_RPC_SOCKET_OPERATION::BOF_WEB_RPC_SOCKET_OPERATION_EXIT:
            // DBGLOG("%d: WEB_RPC_SOCKET_OPERATION_EXIT\n", BOF::Bof_GetMsTickCount());
            Result_X.Sts_E = BOF_ERR_CANCEL;
            Rts_E = BOF_ERR_CANCEL; // To exit loop here and in BofThread:
            break;

          case BOF_WEB_RPC_SOCKET_OPERATION::BOF_WEB_RPC_SOCKET_OPERATION_READ:
            Result_X.Sts_E = BOF_ERR_ENOTCONN;
            if (!mWebSocketParam_X.NbMaxClient_U32)
            {
              if ((mWebSocketState_X.ConnectDone_B) && (mWebSocketState_X.pWebSocket_X))
              {
                Result_X.Sts_E = BOF_ERR_NO_ERROR;
              }
            }
            else
            {
              if (mWebSocketState_X.pWebSocket_X)
              {
                Result_X.Sts_E = BOF_ERR_NO_ERROR;
              }
            }
            if (Result_X.Sts_E == BOF_ERR_NO_ERROR)
            {
              if ((mCurrentOp_X.OpParam.Read_X.pBuffer_U8) && (mCurrentOp_X.OpParam.Read_X.Nb_U32))
              {
                Start_U32 = BOF::Bof_GetMsTickCount();
                Result_X.Sts_E = BOF_ERR_NO_ERROR;
              }
              else
              {
                Result_X.Sts_E = BOF_ERR_EINVAL;
              }
            }
            break;

          case BOF_WEB_RPC_SOCKET_OPERATION::BOF_WEB_RPC_SOCKET_OPERATION_WRITE:
            Result_X.Sts_E = BOF_ERR_ENOTCONN;
            if (!mWebSocketParam_X.NbMaxClient_U32)
            {
              if ((mWebSocketState_X.ConnectDone_B) && (mWebSocketState_X.pWebSocket_X))
              {
                Result_X.Sts_E = BOF_ERR_NO_ERROR;
              }
            }
            else
            {
              if (mWebSocketState_X.pWebSocket_X)
              {
                Result_X.Sts_E = BOF_ERR_NO_ERROR;
              }
            }
            if (Result_X.Sts_E == BOF_ERR_NO_ERROR)
            {
              if ((mCurrentOp_X.OpParam.Write_X.pBuffer_U8) && (mCurrentOp_X.OpParam.Write_X.Nb_U32))
              {
                // NO because this is a stack var and it can contains a still active buffer which has been pushed on mpuSocketOperationWriteOpCollection and is storing data in an async way
                // NO SocketOperationWriteOp_X.Reset(); IF SocketOperationWriteOp_X.Buffer_X.pData_U8 is not null buffer will be deleted
                SocketOperationWriteOp_X.Buffer_X.pData_U8 = nullptr; // Detach storage
                SocketOperationWriteOp_X.Reset();
                BOF_LWS_ALLOCATE_BUFFER(SocketOperationWriteOp_X.Buffer_X, mCurrentOp_X.OpParam.Write_X.Nb_U32);
                BOF_LWS_WRITE_BUFFER(SocketOperationWriteOp_X.Buffer_X, mCurrentOp_X.OpParam.Write_X.Nb_U32, mCurrentOp_X.OpParam.Write_X.pBuffer_U8);
                BOF_LOG_TO_DBG("-->ALLOCIO buf %zx:%p sz %zx BEGIN %zx:%p\n", SocketOperationWriteOp_X.Buffer_X.Capacity_U64, SocketOperationWriteOp_X.Buffer_X.pData_U8, SocketOperationWriteOp_X.Buffer_X.Size_U64, SocketOperationWriteOp_X.Buffer_X.RemainToRead(), &SocketOperationWriteOp_X.Buffer_X.pData_U8[SocketOperationWriteOp_X.Buffer_X.Offset_U64]);

                /*
                SocketOperationWriteOp_X.Buffer_X.SetStorage(mCurrentOp_X.OpParam.Write_X.Nb_U32 + LWS_PRE, mCurrentOp_X.OpParam.Write_X.Nb_U32 + LWS_PRE, mCurrentOp_X.OpParam.Write_X.pBuffer_U8);
                SocketOperationWriteOp_X.Buffer_X.SeekAbs(LWS_PRE, Remain_U64);
                */
                SocketOperationWriteOp_X.TimeOut_U32 = mCurrentOp_X.TimeOut_U32;
                SocketOperationWriteOp_X.Timer_U32 = mCurrentOp_X.Timer_U32;
                SocketOperationWriteOp_X.Result_X = Result_X;
                Result_X.Sts_E = mpuSocketOperationWriteOpCollection->Push(&SocketOperationWriteOp_X, 0, nullptr, nullptr);
                if (Result_X.Sts_E == BOF_ERR_NO_ERROR)
                {
                  Sts_i = lws_callback_on_writable(mWebSocketState_X.pWebSocket_X);
                  //No because if client disconnect                  BOF_ASSERT(Sts_i > 0);
                  if (Sts_i > 0)
                  {
                    Start_U32 = BOF::Bof_GetMsTickCount();
                  }
                  else
                  {
                    Result_X.Sts_E = BOF_ERR_OPERATION_FAILED;
                  }
                }
              }
              else
              {
                Result_X.Sts_E = BOF_ERR_EINVAL;
              }
            }
            break;

          default:
            Result_X.Sts_E = BOF_ERR_INVALID_COMMAND;
            break;
        } // switch (mCurrentOp_X.Operation_E)
        if (Result_X.Sts_E == BOF_ERR_NO_ERROR)
        {
          IoPending_B = true;
        }
      } // if (NewCommandRcv_B)
    }   // if (!IoPending_B)

    if (IoPending_B)
    {
      SendResult_B = false;
      switch (mCurrentOp_X.Operation_E)
      {
        case BOF_WEB_RPC_SOCKET_OPERATION::BOF_WEB_RPC_SOCKET_OPERATION_LISTEN:
          if (mWebSocketState_X.ConnectDone_B)
          {
            mWebSocketState_X.ConnectDone_B = false;
            mCurrentOp_X.Timer_U32 = BOF::Bof_GetMsTickCount();
            Result_X.Operation_E = BOF_WEB_RPC_SOCKET_OPERATION::BOF_WEB_RPC_SOCKET_OPERATION_CONNECT;
            // BOF_INC_TICKET_NUMBER(ListenTicketId_U32);
            Result_X.OpTicket_U32 = ListenTicketId_U32; // All WEB_RPC_SOCKET_OPERATION_CONNECT event will return ticket of listen op
            Result_X.Sts_E = BOF_ERR_NO_ERROR;
            SendResult_B = true;
            IoPending_B = true;
          }
          break;

        case BOF_WEB_RPC_SOCKET_OPERATION::BOF_WEB_RPC_SOCKET_OPERATION_CONNECT:
          if (mWebSocketState_X.ConnectDone_B)
          {
            Result_X.Sts_E = BOF_ERR_NO_ERROR;
            SendResult_B = true;
            IoPending_B = false;
          }
          break;

        case BOF_WEB_RPC_SOCKET_OPERATION::BOF_WEB_RPC_SOCKET_OPERATION_DISCONNECT:
          SendResult_B = true;
          IoPending_B = false;
          break;

        case BOF_WEB_RPC_SOCKET_OPERATION::BOF_WEB_RPC_SOCKET_OPERATION_READ:
          Nb_U32 = mCurrentOp_X.OpParam.Read_X.Nb_U32;
          Result_X.Sts_E = mpuRxBufferCollection->PopBuffer(PollTimeout_U32, &Nb_U32, mCurrentOp_X.OpParam.Read_X.pBuffer_U8);
          // BOF_LOG_TO_DBG("%d: sts %d to %d try pop WEB_RPC_SOCKET_OPERATION_READ mpuRxBufferCollection %p SizeOfFirst %d FifoSz %d\n", BOF::Bof_GetMsTickCount(), Result_X.Sts_E, PollTimeout_U32, mpuRxBufferCollection.get(), SizeOfFirst_U32, Nb_U32);
          //  BOF_LOG_TO_DBG("WEB_RPC_SOCKET_OPERATION_READ sts %d Nb %d\n", Result_X.Sts_E, Nb_U32);
          if (Result_X.Sts_E == BOF_ERR_NO_ERROR)
          {
            Result_X.Size_U32 = Nb_U32;
            Result_X.pBuffer_U8 = mCurrentOp_X.OpParam.Read_X.pBuffer_U8;
            SendResult_B = true;
            IoPending_B = false;
          }
          else
          {
            Result_X.Sts_E = BOF_ERR_PENDING;
          }
          break;

        case BOF_WEB_RPC_SOCKET_OPERATION::BOF_WEB_RPC_SOCKET_OPERATION_WRITE:
          Result_X.Sts_E = BOF_ERR_NO_ERROR;
          SendResult_B = false; // Made by case LWS_CALLBACK_SERVER_WRITEABLE:/ case LWS_CALLBACK_CLIENT_WRITEABLE: in LwsCallback
          IoPending_B = false;
          break;

        default:
          Result_X.Sts_E = BOF_ERR_BAD_ID;
          IoPending_B = false;
          break;
      } // switch (mCurrentOp_X.Operation_E)
      Result_X.Time_U32 = BOF::Bof_ElapsedMsTime(mCurrentOp_X.Timer_U32);
      if ((!ListeningMode_B) && (mCurrentOp_X.Operation_E != BOF_WEB_RPC_SOCKET_OPERATION::BOF_WEB_RPC_SOCKET_OPERATION_WRITE)) // Made by case LWS_CALLBACK_SERVER_WRITEABLE:/ case LWS_CALLBACK_CLIENT_WRITEABLE: in LwsCallback
      {
        if (Result_X.Time_U32 > mCurrentOp_X.TimeOut_U32)
        {
          BOF_LOG_TO_DBG("WebSocket::V_OnProcessing Op %s Ticket %d TIMEOUT %d > %d\n !!!", S_WebRpcSocketOpEnumConverter.ToString(mCurrentOp_X.Operation_E).c_str(), mCurrentOp_X.Ticket_U32, Result_X.Time_U32, mCurrentOp_X.TimeOut_U32);
          if (mCurrentOp_X.Operation_E == BOF_WEB_RPC_SOCKET_OPERATION::BOF_WEB_RPC_SOCKET_OPERATION_CONNECT)
          {
            BOF_LWS_CLIENT_DISCONNECT();
          }
          Result_X.Sts_E = BOF_ERR_ETIMEDOUT;
          SendResult_B = true;
          IoPending_B = false;
        }
      }
    } // if (IoPending_B)

    if (mCancel_B)
    {
      mCancel_B = false;
      mpuSocketOperationParamCollection->Reset();
      mpuSocketOperationResultCollection->Reset();
      mpuSocketOperationWriteOpCollection->Reset();
      mpuRxBufferCollection->Reset();
      mCurrentOp_X.Reset();
      mWebSocketState_X.Reset();
      mOperationPending_B = false;
      mNbRxByServer_U64 = 0;
      mNbTxByServer_U64 = 0;
      mNbRxByClient_U64 = 0;
      mNbTxByClient_U64 = 0;

      NewCommandRcv_B = false;
      SendResult_B = false;
      IoPending_B = false;

      Bof_SignalEvent(mCancelEvent_X, 0);
    }

    if (SendResult_B)
    {
      SendResult_B = false;
      Result_X.Time_U32 = BOF::Bof_ElapsedMsTime(mCurrentOp_X.Timer_U32);
      Sts_E = mpuSocketOperationResultCollection->Push(&Result_X, PUSH_POP_TIMEOUT, nullptr, nullptr);

      if (mWebSocketParam_X.OnOperation)
      {
        Sts_E = mWebSocketParam_X.OnOperation(mWebSocketParam_X.pUser, mCurrentOp_X.Operation_E, mCurrentOp_X.Ticket_U32, Result_X.Size_U32, Result_X.pBuffer_U8, Sts_E);
      }
    }
  } while ((!IsThreadLoopMustExit()) && (Rts_E == BOF_ERR_NO_ERROR));

  BOF_LWS_CLIENT_DISCONNECT();
  BOF_LOG_TO_DBG("WebSocket client thread stopped...\n");
  return Rts_E;
}
END_WEBRPC_NAMESPACE()