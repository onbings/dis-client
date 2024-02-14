/*!
  Copyright (c) 2008, EVS. All rights reserved.

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND,  EITHER EXPRESSED OR IMPLIED,  INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  History:
    V 1.00  August 24 2013  BHA : Original version
*/
#pragma once
#include "bofwebrpc/bofwebrpc.h"
#include <libwebsockets.h>
typedef enum lws_callback_reasons BOF_WEB_SOCKET_LWS_CALLBACK_REASON;
typedef struct lws BOF_WEB_SOCKET_LWS;
#include <bofstd/bofcircularbuffer.h>
#include <bofstd/bofrawcircularbuffer.h>
#include <bofstd/bofsocketos.h>
#include <bofstd/bofthread.h>

#include <map>

BEGIN_WEBRPC_NAMESPACE()
enum BOF_WEB_RPC_SOCKET_OPERATION
{
  BOF_WEB_RPC_SOCKET_OPERATION_EXIT = 0,
  BOF_WEB_RPC_SOCKET_OPERATION_LISTEN,
  BOF_WEB_RPC_SOCKET_OPERATION_CONNECT,
  BOF_WEB_RPC_SOCKET_OPERATION_READ,
  BOF_WEB_RPC_SOCKET_OPERATION_WRITE,
  BOF_WEB_RPC_SOCKET_OPERATION_DISCONNECT,
  BOF_WEB_RPC_SOCKET_OPERATION_MAX
};
constexpr uint32_t WS_CLT_MAX_KEY_LEN = 128;

using BOF_WEB_SOCKET_OPERATION_CALLBACK = std::function<BOFERR(void *_pUser, BOFWEBRPC::BOF_WEB_RPC_SOCKET_OPERATION _Operation_E, uint32_t _Ticket_U32, uint32_t _Size_U32, uint8_t *_pBuffer_U8, BOFERR _Sts_E)>;
using BOF_WEB_SOCKET_EVENT_CALLBACK = std::function < BOFERR(void *_pUser, BOF_WEB_SOCKET_LWS_CALLBACK_REASON _Reason_E, uint32_t _Nb_U32, uint8_t *_pData_U8, BOF::BOF_BUFFER &_rReply_X)>;
using BOF_WEB_SOCKET_OPEN_CALLBACK = std::function<BOFERR(void *_pUser)>;
using BOF_WEB_SOCKET_CLOSE_CALLBACK = std::function<BOFERR(void *_pUser)>;
using BOF_WEB_SOCKET_ERROR_CALLBACK = std::function<BOFERR(void *_pUser, BOFERR _Sts_E)>;
using BOF_WEB_SOCKET_MESSAGE_CALLBACK = std::function<BOFERR(void *_pUser, uint32_t _Nb_U32, uint8_t *_pData_U8, BOF::BOF_BUFFER &_rReply_X)>;

struct BOF_WEB_RPC_SOCKET_THREAD_PARAM
{
  std::string Name_S;
  BOF::BOF_THREAD_SCHEDULER_POLICY ThreadSchedulerPolicy_E;
  BOF::BOF_THREAD_PRIORITY ThreadPriority_E;

  BOF_WEB_RPC_SOCKET_THREAD_PARAM()
  {
    Reset();
  }
  void Reset()
  {
    Name_S = "";
    ThreadSchedulerPolicy_E = BOF::BOF_THREAD_SCHEDULER_POLICY::BOF_THREAD_SCHEDULER_POLICY_MAX;
    ThreadPriority_E = BOF::BOF_THREAD_PRIORITY::BOF_THREAD_PRIORITY_000;
  }
};

struct BOF_WEB_SOCKET_PARAM
{
  void *pUser;    //User data delivered by callback
  uint32_t SocketRcvBufferSize_U32; // If 0 use default socket buffer size, otherwise specifies the size of the Rx buffer linked to this websocket. The os can limit this value
  uint32_t SocketSndBufferSize_U32; // If 0 use default socket buffer size, otherwise specifies the size of the Tx buffer linked to this websocket. The os can limit this value
  uint32_t NbMaxOperationPending_U32;
  uint32_t RxBufferSize_U32;
  uint32_t NbMaxBufferEntry_U32;
  BOF_WEB_SOCKET_OPERATION_CALLBACK OnOperation;
  BOF_WEB_SOCKET_EVENT_CALLBACK OnEvent;  //If you return something different from BOF_ERR_NO_ERROR for LWS_CALLBACK_RECEIVE/LWS_CALLBACK_CLIENT_RECEIVE event
                                      //you NEED TO call BOFERR ProgramWebSocketOperation(uint32_t _TimeOut_U32, WEB_RPC_SOCKET_READ_PARAM &_rParam_X, uint32_t &_rOpTicket_U32);
                                      //to release data which are pushed in the mpuRxBufferCollection buffer !!!
  BOF_WEB_SOCKET_OPEN_CALLBACK OnOpen;
  BOF_WEB_SOCKET_CLOSE_CALLBACK OnClose;
  BOF_WEB_SOCKET_ERROR_CALLBACK OnError;
  BOF_WEB_SOCKET_MESSAGE_CALLBACK OnMessage; //If you return something different from BOF_ERR_NO_ERROR, 
                                         //you NEED TO call BOFERR ProgramWebSocketOperation(uint32_t _TimeOut_U32, WEB_RPC_SOCKET_READ_PARAM &_rParam_X, uint32_t &_rOpTicket_U32);
                                         //to release data which are pushed in the mpuRxBufferCollection buffer !!!

  uint32_t NbMaxClient_U32; // If 0->Client otherwise Server
  BOF::BOF_SOCKET_ADDRESS ServerIp_X;
  BOF_WEB_RPC_SOCKET_THREAD_PARAM WebSocketThreadParam_X;

  BOF_WEB_SOCKET_PARAM()
  {
    Reset();
  }
  void Reset()
  {
    SocketRcvBufferSize_U32 = 0;
    SocketSndBufferSize_U32 = 0;
    NbMaxOperationPending_U32 = 0;
    RxBufferSize_U32 = 0;
    NbMaxBufferEntry_U32 = 0;
    OnOperation = nullptr;
    OnOpen = nullptr;
    OnClose = nullptr;
    OnError = nullptr;
    OnMessage = nullptr;
    NbMaxClient_U32 = 0;
    ServerIp_X.Reset();
    WebSocketThreadParam_X.Reset();
  }
};
struct BOF_WEB_RPC_SOCKET_EXIT_PARAM
{
  uint32_t Unused_U32;
  BOF_WEB_RPC_SOCKET_EXIT_PARAM()
  {
    Reset();
  }
  void Reset()
  {
    Unused_U32 = 0;
  }
};

struct BOF_WEB_RPC_SOCKET_CONNECT_PARAM
{
  char pWsCltKey_c[WS_CLT_MAX_KEY_LEN]; // Specify the name of the client which will be created on the server
  BOF::BOF_IPV4_ADDR_U32 DstIpAddr_X;
  uint16_t DstPort_U16;
  BOF_WEB_RPC_SOCKET_CONNECT_PARAM()
  {
    Reset();
  }
  void Reset()
  {
    pWsCltKey_c[0] = 0;
    DstIpAddr_X.Reset();
    DstPort_U16 = 0;
  }
};
struct BOF_WEB_RPC_SOCKET_LISTEN_PARAM
{
  BOF::BOF_IPV4_ADDR_U32 SrcIpAddr_X;
  uint16_t SrcPort_U16;
  uint32_t NbMaxClient_U32;
  BOF_WEB_RPC_SOCKET_LISTEN_PARAM()
  {
    Reset();
  }
  void Reset()
  {
    SrcIpAddr_X.Reset();
    SrcPort_U16 = 0;
    NbMaxClient_U32 = 0;
  }
};

struct BOF_WEB_RPC_SOCKET_READ_PARAM
{
  // Not usefull for now char pWsCltKey_c[WS_CLT_MAX_KEY_LEN];  //If defined it specify the name of the client on the server which will write the data
  uint32_t Nb_U32;
  uint8_t *pBuffer_U8;
  BOF_WEB_RPC_SOCKET_READ_PARAM()
  {
    Reset();
  }
  void Reset()
  {
    // pWsCltKey_c[0] = 0;
    Nb_U32 = 0;
    pBuffer_U8 = nullptr;
  }
};

struct BOF_WEB_RPC_SOCKET_WRITE_PARAM
{
  // Not usefull for now  char pWsCltKey_c[WS_CLT_MAX_KEY_LEN];  //If defined it specify the name of the client on the server which will write the data
  uint32_t Nb_U32;
  uint8_t *pBuffer_U8;
  BOF_WEB_RPC_SOCKET_WRITE_PARAM()
  {
    Reset();
  }
  void Reset()
  {
    // pWsCltKey_c[0] = 0;
    Nb_U32 = 0;
    pBuffer_U8 = nullptr;
  }
};

struct BOF_WEB_RPC_SOCKET_DISCONNECT_PARAM
{
  uint32_t Unused_U32;
  BOF_WEB_RPC_SOCKET_DISCONNECT_PARAM()
  {
    Reset();
  }
  void Reset()
  {
    Unused_U32 = 0;
  }
};
struct BOF_WEB_RPC_SOCKET_OPERATION_RESULT
{
  BOF_WEB_RPC_SOCKET_OPERATION Operation_E;
  uint32_t OpTicket_U32;
  BOFERR Sts_E;      /*! The operation status */
  uint32_t Size_U32; /*! The total size transferred in bytes */
  uint8_t *pBuffer_U8;
  uint32_t Time_U32; /*! The operation time in ms */
  BOF_WEB_RPC_SOCKET_OPERATION_RESULT()
  {
    Reset();
  }
  void Reset()
  {
    Operation_E = BOF_WEB_RPC_SOCKET_OPERATION::BOF_WEB_RPC_SOCKET_OPERATION_MAX;
    OpTicket_U32 = 0;
    Sts_E = BOF_ERR_SOFT_INIT;
    Size_U32 = 0;
    pBuffer_U8 = nullptr;
    Time_U32 = 0;
  }
};

struct BOF_WEB_RPC_SOCKET_OPERATION_PARAM
{
  BOF_WEB_RPC_SOCKET_OPERATION Operation_E; /*! The operation to perform */
  uint32_t Ticket_U32;
  uint32_t TimeOut_U32;
  uint32_t Timer_U32; /*! The associated timer */
  union _OpParam {
    _OpParam()
    {
    }

    BOF_WEB_RPC_SOCKET_EXIT_PARAM Exit_X;       /*! The exit operation params */
    BOF_WEB_RPC_SOCKET_LISTEN_PARAM Listen_X;   /*! The listen operation params */
    BOF_WEB_RPC_SOCKET_CONNECT_PARAM Connect_X; /*! The connect operation params */
    BOF_WEB_RPC_SOCKET_READ_PARAM Read_X;       /*! The read operation params */
    BOF_WEB_RPC_SOCKET_WRITE_PARAM Write_X;     /*! The write operation params */
    BOF_WEB_RPC_SOCKET_DISCONNECT_PARAM Disconnect_X;
  } OpParam;
  BOF_WEB_RPC_SOCKET_OPERATION_PARAM()
  {
    Reset();
  }
  void Reset()
  {
    Operation_E = BOF_WEB_RPC_SOCKET_OPERATION::BOF_WEB_RPC_SOCKET_OPERATION_MAX;
    Ticket_U32 = 0;
    TimeOut_U32 = 0;
    Timer_U32 = 0;
  }
};

struct BOF_WEB_RPC_SOCKET_WRITE_OP
{
  uint32_t TimeOut_U32;
  uint32_t Timer_U32; /*! The associated timer */
  BOF::BOF_BUFFER Buffer_X;
  BOF_WEB_RPC_SOCKET_OPERATION_RESULT Result_X;

  BOF_WEB_RPC_SOCKET_WRITE_OP()
  {
    Reset();
  }
  void Reset()
  {
    TimeOut_U32 = 0;
    Timer_U32 = 0;
    Buffer_X.Reset();
    Result_X.Reset();
  }
};

#define BOF_LWS_ALLOCATE_BUFFER(buf, size)                \
  {                                                   \
    uint64_t remain;                                  \
    buf.SetStorage(LWS_PRE + size, LWS_PRE, nullptr); \
    buf.SeekAbs(LWS_PRE, remain);                     \
  }
#define BOF_LWS_WRITE_BUFFER(buf, len, p)              \
  {                                                \
    uint64_t nbwritten;                            \
    buf.Write(len, (const uint8_t *)p, nbwritten); \
    BOF_ASSERT(len == nbwritten);                  \
  }
// BOF_ASSERT(len == nbwritten);

#define BOF_LWS_SPRINT_BUFFER(buf, ...)                    \
  {                                                    \
    int len = sprintf((char *)buf.Pos(), __VA_ARGS__); \
    buf.Size_U64 += len;                               \
  }
#define BOF_LWS_CLEAR_BUFFER(buf, remain) \
  {                                   \
    buf.Clear();                      \
    buf.SeekAbs(LWS_PRE, remain);     \
  }
#define BOF_LWS_FREE_BUFFER(buf) \
  {                          \
    buf.ReleaseStorage();    \
    buf.Reset();             \
  }

// Use LWS_... macro just above to work with BOF::BOF_BUFFER &_rReply_X in V_HttpCallback and V_WsCallback
// Lws needs a buffer with an placeholder of LWS_PRE bytes at the beginning of the buffer
// If you need to answer, allocate the reply buffer in V_HttpCallback or V_WsCallback with
// LWS_REPLY_ALLOCATE_BUFFER(4080) or do nothing if no reply needed.
//  Populate this buffer using LWS_REPLY_WRITE or LWS_REPLY_SPRINT.
//  This buffer will be freed by the engine

// Per session data will be different for each connection.

class BofWebSocket;
struct BOF_WEB_SOCKET_DATA_PER_SESSION
{
  bool ConnectDone_B;
  struct lws *pWebSocket_X;
  BOF_WEB_SOCKET_DATA_PER_SESSION()
  {
    Reset();
  }
  void Reset()
  {
    ConnectDone_B = false;
    pWebSocket_X = nullptr;
  }
};

class BofWebSocket : BOF::BofThread
{
public:
  BofWebSocket(const BOF_WEB_SOCKET_PARAM &_rWebSocketParam_X);
  ~BofWebSocket();

  BOFERR ProgramWebSocketOperation(uint32_t _TimeOut_U32, BOF_WEB_RPC_SOCKET_EXIT_PARAM &_rParam_X, uint32_t &_rOpTicket_U32);
  BOFERR ProgramWebSocketOperation(uint32_t _TimeOut_U32, BOF_WEB_RPC_SOCKET_LISTEN_PARAM &_rParam_X, uint32_t &_rOpTicket_U32);
  BOFERR ProgramWebSocketOperation(uint32_t _TimeOut_U32, BOF_WEB_RPC_SOCKET_CONNECT_PARAM &_rParam_X, uint32_t &_rOpTicket_U32);
  BOFERR ProgramWebSocketOperation(uint32_t _TimeOut_U32, BOF_WEB_RPC_SOCKET_READ_PARAM &_rParam_X, uint32_t &_rOpTicket_U32);
  BOFERR ProgramWebSocketOperation(uint32_t _TimeOut_U32, BOF_WEB_RPC_SOCKET_WRITE_PARAM &_rParam_X, uint32_t &_rOpTicket_U32);
  BOFERR ProgramWebSocketOperation(uint32_t _TimeOut_U32, BOF_WEB_RPC_SOCKET_DISCONNECT_PARAM &_rParam_X, uint32_t &_rOpTicket_U32);
  BOFERR GetWebSocketOperationResult(uint32_t _TimeOut_U32, BOF_WEB_RPC_SOCKET_OPERATION_RESULT &_rOperationResult_X);
  uint32_t NumberOfWebSocketOperationWaiting();
  bool IsWebSocketOperationPending();
  uint32_t NumberOfWebSocketResultPending();
  BOFERR CancelWebSocketOperation(uint32_t _TimeOut_U32);
  BOFERR ClearWebSocketOperation();
  BOFERR ResetWebSocketOperation();

  BOFERR Run();
  BOFERR Connect(uint32_t TimeoutInMs_U32, const std::string &_rWsEndpoint_S, const std::string &_rWsCltKey_S);  // "ws ://10.129.171.112:8080";
  BOFERR Write(uint32_t TimeoutInMs_U32, uint32_t &_rNbToWrite_U32, uint8_t *_pData_U8);
  BOFERR Read(uint32_t TimeoutInMs_U32, uint32_t &_rNbMaxToRead_U32, uint8_t *_pData_U8);
  BOFERR Disconnect(uint32_t TimeoutInMs_U32);
  BOFERR Stop();
  uint32_t GetNumberOfClient();
  std::string GetClientList();

  int LwsCallback(struct lws *_pLws_X, enum lws_callback_reasons _LwsReason_E, void *_pLwsUser, void *_pInput, size_t _Len);

//  virtual BOFERR V_WebSocketCallback(WEB_SOCKET_LWS_CALLBACK_REASON _Reason_E, uint32_t _Nb_U32, uint8_t *_pData_U8, BOF::BOF_BUFFER &_rReply_X) = 0;
  // virtual BOFERR V_WsCallback(WEB_SOCKET_LWS_CALLBACK_REASON _Reason_E, const BOF::BOF_BUFFER &_rPayload_X, BOF::BOF_BUFFER &_rReply_X) = 0;

private:
  BOFERR V_OnProcessing() override;
  BOFERR AddClient(const std::string &_rWsCltKey_S, BOF_WEB_SOCKET_DATA_PER_SESSION *_pWebSocketDataPerSession_X);
  bool IsClientExist(const std::string &_rWsCltKey_S);
  BOFERR RemoveClient(const std::string &_rWsCltKey_S);
  BOFERR RemoveClient(BOF_WEB_SOCKET_DATA_PER_SESSION *_pWebSocketDataPerSession_X);
  BOFERR GetCloseInfo(struct lws *_pWsi_X, uint16_t &_rCloseStatus_U16, uint32_t &_rMaxSize_U32, char *_pClosingReason_c);
  BOFERR SetSocketBufferSize(struct lws *_pWsi_X, uint32_t &_rRcvBufferSize_U32, uint32_t &_rSndBufferSize_U32);

  BOF_WEB_SOCKET_PARAM mWebSocketParam_X;
  struct lws_context *mpLwsContext_X = nullptr;
  BOF_WEB_SOCKET_DATA_PER_SESSION mWebSocketState_X;
  std::mutex mMtxClientCollection;
  std::map<std::string, BOF_WEB_SOCKET_DATA_PER_SESSION> mClientCollection;

  uint32_t mTicket_U32 = 1;
  static uint32_t S_mWsCltId_U32;
  std::unique_ptr<BOF::BofCircularBuffer<BOF_WEB_RPC_SOCKET_OPERATION_PARAM>> mpuSocketOperationParamCollection = nullptr;   /*! The operation params */
  std::unique_ptr<BOF::BofCircularBuffer<BOF_WEB_RPC_SOCKET_WRITE_OP>> mpuSocketOperationWriteOpCollection = nullptr;        /*! The WRITE operation params */
  std::unique_ptr<BOF::BofCircularBuffer<BOF_WEB_RPC_SOCKET_OPERATION_RESULT>> mpuSocketOperationResultCollection = nullptr; /*! The operation result */
  std::unique_ptr<BOF::BofRawCircularBuffer> mpuRxBufferCollection = nullptr;                                            /*! The RxBuffer */

  std::atomic<bool> mCancel_B = false;
  std::atomic<bool> mOperationPending_B = false; // only cleared by ClearSocketOperation or CancelSocketOperation
  BOF::BOF_EVENT mCancelEvent_X;
  BOF_WEB_RPC_SOCKET_OPERATION_PARAM mCurrentOp_X;
  uint64_t mNbRxByServer_U64 = 0;
  uint64_t mNbTxByServer_U64 = 0;
  uint64_t mNbRxByClient_U64 = 0;
  uint64_t mNbTxByClient_U64 = 0;
};
END_WEBRPC_NAMESPACE()
