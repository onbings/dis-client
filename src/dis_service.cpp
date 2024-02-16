/*
 * Copyright (c) 2023-2033, Onbings. All rights reserved.
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * This module implements a dis service
 *
 * History:
 *
 * V 1.00  Jan 25 2024  Bernard HARMEL: b.harmel@evs.com : Initial release
 *
 * See data flow in comment at the end of this file
 *
 */
#include <bofstd/boffs.h>
#include "dis_client.h"

#include <bofstd/bofenum.h>
#include <bofstd/bofhttprequest.h>

uint32_t DisService::S_mSeqId_U32 = 1;

#define DIS_CLIENT_MAIN_LOOP_IDLE_TIME 100                          // 100ms We no state machine action during this time (keep imgui main loop cpu friendly)
#define DIS_CLIENT_WS_TIMEOUT (DIS_CLIENT_MAIN_LOOP_IDLE_TIME * 10) // 1000ms Timeout fo ws connect, read or write op
#define DIS_SERVICE_STATE_TIMEOUT (DIS_CLIENT_WS_TIMEOUT * 3)       // 3000ms Maximum time in a given state before returning to DIS_SERVICE_STATE_OPEN_WS
static_assert(DIS_CLIENT_WS_TIMEOUT >= (DIS_CLIENT_MAIN_LOOP_IDLE_TIME * 10), "DIS_CLIENT_WS_TIMEOUT must be >= (DIS_CLIENT_MAIN_LOOP_IDLE_TIME * 10)");
static_assert(DIS_SERVICE_STATE_TIMEOUT >= (DIS_CLIENT_WS_TIMEOUT * 3), "DIS_SERVICE_STATE_TIMEOUT must be >= (DIS_CLIENT_WS_TIMEOUT * 3 )");
#define DIS_CLIENT_RX_BUFFER_SIZE 0x40000
#define DIS_CLIENT_NB_MAX_BUFFER_ENTRY 32

static BOF::BofEnum<DIS_SERVICE_STATE> S_DisClientStateTypeEnumConverter(
    {
        {DIS_SERVICE_STATE::DIS_SERVICE_STATE_IDLE, "Idle"},
        {DIS_SERVICE_STATE::DIS_SERVICE_STATE_CREATE_WS, "CreateWs"},
        {DIS_SERVICE_STATE::DIS_SERVICE_STATE_OPEN_WS, "OpenWs"},
        {DIS_SERVICE_STATE::DIS_SERVICE_STATE_CONNECT, "Connect"},
        {DIS_SERVICE_STATE::DIS_SERVICE_STATE_GET_DIS_SERVICE, "GetDisService"},
        {DIS_SERVICE_STATE::DIS_SERVICE_STATE_GET_DEBUG_PAGE_LAYOUT, "GetDbgPageLayout"},
        {DIS_SERVICE_STATE::DIS_SERVICE_STATE_GET_DEBUG_PAGE_INFO, "GetDbgPageInfo"},
        {DIS_SERVICE_STATE::DIS_SERVICE_STATE_MAX, "Max"},
    },
    DIS_SERVICE_STATE::DIS_SERVICE_STATE_MAX);

#define DIS_CLIENT_END_OF_COMMAND(pDisService)     \
  {                                                \
    (pDisService)->LastCmdSentTimeoutInMs_U32 = 0; \
    (pDisService)->WaitForReplyId_U32 = 0;         \
  }

#if defined(__EMSCRIPTEN__)
EM_BOOL WebSocket_OnOpen(int _Event_i, const EmscriptenWebSocketOpenEvent *_pWebsocketEvent_X, void *_pUserData)
{
  EM_BOOL Rts_B = EM_FALSE;
  WS_CALLBBACK_PARAM *pWsCallbackParam_X = (WS_CALLBBACK_PARAM *)_pUserData;

  if ((pWsCallbackParam_X) && (pWsCallbackParam_X->pDisService))
  {
    Rts_B = (pWsCallbackParam_X->pDisService->OnWebSocketOpenEvent(pWsCallbackParam_X->pDisService_X, _pWebsocketEvent_X) == BOF_ERR_NO_ERROR) ? EM_TRUE : EM_FALSE;
  }
  return Rts_B;
}

EM_BOOL WebSocket_OnError(int _Event_i, const EmscriptenWebSocketErrorEvent *_pWebsocketEvent_X, void *_pUserData)
{
  EM_BOOL Rts_B = EM_FALSE;
  WS_CALLBBACK_PARAM *pWsCallbackParam_X = (WS_CALLBBACK_PARAM *)_pUserData;

  if ((pWsCallbackParam_X) && (pWsCallbackParam_X->pDisService))
  {
    Rts_B = (pWsCallbackParam_X->pDisService->OnWebSocketErrorEvent(pWsCallbackParam_X->pDisService_X, _pWebsocketEvent_X) == BOF_ERR_NO_ERROR) ? EM_TRUE : EM_FALSE;
  }
  return Rts_B;
}

EM_BOOL WebSocket_OnClose(int _Event_i, const EmscriptenWebSocketCloseEvent *_pWebsocketEvent_X, void *_pUserData)
{
  EM_BOOL Rts_B = EM_FALSE;
  WS_CALLBBACK_PARAM *pWsCallbackParam_X = (WS_CALLBBACK_PARAM *)_pUserData;
  if ((pWsCallbackParam_X) && (pWsCallbackParam_X->pDisService))
  {
    Rts_B = (pWsCallbackParam_X->pDisService->OnWebSocketCloseEvent(pWsCallbackParam_X->pDisService_X, _pWebsocketEvent_X) == BOF_ERR_NO_ERROR) ? EM_TRUE : EM_FALSE;
  }
  return Rts_B;
}
EM_BOOL WebSocket_OnMessage(int _Event_i, const EmscriptenWebSocketMessageEvent *_pWebsocketEvent_X, void *_pUserData)
{
  EM_BOOL Rts_B = EM_FALSE;
  WS_CALLBBACK_PARAM *pWsCallbackParam_X = (WS_CALLBBACK_PARAM *)_pUserData;
  if ((pWsCallbackParam_X) && (pWsCallbackParam_X->pDisService))
  {
    Rts_B = (pWsCallbackParam_X->pDisService->OnWebSocketMessageEvent(pWsCallbackParam_X->pDisService_X, _pWebsocketEvent_X) == BOF_ERR_NO_ERROR) ? EM_TRUE : EM_FALSE;
  }
  return Rts_B;
}

#endif

DisService::DisService(const DIS_SERVICE_PARAM &_rDisServiceParam_X)
{
  mDisServiceParam_X = _rDisServiceParam_X;
  mDisService_X.DisDevice_X = mDisServiceParam_X.DisDevice_X;
}

DisService::~DisService()
{
  CloseWebSocket(&mDisService_X);
}
const DIS_DBG_SERVICE &DisService::GetDbgDisService()
{
  return mDisService_X;
}
void DisService::ResetPageInfoTimer()
{
  mDisService_X.PageInfoTimer_U32 = 0;
}

BOFERR DisService::Run()
{
  BOFERR Rts_E = BOF_ERR_INVALID_STATE;

  if (mDisService_X.DisClientState_E == DIS_SERVICE_STATE::DIS_SERVICE_STATE_MAX)
  {
    Rts_E = BOF_ERR_NO_ERROR;
    mDisService_X.DisClientState_E = DIS_SERVICE_STATE::DIS_SERVICE_STATE_IDLE;
  }
  return Rts_E;
}

BOFERR DisService::Stop()
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;

  mDisService_X.DisClientState_E = DIS_SERVICE_STATE::DIS_SERVICE_STATE_MAX;
  return Rts_E;
}
/*
{
  "version": "1.0.0",
  "protocolInfo": "GET /DebugServiceRoute?seq=1",
  "route": [
    {
      "name": "/Gbe/",
      "ip": "127.0.0.1:2510"
    },
*/

BOFERR DisService::DecodeDisService(const nlohmann::json &_rDisServiceJsonData)
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;
  uint32_t i_U32;
  DIS_DBG_SERVICE_ITEM DisDbgServiceItem_X;

  try
  {
    auto DisServiceRouteIterator = _rDisServiceJsonData.find("route");
    if ((DisServiceRouteIterator != _rDisServiceJsonData.end()) && (DisServiceRouteIterator->is_array()))
    {
      mDisService_X.DisDbgServiceItemCollection.clear();
      i_U32 = 0;
      for (const auto &rRoute : *DisServiceRouteIterator)
      {
        DisDbgServiceItem_X.Name_S = rRoute.value("name", "Unknown");
        DisDbgServiceItem_X.IpAddress_S = rRoute.value("ip", "0.0.0.0:0");
        mDisService_X.DisDbgServiceItemCollection.push_back(DisDbgServiceItem_X);
        i_U32++;
      } // for (const auto &rRoute : *DisServiceRouteIterator)
    }   // if ((DisServiceIterator != _rDisServiceJsonData.end()) && (DisServiceIterator->is_array()))
  }
  catch (nlohmann::json::exception &re)
  {
    DisClient::S_Log("[%u] DecodeDisService: exception caught '%s'\n", BOF::Bof_GetMsTickCount(), re.what());
    Rts_E = BOF_ERR_FORMAT;
  }
  return Rts_E;
}

/*
"page": [
    {
      "label": "Gigabit - Configuration",
      "page": 800,
      "maxChnl": 0,
      "subPage": [
        {
          "label": "Ip configuration",
          "help": "<h1>This is a heading</h1><p>This is some paragraph text.</p>"
        },
*/

BOFERR DisService::DecodePageLayout(const nlohmann::json &_rPageLayoutJsonData, DIS_DBG_SERVICE_ITEM &_rDisDbgServiceItem)
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;
  uint32_t i_U32, j_U32;
  DIS_DBG_PAGE_LAYOUT DisDbgPage_X;
  DIS_DBG_SUB_PAGE_LAYOUT DisDbgSubPage_X;
  try
  {
    _rDisDbgServiceItem.DisDbgPageLayoutCollection.clear();
    // Check if "page" exists and is an array
    auto PageIterator = _rPageLayoutJsonData.find("page");
    if ((PageIterator != _rPageLayoutJsonData.end()) && (PageIterator->is_array()))
    {
      const auto &PageCollection = *PageIterator;
      for (i_U32 = 0; i_U32 < PageCollection.size(); i_U32++)
      {
        const auto &CrtPage = PageCollection[i_U32];
        DisDbgPage_X.Reset();
        DisDbgPage_X.Label_S = CrtPage.value("label", "Label_NotFound_" + std::to_string(i_U32));
        DisDbgPage_X.Page_U32 = CrtPage.value("page", 0);
        DisDbgPage_X.Channel_U32 = 0;
        DisDbgPage_X.MaxChannel_U32 = CrtPage.value("maxChnl", 0);
        DisDbgPage_X.DisDbgSubPageCollection.clear();
        auto SubPageIterator = CrtPage.find("subPage");
        if ((SubPageIterator != CrtPage.end()) && (SubPageIterator->is_array()))
        {
          const auto &SubPageCollection = *SubPageIterator;
          // DisClient::S_Log("SubPage JSON: %s", SubPageCollection.dump(2).c_str());
          for (j_U32 = 0; j_U32 < SubPageCollection.size(); j_U32++)
          {
            const auto &CrtSubPage = SubPageCollection[j_U32];
            // DisClient::S_Log("SubPage[%d] JSON: %s", j_U32, CrtSubPage.dump(2).c_str());
            DisDbgSubPage_X.Reset();
            DisDbgSubPage_X.Label_S = CrtSubPage.value("label", "Label_NotFound_" + std::to_string(j_U32)) + '[' + std::to_string(DisDbgPage_X.MaxChannel_U32) + ']';
            DisDbgSubPage_X.Help_S = CrtSubPage.value("help", "Help_NotFound_" + std::to_string(j_U32));
            DisDbgPage_X.DisDbgSubPageCollection.push_back(DisDbgSubPage_X);
          }
        } // if ((SubPageIterator != CrtPage.end()) && (SubPageIterator->is_array()))
        _rDisDbgServiceItem.DisDbgPageLayoutCollection.push_back(DisDbgPage_X);
      } // for (i = 0; i < pages.size(); ++i)
    }   // if ((PageIterator != _rPageLayoutJsonData.end()) && (PageIterator->is_array()))
  }
  catch (nlohmann::json::exception &re)
  {
    DisClient::S_Log("[%u] DecodePageLayout: exception caught '%s'\n", BOF::Bof_GetMsTickCount(), re.what());
    Rts_E = BOF_ERR_FORMAT;
  }
  return Rts_E;
}

/*
{
  "version": "1.0.0",
  "protocolInfo": "GET /Gbe/DebugPageInfo/800/0/?chnl=0&flag=3&key=None&seq=5&user_input",
  "background": {
    "bg": "#808080",
    "line": [
      {
        "x": 2,
        "y": 2,
        "bg": "#808080",
        "fg": "#FF0000",
        "text": "[r] Reset TcpIp stack"
      },
  },
  "foreground": {
    "bg": "#808080",
    "line": [
      {
        "x": 21,
        "y": 4,
        "bg": "#808080",
        "fg": "#0000FF",
        "text": "0x0004000A"
      },
*/

BOFERR DisService::DecodePageInfo(const nlohmann::json &_rPageInfoJsonData)
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;
  const nlohmann::json *pLineInfoJsonData;
  uint32_t i_U32;
  DIS_SERVICE_PAGE_INFO_LINE PageInfoLine_X;

  try
  {
    mDisService_X.PageInfo_X.Reset();
    mDisService_X.PageInfo_X.BackColor_S = _rPageInfoJsonData["background"].value("bg", "#000000");
    for (i_U32 = 0; i_U32 < 2; i_U32++) // i_U32==0: Draw background i_U32==1: Draw foreground
    {
      if (i_U32 == 0)
      {
        pLineInfoJsonData = &_rPageInfoJsonData["background"]["line"];
        mDisService_X.PageInfo_X.BackPageInfoLineCollection.clear();
      }
      else
      {
        pLineInfoJsonData = &_rPageInfoJsonData["foreground"]["line"];
        mDisService_X.PageInfo_X.ForePageInfoLineCollection.clear();
      }
      for (const auto &rLine : (*pLineInfoJsonData))
      {
        PageInfoLine_X.x_U32 = rLine.value("x", 0);
        PageInfoLine_X.y_U32 = rLine.value("y", 0);
        PageInfoLine_X.ForeColor_S = rLine.value("fg", "#FFFFFF");
        PageInfoLine_X.BackColor_S = rLine.value("bg", "#000000");
        PageInfoLine_X.Text_S = rLine.value("text", "");
        if (i_U32 == 0)
        {
          mDisService_X.PageInfo_X.BackPageInfoLineCollection.push_back(PageInfoLine_X);
        }
        else
        {
          mDisService_X.PageInfo_X.ForePageInfoLineCollection.push_back(PageInfoLine_X);
        }
      }
    }
  }
  catch (nlohmann::json::exception &re)
  {
    DisClient::S_Log("[%u] DecodePageInfo: exception caught '%s'\n", BOF::Bof_GetMsTickCount(), re.what());
    Rts_E = BOF_ERR_FORMAT;
  }
  return Rts_E;
}

DIS_SERVICE_STATE DisService::StateMachine(const DIS_DBG_STATE_MACHINE &_rStateMachine_X)
{
  BOFERR Sts_E, StsCmd_E;
  bool Rts_B = false;
  uint32_t DeltaState_U32, DeltaPageInfo_U32;
  BOF::BOF_RAW_CIRCULAR_BUFFER_PARAM RawCircularBufferParam_X;

#if defined(__EMSCRIPTEN__)
#else
  BOFWEBRPC::BOF_WEB_SOCKET_PARAM WebSocketParam_X;
#endif

  try
  {
    if (mDisService_X.DisClientState_E != mDisService_X.DisClientLastState_E)
    {
      DisClient::S_Log("[%u] ENTER: %s Sz %zd State change %s->%s: %s\n", BOF::Bof_GetMsTickCount(), _rStateMachine_X.DisDevice_X.MetaData_X.DeviceUniqueKey_S.c_str(), mDisService_X.DisDbgServiceItemCollection.size(),
                       S_DisClientStateTypeEnumConverter.ToString(mDisService_X.DisClientLastState_E).c_str(),
                       S_DisClientStateTypeEnumConverter.ToString(mDisService_X.DisClientState_E).c_str(), mDisService_X.IsWebSocketConnected_B ? "Connected" : "Disconnected");
      mDisService_X.DisClientLastState_E = mDisService_X.DisClientState_E;
      mDisService_X.StateTimer_U32 = BOF::Bof_GetMsTickCount();
    }
    DeltaState_U32 = BOF::Bof_ElapsedMsTime(mDisService_X.StateTimer_U32);
    DeltaPageInfo_U32 = BOF::Bof_ElapsedMsTime(mDisService_X.PageInfoTimer_U32);
    Sts_E = BOF_ERR_NO_ERROR;
    StsCmd_E = BOF_ERR_NO;

    if (mDisService_X.IsWebSocketConnected_B)
    {
      StsCmd_E = IsCommandRunning(&mDisService_X) ? CheckForCommandReply(&mDisService_X, mDisService_X.LastWebSocketMessage_S) : BOF_ERR_NO;
    }
    switch (mDisService_X.DisClientState_E)
    {
      case DIS_SERVICE_STATE::DIS_SERVICE_STATE_IDLE:
        RawCircularBufferParam_X.MultiThreadAware_B = true;
        RawCircularBufferParam_X.SlotMode_B = false;
        RawCircularBufferParam_X.AlwaysContiguous_B = true;
        RawCircularBufferParam_X.BufferSizeInByte_U32 = DIS_CLIENT_RX_BUFFER_SIZE;
        RawCircularBufferParam_X.NbMaxBufferEntry_U32 = DIS_CLIENT_NB_MAX_BUFFER_ENTRY;
        RawCircularBufferParam_X.pData_U8 = nullptr;
        RawCircularBufferParam_X.Overwrite_B = false;
        RawCircularBufferParam_X.Blocking_B = true;
        mDisService_X.puRxBufferCollection = std::make_unique<BOF::BofRawCircularBuffer>(RawCircularBufferParam_X);
        if ((mDisService_X.puRxBufferCollection) && (mDisService_X.puRxBufferCollection->LastErrorCode() == BOF_ERR_NO_ERROR))
        {
          mDisService_X.DisClientState_E = DIS_SERVICE_STATE::DIS_SERVICE_STATE_CREATE_WS;
        }
        break;

      case DIS_SERVICE_STATE::DIS_SERVICE_STATE_CREATE_WS:
        mDisService_X.WsCbParam_X.pDisService = this;
        mDisService_X.WsCbParam_X.pDisService_X = &mDisService_X;
        DisClient::S_Log("DIS_SERVICE_STATE_CREATE_WS ptr mDisService_X %p pDisService %p pDisService_X %p\n", &mDisService_X, mDisService_X.WsCbParam_X.pDisService, mDisService_X.WsCbParam_X.pDisService_X);

#if defined(__EMSCRIPTEN__)
#else
        WebSocketParam_X.pUser = mDisService_X.WsCbParam_X.pDisService_X;
        WebSocketParam_X.NbMaxOperationPending_U32 = 4;
        WebSocketParam_X.RxBufferSize_U32 = DIS_CLIENT_RX_BUFFER_SIZE;
        WebSocketParam_X.NbMaxBufferEntry_U32 = DIS_CLIENT_NB_MAX_BUFFER_ENTRY;
        WebSocketParam_X.OnOperation = nullptr;
        WebSocketParam_X.OnOpen = BOF_BIND_1_ARG_TO_METHOD(this, DisService::OnWebSocketOpenEvent);
        WebSocketParam_X.OnClose = BOF_BIND_1_ARG_TO_METHOD(this, DisService::OnWebSocketCloseEvent);
        WebSocketParam_X.OnError = BOF_BIND_2_ARG_TO_METHOD(this, DisService::OnWebSocketErrorEvent);
        WebSocketParam_X.OnMessage = BOF_BIND_4_ARG_TO_METHOD(this, DisService::OnWebSocketMessageEvent);
        WebSocketParam_X.NbMaxClient_U32 = 0; // 0: client
        // WebSocketParam_X.ServerIp_X = BOF::BOF_SOCKET_ADDRESS(mDisClientParam_X.DisServerEndpoint_S);
        WebSocketParam_X.WebSocketThreadParam_X.Name_S = _rStateMachine_X.DisDevice_X.MetaData_X.DeviceUniqueKey_S;
        WebSocketParam_X.WebSocketThreadParam_X.ThreadSchedulerPolicy_E = BOF::BOF_THREAD_SCHEDULER_POLICY::BOF_THREAD_SCHEDULER_POLICY_OTHER;
        WebSocketParam_X.WebSocketThreadParam_X.ThreadPriority_E = BOF::BOF_THREAD_PRIORITY::BOF_THREAD_PRIORITY_000;
        mDisService_X.puDisClientWebSocket = std::make_unique<DisClientWebSocket>(WebSocketParam_X);
        if (mDisService_X.puDisClientWebSocket)
        {
          mDisService_X.puDisClientWebSocket->Run();
          mDisService_X.DisClientState_E = DIS_SERVICE_STATE::DIS_SERVICE_STATE_OPEN_WS;
#if 0
              BOFERR Sts_E;
              uint8_t pData_U8[0x2000];
              uint32_t Nb_U32, j_U32;
              Sts_E = mpuDisClientWebSocket->Connect(DIS_CLIENT_WEB_TIMEOUT, mDisClientParam_X.DisServerEndpoint_S, "DisClientWs");
              //Nb_U32 = sizeof(pData_U8);
              //Sts_E = mpuDisClientWebSocket->Read(DIS_CLIENT_WEB_TIMEOUT, Nb_U32, pData_U8);

              for (j_U32 = 0; j_U32 < 10; j_U32++)
              {
                Nb_U32 = sprintf((char *)pData_U8, "GET /DebugServiceRoute?seq=1");
                Sts_E = mpuDisClientWebSocket->Write(DIS_CLIENT_WEB_TIMEOUT, Nb_U32, pData_U8);
                Nb_U32 = sizeof(pData_U8);
                Sts_E = mpuDisClientWebSocket->Read(DIS_CLIENT_WEB_TIMEOUT, Nb_U32, pData_U8);
                //        BOF::Bof_MsSleep(250);
              }
              //BOF::Bof_MsSleep(9999999);
              Sts_E = mpuDisClientWebSocket->Disconnect(DIS_CLIENT_WEB_TIMEOUT);
#endif
        }
        else
        {
          mDisService_X.DisClientState_E = DIS_SERVICE_STATE::DIS_SERVICE_STATE_IDLE;
        }
#endif
        break;

      case DIS_SERVICE_STATE::DIS_SERVICE_STATE_OPEN_WS:
        Sts_E = OpenWebSocket(&mDisService_X);
        if (Sts_E == BOF_ERR_NO_ERROR)
        {
          mDisService_X.DisClientState_E = DIS_SERVICE_STATE::DIS_SERVICE_STATE_CONNECT;
        }
        break;

      case DIS_SERVICE_STATE::DIS_SERVICE_STATE_CONNECT:
        DisClient::S_Log("DIS_SERVICE_STATE_CONNECT ptr %p Ws %d IsWebSocketConnected_B %d\n", &mDisService_X, mDisService_X.Ws, mDisService_X.IsWebSocketConnected_B);
        if (mDisService_X.IsWebSocketConnected_B)
        {
          mDisService_X.DisClientState_E = DIS_SERVICE_STATE::DIS_SERVICE_STATE_GET_DIS_SERVICE;
        }
        break;

      case DIS_SERVICE_STATE::DIS_SERVICE_STATE_GET_DIS_SERVICE:
        if (StsCmd_E == BOF_ERR_NO)
        {
          mDisService_X.DisDbgServiceItemCollection.clear();
          Sts_E = SendCommand(&mDisService_X, DIS_CLIENT_WS_TIMEOUT, "GET /DebugServiceRoute");
          if (Sts_E != BOF_ERR_NO_ERROR)
          {
            mDisService_X.DisClientState_E = DIS_SERVICE_STATE::DIS_SERVICE_STATE_OPEN_WS;
          }
        }
        else
        {
          if (StsCmd_E == BOF_ERR_NO_ERROR)
          {
            Sts_E = DecodeDisService(nlohmann::json::parse(mDisService_X.LastWebSocketMessage_S));
            if (Sts_E == BOF_ERR_NO_ERROR)
            {
              mDisService_X.DisDbgServicePageLayoutIndex_U32 = 0;
              mDisService_X.DisClientState_E = DIS_SERVICE_STATE::DIS_SERVICE_STATE_GET_DEBUG_PAGE_LAYOUT;
            }
            else
            {
              mDisService_X.DisClientState_E = DIS_SERVICE_STATE::DIS_SERVICE_STATE_OPEN_WS;
            }
          }
        }
        break;

      case DIS_SERVICE_STATE::DIS_SERVICE_STATE_GET_DEBUG_PAGE_LAYOUT:
        // DisClient::S_Log("DIS_SERVICE_STATE_GET_DEBUG_PAGE_LAYOUT %d/%d\n",mDisService_X.DisDbgServicePageLayoutIndex_U32,mDisService_X.DisDbgServiceItemCollection.size());
        if (mDisService_X.DisDbgServicePageLayoutIndex_U32 < mDisService_X.DisDbgServiceItemCollection.size())
        {
          if (StsCmd_E == BOF_ERR_NO)
          {
            if (_rStateMachine_X.DisService_S == "")
            {
              mDisService_X.StateTimer_U32 = BOF::Bof_GetMsTickCount(); // Reset state timer as all is ok, we just wait for user selection
            }
            else
            {
              Sts_E = SendCommand(&mDisService_X, DIS_CLIENT_WS_TIMEOUT, "GET " + _rStateMachine_X.DisService_S + "DebugPageLayout");
              if (Sts_E != BOF_ERR_NO_ERROR)
              {
                mDisService_X.DisClientState_E = DIS_SERVICE_STATE::DIS_SERVICE_STATE_OPEN_WS;
              }
            }
          }
          else
          {
            if (StsCmd_E == BOF_ERR_NO_ERROR)
            {
              Sts_E = DecodePageLayout(nlohmann::json::parse(mDisService_X.LastWebSocketMessage_S), mDisService_X.DisDbgServiceItemCollection[mDisService_X.DisDbgServicePageLayoutIndex_U32]);
              if (Sts_E == BOF_ERR_NO_ERROR)
              {
                mDisService_X.DisDbgServicePageLayoutIndex_U32++;
              }
              else
              {
                mDisService_X.DisClientState_E = DIS_SERVICE_STATE::DIS_SERVICE_STATE_OPEN_WS;
              }
              // DisClient::S_Log("DIS_SERVICE_STATE_GET_DEBUG_PAGE_LAYOUT %d/%d Sts %d\n", mDisService_X.DisDbgServicePageLayoutIndex_U32, mDisService_X.DisDbgServiceItemCollection.size(), Sts_E);
            }
          }
        }
        else
        {
          mDisService_X.DisClientState_E = DIS_SERVICE_STATE::DIS_SERVICE_STATE_GET_DEBUG_PAGE_INFO;
        }
        break;

      case DIS_SERVICE_STATE::DIS_SERVICE_STATE_GET_DEBUG_PAGE_INFO:
        if (StsCmd_E == BOF_ERR_NO)
        {
          if (DeltaPageInfo_U32 > mDisServiceParam_X.QueryServerPollTimeInMs_U32)
          {
            if ((_rStateMachine_X.Page_U32 == -1) && (_rStateMachine_X.SubPage_U32 == -1))
            {
              mDisService_X.StateTimer_U32 = BOF::Bof_GetMsTickCount(); // Reset state timer as all is ok, we just wait for user selection
            }
            else
            {
              mDisService_X.PageInfoTimer_U32 = BOF::Bof_GetMsTickCount();
              //"GET /Gbe/DebugPageInfo/800/0/?chnl=0&flag=3&key=0&user_input=");
              Sts_E = SendCommand(&mDisService_X, DIS_CLIENT_WS_TIMEOUT, "GET " + _rStateMachine_X.DisService_S + "DebugPageInfo/" + std::to_string(_rStateMachine_X.Page_U32) + "/" + std::to_string(_rStateMachine_X.SubPage_U32) + "/?" + "chnl=" + std::to_string(_rStateMachine_X.Channel_U32) + "&flag=" + std::to_string(_rStateMachine_X.CtrlFlag_U32) + "&key=" + std::to_string(_rStateMachine_X.FctKeyFlag_U32) + "&user_input=" + _rStateMachine_X.UserInput_S);
              if (Sts_E != BOF_ERR_NO_ERROR)
              {
                mDisService_X.DisClientState_E = DIS_SERVICE_STATE::DIS_SERVICE_STATE_OPEN_WS;
              }
            }
          }
        }
        else
        {
          if (StsCmd_E == BOF_ERR_NO_ERROR)
          {
            mDisService_X.StateTimer_U32 = BOF::Bof_GetMsTickCount(); // Reset state timer as all is ok, we just receive an answer

            if ((_rStateMachine_X.CtrlFlag_U32 & (DIS_CTRL_FLAG_BACK | DIS_CTRL_FLAG_FORE)) == (DIS_CTRL_FLAG_BACK | DIS_CTRL_FLAG_FORE))
            {
              Sts_E = DecodePageInfo(nlohmann::json::parse(mDisService_X.LastWebSocketMessage_S));
              // DisClient::S_Log("[%u] NewPageData Sts %d\n%s\n", BOF::Bof_GetMsTickCount(), Sts_E, mDisService_X.LastWebSocketMessage_S.c_str());

              if (Sts_E == BOF_ERR_NO_ERROR)
              {
                mDisService_X.DisClientState_E = DIS_SERVICE_STATE::DIS_SERVICE_STATE_GET_DEBUG_PAGE_INFO;
              }
              else
              {
                mDisService_X.DisClientState_E = DIS_SERVICE_STATE::DIS_SERVICE_STATE_OPEN_WS;
              }
            }
          }
        }
        break;

      default:
        break;
    }

    if (mDisService_X.DisClientState_E != mDisService_X.DisClientLastState_E)
    {
      DisClient::S_Log("[%u] LEAVE: %s Sz %zd State change %s->%s: %s\n", BOF::Bof_GetMsTickCount(), _rStateMachine_X.DisDevice_X.MetaData_X.DeviceUniqueKey_S.c_str(), mDisService_X.DisDbgServiceItemCollection.size(),
                       S_DisClientStateTypeEnumConverter.ToString(mDisService_X.DisClientLastState_E).c_str(),
                       S_DisClientStateTypeEnumConverter.ToString(mDisService_X.DisClientState_E).c_str(), mDisService_X.IsWebSocketConnected_B ? "Connected" : "Disconnected");
      mDisService_X.DisClientLastState_E = mDisService_X.DisClientState_E;
      mDisService_X.StateTimer_U32 = BOF::Bof_GetMsTickCount();
    }
    if (DeltaState_U32 > DIS_SERVICE_STATE_TIMEOUT)
    {
      mDisService_X.DisClientState_E = DIS_SERVICE_STATE::DIS_SERVICE_STATE_OPEN_WS;
      DisClient::S_Log("[%u] ERROR: %s Sz %zd State timeout (%d/%d)  %s->%s: %s\n", BOF::Bof_GetMsTickCount(), _rStateMachine_X.DisDevice_X.MetaData_X.DeviceUniqueKey_S.c_str(), mDisService_X.DisDbgServiceItemCollection.size(),
                       DeltaState_U32, DIS_SERVICE_STATE_TIMEOUT, S_DisClientStateTypeEnumConverter.ToString(mDisService_X.DisClientLastState_E).c_str(),
                       S_DisClientStateTypeEnumConverter.ToString(mDisService_X.DisClientState_E).c_str(), mDisService_X.IsWebSocketConnected_B ? "Connected" : "Disconnected");
      DIS_CLIENT_END_OF_COMMAND(&mDisService_X);
      StsCmd_E = BOF_ERR_ENOTCONN;
      mDisService_X.StateTimer_U32 = BOF::Bof_GetMsTickCount();
    }
  }
  catch (std::exception &re)
  {
    DisClient::S_Log("[%u] V_PreNewFrame: %zd exception caught '%s'\n", BOF::Bof_GetMsTickCount(), mDisService_X.DisDbgServiceItemCollection.size(), re.what());
  }
  return mDisService_X.DisClientState_E;
}

bool DisService::IsCommandRunning(DIS_DBG_SERVICE *_pDisService_X)
{
  bool Rts_B = false;
  uint32_t Delta_U32;

  if ((_pDisService_X) && (_pDisService_X->IsWebSocketConnected_B))
  {
    if (_pDisService_X->LastCmdSentTimeoutInMs_U32)
    {
      Delta_U32 = BOF::Bof_ElapsedMsTime(_pDisService_X->LastCmdSentTimer_U32);
      if (Delta_U32 >= _pDisService_X->LastCmdSentTimeoutInMs_U32)
      {
        DIS_CLIENT_END_OF_COMMAND(_pDisService_X); // Cancel previous one
      }
    }
    if (_pDisService_X->LastCmdSentTimeoutInMs_U32)
    {
      Rts_B = true;
    }
  }
  return Rts_B;
}
BOFERR DisService::SendCommand(DIS_DBG_SERVICE *_pDisService_X, uint32_t _TimeoutInMsForReply_U32, const std::string &_rCmd_S)
{
  BOFERR Rts_E = BOF_ERR_ENOTCONN;
  std::string Command_S;

  if ((_pDisService_X) && (!IsCommandRunning(_pDisService_X)))
  {
    Rts_E = BOF_ERR_EINVAL;
    if (_TimeoutInMsForReply_U32)
    {
      _pDisService_X->WaitForReplyId_U32 = S_mSeqId_U32;
      BOF_INC_TICKET_NUMBER(S_mSeqId_U32);

      if (_rCmd_S.find('?') != std::string::npos)
      {
        Command_S = _rCmd_S + "&seq=" + std::to_string(_pDisService_X->WaitForReplyId_U32);
      }
      else
      {
        Command_S = _rCmd_S + "?seq=" + std::to_string(_pDisService_X->WaitForReplyId_U32);
      }
      _pDisService_X->LastCmdSentTimer_U32 = BOF::Bof_GetMsTickCount();
      Rts_E = WriteWebSocket(_pDisService_X, Command_S.c_str());
      // DisClient::S_Log("%u %s==>Send '%s' sts %s\n", BOF::Bof_GetMsTickCount(), _pDisService_X->DisDevice_X.DeviceUniqueKey_S.c_str(), Command_S.c_str(), BOF::Bof_ErrorCode(Rts_E));
      if (Rts_E == BOF_ERR_NO_ERROR)
      {
        _pDisService_X->LastCmdSentTimeoutInMs_U32 = _TimeoutInMsForReply_U32;
      }
    }
  }
  return Rts_E;
}
BOFERR DisService::CheckForCommandReply(DIS_DBG_SERVICE *_pDisService_X, std::string &_rReply_S)
{
  BOFERR Rts_E = BOF_ERR_ENOTCONN;
  std::string Command_S;
  nlohmann::json CommandJsonData;
  uint32_t MaxSize_U32, Seq_U32, NbOfSubDir_U32;
  char pData_c[0x8000];
  std::map<std::string, std::string> QueryParamCollection;
  std::string ProtocolInfo_S;
  BOF::BofHttpRequest HttpRequest;
  BOF::BOF_HTTP_REQUEST_TYPE Method_E;

  if ((_pDisService_X) && (IsCommandRunning(_pDisService_X)))
  {
    MaxSize_U32 = sizeof(pData_c) - 1;
    Rts_E = ReadWebSocket(_pDisService_X, MaxSize_U32, pData_c);
    // DisClient::S_Log("%u %s=1=>Chk Sts %s Max %d\n", BOF::Bof_GetMsTickCount(), _pDisService_X->DisDevice_X.DeviceUniqueKey_S.c_str(),BOF::Bof_ErrorCode(Rts_E), MaxSize_U32);
    if (Rts_E == BOF_ERR_NO_ERROR)
    {
      Rts_E = BOF_ERR_EMPTY;
      if (MaxSize_U32)
      {
        pData_c[MaxSize_U32] = 0;
        Rts_E = BOF_ERR_EBADF;
        try
        {
          // DisClient::S_Log("%u %s=2=>Chk\n%s\n", BOF::Bof_GetMsTickCount(), _pDisService_X->DisDevice_X.DeviceUniqueKey_S.c_str(), pData_c);
          // DisClient::S_Log("%u %s=2=>Chk DataOk\n", BOF::Bof_GetMsTickCount(), _pDisService_X->DisDevice_X.DeviceUniqueKey_S.c_str());
          CommandJsonData = nlohmann::json::parse(pData_c);
          auto It = CommandJsonData.find("protocolInfo");
          if (It != CommandJsonData.end())
          {
            ProtocolInfo_S = *It;
            HttpRequest = ProtocolInfo_S;
            QueryParamCollection = HttpRequest.QueryParamCollection();
            Seq_U32 = BOF::Bof_StrToBin(10, QueryParamCollection["seq"].c_str());
            Method_E = BOF::BofHttpRequest::S_RequestType((const char *)pData_c);
            NbOfSubDir_U32 = HttpRequest.Path().NumberOfSubDirectory();
            // DisClient::S_Log("ProtoInfo %s Seq %d wait %d Meth %d NbSub %d\n", ProtocolInfo_S.c_str(), Seq_U32, mWaitForReplyId_U32, Method_E, NbOfSubDir_U32);

            switch (Method_E)
            {
              case BOF::BOF_HTTP_REQUEST_TYPE::BOF_HTTP_REQUEST_GET:
                break;

              default:
                break;
            }
            if (Seq_U32 == _pDisService_X->WaitForReplyId_U32)
            {
              _rReply_S = pData_c;
              DIS_CLIENT_END_OF_COMMAND(_pDisService_X);
              Rts_E = BOF_ERR_NO_ERROR;
            }
          }
        }
        catch (nlohmann::json::exception &re)
        {
          Rts_E = BOF_ERR_INVALID_ANSWER;
          DisClient::S_Log("[%u] %s CheckForCommandReply: exception caught '%s'\n", BOF::Bof_GetMsTickCount(), _pDisService_X->DisDevice_X.MetaData_X.DeviceUniqueKey_S.c_str(), re.what());
        }
      }
    }
  }
  return Rts_E;
}
BOFERR DisService::ReadWebSocket(DIS_DBG_SERVICE *_pDisService_X, uint32_t &_rMaxSize_U32, char *_pData_c)
{
  BOFERR Rts_E = BOF_ERR_INTERNAL;

  if (_pDisService_X)
  {
    Rts_E = _pDisService_X->puRxBufferCollection->PopBuffer(0, &_rMaxSize_U32, (uint8_t *)_pData_c);
  }
  return Rts_E;
}

#if defined(__EMSCRIPTEN__)

BOFERR DisService::OpenWebSocket(DIS_DBG_SERVICE *_pDisService_X)
{
  BOFERR Rts_E = BOF_ERR_INTERNAL;
  // EmscriptenWebSocketCreateAttributes WebSocketCreateAttribute_X = {"wss://echo.websocket.org", nullptr, EM_TRUE};
  // EmscriptenWebSocketCreateAttributes WebSocketCreateAttribute_X = {"ws://10.129.171.112:8080", nullptr, EM_TRUE};
  if (_pDisService_X)
  {
    Rts_E = BOF_ERR_NOT_SUPPORTED;
    EmscriptenWebSocketCreateAttributes WebSocketCreateAttribute_X = {mDisServiceParam_X.DisServerEndpoint_S.c_str(), nullptr, EM_TRUE};

    if (emscripten_websocket_is_supported())
    {
      CloseWebSocket(_pDisService_X);
      _pDisService_X->Ws = emscripten_websocket_new(&WebSocketCreateAttribute_X);

      DisClient::S_Log("CREATE Ptr %p Ws %d on '%s'\n", _pDisService_X, _pDisService_X->Ws, mDisServiceParam_X.DisServerEndpoint_S.c_str());
      if (_pDisService_X->Ws > 0)
      {
        emscripten_websocket_set_onopen_callback(_pDisService_X->Ws, &_pDisService_X->WsCbParam_X, WebSocket_OnOpen);
        emscripten_websocket_set_onerror_callback(_pDisService_X->Ws, &_pDisService_X->WsCbParam_X, WebSocket_OnError);
        emscripten_websocket_set_onclose_callback(_pDisService_X->Ws, &_pDisService_X->WsCbParam_X, WebSocket_OnClose);
        emscripten_websocket_set_onmessage_callback(_pDisService_X->Ws, &_pDisService_X->WsCbParam_X, WebSocket_OnMessage);
        Rts_E = BOF_ERR_NO_ERROR;
      }
    }
  }
  return Rts_E;
}
BOFERR DisService::CloseWebSocket(DIS_DBG_SERVICE *_pDisService_X)
{
  BOFERR Rts_E = BOF_ERR_INTERNAL;
  EMSCRIPTEN_RESULT Sts;

  if (_pDisService_X)
  {
    Rts_E = BOF_ERR_CLOSE;
    _pDisService_X->puRxBufferCollection->Reset();
    /*
    Here are some of the standard WebSocket close codes defined by the WebSocket protocol (RFC 6455):

    1000 (NORMAL_CLOSURE): Normal closure; the connection successfully completed its purpose.
    1001 (GOING_AWAY): The endpoint is going away, such as a server going down or a browser navigating away from the page.
    1002 (PROTOCOL_ERROR): The endpoint received an invalid or inconsistent WebSocket protocol data.
    1003 (UNSUPPORTED_DATA): The received data violates the WebSocket protocol.
    1005 (NO_STATUS_RECEIVED): Indicates that no status code was actually present in the close frame.
    1006 (ABNORMAL_CLOSURE): The connection was closed abnormally (without sending or receiving a close frame).
    */
    Sts = emscripten_websocket_close(_pDisService_X->Ws, 1000, "CloseWebSocket");
    DisClient::S_Log("STOP Ws %d code %d res %d\n", _pDisService_X->Ws, 1000, Sts);
    // NO if (Sts == 0)
    {
      _pDisService_X->Ws = -1;
      _pDisService_X->IsWebSocketConnected_B = false;
      Rts_E = BOF_ERR_NO_ERROR;
    }
  }
  return Rts_E;
}
BOFERR DisService::WriteWebSocket(DIS_DBG_SERVICE *_pDisService_X, const char *_pData_c)
{
  BOFERR Rts_E = BOF_ERR_INTERNAL;
  EMSCRIPTEN_RESULT Sts;

  if ((_pDisService_X) && (_pData_c))
  {
    Rts_E = BOF_ERR_WRITE;
    Sts = emscripten_websocket_send_utf8_text(_pDisService_X->Ws, _pData_c);
    if (Sts == 0)
    {
      Rts_E = BOF_ERR_NO_ERROR;
    }
  }
  return Rts_E;
}

BOFERR DisService::OnWebSocketOpenEvent(void *_pWsCbParam, const EmscriptenWebSocketOpenEvent *_pWebsocketEvent_X)
{
  BOFERR Rts_E = BOF_ERR_EINVAL;
  DIS_DBG_SERVICE *pDisService_X = (DIS_DBG_SERVICE *)_pWsCbParam;

  if ((pDisService_X) && (_pWebsocketEvent_X))
  {
    DisClient::S_Log("AVANT OnWebSocketOpenEvent ptr %p Ws %d IsWebSocketConnected_B %d\n", pDisService_X, pDisService_X->Ws, pDisService_X->IsWebSocketConnected_B);

    pDisService_X->IsWebSocketConnected_B = true;

    DisClient::S_Log("APRES OnWebSocketOpenEvent ptr %p Ws %d IsWebSocketConnected_B %d\n", pDisService_X, pDisService_X->Ws, pDisService_X->IsWebSocketConnected_B);

    Rts_E = BOF_ERR_NO_ERROR;
  }
  return Rts_E;
}
BOFERR DisService::OnWebSocketErrorEvent(void *_pWsCbParam, const EmscriptenWebSocketErrorEvent *_pWebsocketEvent_X)
{
  BOFERR Rts_E = BOF_ERR_INTERNAL;
  DIS_DBG_SERVICE *pDisService_X = (DIS_DBG_SERVICE *)_pWsCbParam;

  if ((pDisService_X) && (_pWebsocketEvent_X))
  {
    Rts_E = CloseWebSocket(pDisService_X);
    Rts_E = BOF_ERR_NO_ERROR;
  }
  return Rts_E;
}
BOFERR DisService::OnWebSocketCloseEvent(void *_pWsCbParam, const EmscriptenWebSocketCloseEvent *_pWebsocketEvent_X)
{
  BOFERR Rts_E = BOF_ERR_INTERNAL;
  DIS_DBG_SERVICE *pDisService_X = (DIS_DBG_SERVICE *)_pWsCbParam;

  if ((pDisService_X) && (_pWebsocketEvent_X))
  {
    pDisService_X->IsWebSocketConnected_B = false;
    pDisService_X->Ws = -1;
    DIS_CLIENT_END_OF_COMMAND(pDisService_X);
    Rts_E = BOF_ERR_NO_ERROR;
  }
  return Rts_E;
}
BOFERR DisService::OnWebSocketMessageEvent(void *_pWsCbParam, const EmscriptenWebSocketMessageEvent *_pWebsocketEvent_X)
{
  BOFERR Rts_E = BOF_ERR_INTERNAL;
  DIS_DBG_SERVICE *pDisService_X = (DIS_DBG_SERVICE *)_pWsCbParam;
  EMSCRIPTEN_RESULT Sts;
  BOF::BOF_RAW_BUFFER *pRawBuffer_X;
  bool FinalFragment_B;

  if ((pDisService_X) && (_pWebsocketEvent_X))
  {
    if (_pWebsocketEvent_X->isText)
    {
      // For only ascii chars.
      // DisClient::S_Log("----------------->TXT message from %d: %d:%s\n", _pWebsocketEvent_X->socket, _pWebsocketEvent_X->numBytes, _pWebsocketEvent_X->data);
    }
    pDisService_X->puRxBufferCollection->SetAppendMode(0, true, nullptr); // By default we append
    FinalFragment_B = true;
    Rts_E = pDisService_X->puRxBufferCollection->PushBuffer(0, _pWebsocketEvent_X->numBytes, (const uint8_t *)_pWebsocketEvent_X->data, &pRawBuffer_X);
    pDisService_X->puRxBufferCollection->SetAppendMode(0, !FinalFragment_B, &pRawBuffer_X); // If it was the final one, we stop append->Result is pushed and committed
    // DisClient::S_Log("err %d data %d:%p Rts:indx %d slot %x 1: %x:%p 2: %x:%p\n", Rts_E, _pWebsocketEvent_X->numBytes, (const uint8_t *)_pWebsocketEvent_X->data, pRawBuffer_X->IndexInBuffer_U32, pRawBuffer_X->SlotEmpySpace_U32, pRawBuffer_X->Size1_U32, pRawBuffer_X->pData1_U8, pRawBuffer_X->Size2_U32, pRawBuffer_X->pData2_U8);
    if (Rts_E != BOF_ERR_NO_ERROR)
    {
      CloseWebSocket(pDisService_X);
    }
  }
  return Rts_E;
}
#else
BOFERR DisService::OpenWebSocket(DIS_DBG_SERVICE *_pDisService_X)
{
  BOFERR Rts_E = BOF_ERR_INIT;
  if ((_pDisService_X) && (_pDisService_X->puDisClientWebSocket))
  {
    Rts_E = _pDisService_X->puDisClientWebSocket->ResetWebSocketOperation();
    Rts_E = _pDisService_X->puDisClientWebSocket->Connect(DIS_CLIENT_WS_TIMEOUT, mDisServiceParam_X.DisServerEndpoint_S, _pDisService_X->DisDevice_X.MetaData_X.DeviceUniqueKey_S);
  }
  return Rts_E;
}
BOFERR DisService::CloseWebSocket(DIS_DBG_SERVICE *_pDisService_X)
{
  BOFERR Rts_E = BOF_ERR_INIT;
  if ((_pDisService_X) && (_pDisService_X->puDisClientWebSocket))
  {
    Rts_E = _pDisService_X->puDisClientWebSocket->Disconnect(DIS_CLIENT_WS_TIMEOUT);
  }
  return Rts_E;
}
BOFERR DisService::WriteWebSocket(DIS_DBG_SERVICE *_pDisService_X, const char *_pData_c)
{
  BOFERR Rts_E = BOF_ERR_INIT;
  uint32_t Nb_U32;

  if ((_pDisService_X) && (_pDisService_X->puDisClientWebSocket))
  {
    Rts_E = BOF_ERR_EINVAL;
    if (_pData_c)
    {
      Nb_U32 = strlen(_pData_c);
      Rts_E = _pDisService_X->puDisClientWebSocket->Write(DIS_CLIENT_WS_TIMEOUT, Nb_U32, (uint8_t *)_pData_c);
    }
  }
  return Rts_E;
}
BOFERR DisService::OnWebSocketOpenEvent(void *_pWsCbParam)
{
  BOFERR Rts_E = BOF_ERR_INTERNAL;
  DIS_DBG_SERVICE *pDisService_X = (DIS_DBG_SERVICE *)_pWsCbParam;

  if (pDisService_X)
  {
    pDisService_X->IsWebSocketConnected_B = true;
    Rts_E = BOF_ERR_NO_ERROR;
  }
  return Rts_E;
}
BOFERR DisService::OnWebSocketErrorEvent(void *_pWsCbParam, BOFERR _Sts_E)
{
  BOFERR Rts_E = BOF_ERR_INTERNAL;
  DIS_DBG_SERVICE *pDisService_X = (DIS_DBG_SERVICE *)_pWsCbParam;

  if (pDisService_X)
  {
    Rts_E = CloseWebSocket(pDisService_X);
    pDisService_X->Ws = reinterpret_cast<uintptr_t>(pDisService_X);
    Rts_E = BOF_ERR_NO_ERROR;
  }
  return Rts_E;
}
BOFERR DisService::OnWebSocketCloseEvent(void *_pWsCbParam)
{
  BOFERR Rts_E = BOF_ERR_INTERNAL;
  DIS_DBG_SERVICE *pDisService_X = (DIS_DBG_SERVICE *)_pWsCbParam;

  if (pDisService_X)
  {
    pDisService_X->IsWebSocketConnected_B = false;
    pDisService_X->Ws = -1;
    DIS_CLIENT_END_OF_COMMAND(pDisService_X);
    Rts_E = BOF_ERR_NO_ERROR;
  }
  return Rts_E;
}
BOFERR DisService::OnWebSocketMessageEvent(void *_pWsCbParam, uint32_t _Nb_U32, uint8_t *_pData_U8, BOF::BOF_BUFFER &_rReply_X)
{
  BOFERR Rts_E = BOF_ERR_INTERNAL;
  DIS_DBG_SERVICE *pDisService_X = (DIS_DBG_SERVICE *)_pWsCbParam;
  BOF::BOF_RAW_BUFFER *pRawBuffer_X;
  bool FinalFragment_B;

  if ((pDisService_X) && (_pData_U8))
  {
    pDisService_X->puRxBufferCollection->SetAppendMode(0, true, nullptr); // By default we append
    FinalFragment_B = true;
    Rts_E = pDisService_X->puRxBufferCollection->PushBuffer(0, _Nb_U32, _pData_U8, &pRawBuffer_X);
    pDisService_X->puRxBufferCollection->SetAppendMode(0, !FinalFragment_B, &pRawBuffer_X); // If it was the final one, we stop append->Result is pushed and committed
    // DisClient::S_Log("err %d data %d:%p Rts:indx %d slot %x 1: %x:%p 2: %x:%p\n", Rts_E, _Nb_U32, _pData_U8, pRawBuffer_X->IndexInBuffer_U32, pRawBuffer_X->SlotEmpySpace_U32, pRawBuffer_X->Size1_U32, pRawBuffer_X->pData1_U8, pRawBuffer_X->Size2_U32, pRawBuffer_X->pData2_U8);
    if (Rts_E != BOF_ERR_NO_ERROR)
    {
      CloseWebSocket(pDisService_X);
    }
  }
  return Rts_E;
}
#endif

/*
Windows PowerShell
Copyright (C) Microsoft Corporation. All rights reserved.

Install the latest PowerShell for new features and improvements! https://aka.ms/PSWindows

PS C:\pro\evs-gbio\evs-dis-client>  & 'C:\python310\python.exe' 'c:\Users\bha\.vscode\extensions\ms-python.python-2023.22.1\pythonFiles\lib\python\debugpy\adapter/../..\debugpy\launcher' '62186' '--' './evs-dis-client.py' '--verbose'
Running from the non-packaged version (data in C:\pro\evs-gbio\evs-dis-client\data).
Config in C:\Users\bha\.evs-dis-client.
[DBG_SERVICE_CLT] (v) 1.0.0 (d) 10/07/23 (a) B.harmel

1 command line arguments passed to './evs-dis-client.py' (cwd 'C:\pro\evs-gbio\evs-dis-client')
--verbose
Thread starts
Connection opened
================ Send seq '1' cmd 'GET /DebugServiceRoute?seq=1'. ================
DEBUG: DebugServiceRoute
{
  "version": "1.0.0",
  "protocolInfo": "GET /DebugServiceRoute?seq=1",
  "route": [
    {
      "name": "/Gbe/",
      "ip": "127.0.0.1:2510"
    },
    {
      "name": "/V6x/",
      "ip": "127.0.0.1:2511"
    },
    {
      "name": "/P6x/",
      "ip": "127.0.0.1:2512"
    }
  ]
}
================ Send seq '2' cmd 'GET /Gbe/DebugPageLayout?seq=2'. ================
DEBUG: DebugServicePageLayout
{
  "version": "1.0.0",
  "protocolInfo": "GET /Gbe/DebugPageLayout?seq=2",
  "title": "Gbe Debug Info Server",
  "page": [
    {
      "label": "Gigabit - Configuration",
      "page": 800,
      "maxChnl": 0,
      "subPage": [
        {
          "label": "Ip configuration",
          "help": "<h1>This is a heading</h1><p>This is some paragraph text.</p>"
        },
        {
          "label": "Filesystem configuration",
          "help": "untangle yourself !"
        },
        {
          "label": "Ftp server configuration",
          "help": "<h1>Keyboard Shortcuts:</h1><ul><li><strong>Page Down (\u2193)</strong>: Decrease Page</li><li><strong>Page Up (\u2191)</strong>: Increase Page</li><li><strong>Down Arrow (\u2193)</strong>: Decrease SubPage</li><li><strong>Up Arrow (\u2191)</strong>: Increase SubPage</li><li><strong>F</strong>: Pause/Resume Refresh</li><li><strong>R</strong>: Reset Component</li><li><strong>+</strong>: Increase Channel</li><li><strong>-</strong>: Decrease Channel</li><li><strong>!</strong>: Enter User Data</li></ul>"
        },
        {
          "label": "Ftp server user table"
        },
        {
          "label": "Telnet server configuration"
        }
      ]
    },
    {
      "label": "Gigabit - Comram",
      "page": 801,
      "maxChnl": 0,
      "subPage": [
        {
          "label": "PC to uC Comram info"
        },
        {
          "help": "<h1>Navigation</h1><p>F1 decrease offset.</p><p>Shift-F1 increase offset.</p>",
          "label": "PC to uC Comram Gbe Cmd"
        },
        {
          "help": "<h1>Navigation</h1><p>F1 decrease offset.</p><p>Shift-F1 increase offset.</p>",
          "label": "uC to PC Comram Gbe Cmd"
        }
      ]
    },
    {
      "label": "Gigabit - Ethernet Server",
      "page": 804,
      "maxChnl": 8,
      "subPage": [
        {
          "label": "Tge server state"
        },
        {
          "label": "Tge file system state"
        },
        {
          "label": "Tge file system lock table [8]"
        }
      ]
    },
    {
      "label": "Gigabit - Server",
      "page": 805,
      "maxChnl": 0,
      "subPage": [
        {
          "label": "Server state"
        },
        {
          "label": "Ftp server state"
        },
        {
          "label": "Cdx protocol command statistics"
        },
        {
          "label": "Cdx protocol command history"
        }
      ]
    },
    {
      "label": "Gigabit - Session",
      "page": 806,
      "maxChnl": 25,
      "subPage": [
        {
          "label": "Session state [25]"
        },
        {
          "label": "Ftp session state (1/3) [25]"
        },
        {
          "label": "Ftp session state (2/3) [25]"
        },
        {
          "label": "Ftp session state (3/3) [25]"
        },
        {
          "label": "Client state [25]"
        },
        {
          "label": "Cdx protocol command statistics [25]"
        },
        {
          "label": "Cdx protocol command history [25]"
        },
        {
          "label": "Clip info [25]"
        }
      ]
    },
    {
      "label": "Gigabit - Rpc",
      "page": 807,
      "maxChnl": 0,
      "subPage": [
        {
          "label": "Rpc state [25]"
        }
      ]
    },
    {
      "label": "Gigabit - Telnet",
      "page": 808,
      "maxChnl": 0,
      "subPage": [
        {
          "label": "Telnet server"
        },
        {
          "label": "Telnet session"
        }
      ]
    },
    {
      "label": "Gigabit - System",
      "page": 810,
      "maxChnl": 0,
      "subPage": [
        {
          "label": "System Reset"
        }
      ]
    },
    {
      "label": "Gigabit - Ftp Client Manager",
      "page": 811,
      "maxChnl": 0,
      "subPage": [
        {
          "label": "Ftp Client State"
        }
      ]
    },
    {
      "label": "Gigabit - Performance",
      "page": 812,
      "maxChnl": 0,
      "subPage": [
        {
          "label": "Read"
        },
        {
          "label": "Write"
        },
        {
          "label": "Active Session"
        },
        {
          "label": "Measurement points"
        },
        {
          "label": "Rpc Stat"
        }
      ]
    },
    {
      "label": "Gigabit - Sensor",
      "page": 813,
      "maxChnl": 0,
      "subPage": [
        {
          "label": "Temperature"
        }
      ]
    }
  ]
}
================ Send seq '3' cmd 'GET /V6x/DebugPageLayout?seq=3'. ================
DEBUG: DebugServicePageLayout
{
  "version": "1.0.0",
  "protocolInfo": "GET /V6x/DebugPageLayout?seq=3",
  "title": "V6x Debug Info Server",
  "page": [
    {
      "label": "V6x - Configuration",
      "page": 120,
      "maxChnl": 12,
      "subPage": [
        {
          "label": "Video"
        },
        {
          "label": "Audio"
        }
      ]
    },
    {
      "label": "V6x - Status",
      "page": 121,
      "maxChnl": 12,
      "subPage": [
        {
          "label": "VideoSts"
        },
        {
          "label": "AudioSts"
        }
      ]
    }
  ]
}
================ Send seq '4' cmd 'GET /P6x/DebugPageLayout?seq=4'. ================
send_command: Invalid JSON format.
Cannot 'GET /P6x/DebugPageLayout' from 'ws://10.129.171.112:8080'
select_navigation_tree_node crt_page_index 0 crt_page 800 crt_sub_page 0 col[i] (800, 0)
select_navigation_tree_node selection_set I003 page_label Gigabit - Configuration sub_page_label Ip configuration

================ Send seq '5' cmd 'GET /Gbe/DebugPageInfo/800/0/?chnl=0&flag=3&key=None&user_input=&seq=5'. ================
AbsTime 79888113 nb 1 crt 31427 uS mean 31427.0 uS cmd GET /Gbe/DebugPageInfo/800/0/?chnl=0&flag=3&key=None&user_input=
DEBUG: DebugPageInfo
{
  "version": "1.0.0",
  "protocolInfo": "GET /Gbe/DebugPageInfo/800/0/?chnl=0&flag=3&key=None&seq=5&user_input",
  "background": {
    "bg": "#808080",
    "line": [
      {
        "x": 2,
        "y": 2,
        "bg": "#808080",
        "fg": "#FF0000",
        "text": "[r] Reset TcpIp stack"
      },
      {
        "x": 2,
        "y": 4,
        "bg": "#808080",
        "fg": "#FFFFFF",
        "text": "Magic number:"
      },
      {
        "x": 2,
        "y": 5,
        "bg": "#808080",
        "fg": "#FFFFFF",
        "text": "Cdx version:"
      },
      {
        "x": 2,
        "y": 6,
        "bg": "#808080",
        "fg": "#FFFFFF",
        "text": "Cdx channel:"
      },
      {
        "x": 2,
        "y": 7,
        "bg": "#808080",
        "fg": "#FFFFFF",
        "text": "Gbe version:"
      },
      {
        "x": 2,
        "y": 8,
        "bg": "#808080",
        "fg": "#FFFFFF",
        "text": "Gbe interface:"
      },
      {
        "x": 2,
        "y": 10,
        "bg": "#808080",
        "fg": "#00FF00",
        "text": "Interface 0"
      },
      {
        "x": 2,
        "y": 11,
        "bg": "#808080",
        "fg": "#FFFFFF",
        "text": "Name:"
      },
      {
        "x": 2,
        "y": 12,
        "bg": "#808080",
        "fg": "#FFFFFF",
        "text": "Ip address:"
      },
      {
        "x": 2,
        "y": 13,
        "bg": "#808080",
        "fg": "#FFFFFF",
        "text": "Gw address:"
      },
      {
        "x": 2,
        "y": 14,
        "bg": "#808080",
        "fg": "#FFFFFF",
        "text": "Ip mask:"
      },
      {
        "x": 2,
        "y": 15,
        "bg": "#808080",
        "fg": "#FFFFFF",
        "text": "Mac address:"
      },
      {
        "x": 2,
        "y": 17,
        "bg": "#808080",
        "fg": "#00FF00",
        "text": "Interface 1"
      },
      {
        "x": 2,
        "y": 18,
        "bg": "#808080",
        "fg": "#FFFFFF",
        "text": "Name:"
      },
      {
        "x": 2,
        "y": 19,
        "bg": "#808080",
        "fg": "#FFFFFF",
        "text": "Ip address:"
      },
      {
        "x": 2,
        "y": 20,
        "bg": "#808080",
        "fg": "#FFFFFF",
        "text": "Gw address:"
      },
      {
        "x": 2,
        "y": 21,
        "bg": "#808080",
        "fg": "#FFFFFF",
        "text": "Ip mask:"
      },
      {
        "x": 2,
        "y": 22,
        "bg": "#808080",
        "fg": "#FFFFFF",
        "text": "Mac address:"
      }
    ]
  },
  "foreground": {
    "bg": "#808080",
    "line": [
      {
        "x": 21,
        "y": 4,
        "bg": "#808080",
        "fg": "#0000FF",
        "text": "0x0004000A"
      },
      {
        "x": 21,
        "y": 5,
        "bg": "#808080",
        "fg": "#0000FF",
        "text": "0x08020304"
      },
      {
        "x": 21,
        "y": 6,
        "bg": "#808080",
        "fg": "#0000FF",
        "text": "25"
      },
      {
        "x": 21,
        "y": 7,
        "bg": "#808080",
        "fg": "#0000FF",
        "text": "0x20050405"
      },
      {
        "x": 21,
        "y": 8,
        "bg": "#808080",
        "fg": "#0000FF",
        "text": "2"
      },
      {
        "x": 21,
        "y": 11,
        "bg": "#808080",
        "fg": "#0000FF",
        "text": "'eth-1gb0'"
      },
      {
        "x": 21,
        "y": 12,
        "bg": "#808080",
        "fg": "#0000FF",
        "text": "10.129.171.112"
      },
      {
        "x": 21,
        "y": 13,
        "bg": "#808080",
        "fg": "#0000FF",
        "text": "10.129.171.254"
      },
      {
        "x": 21,
        "y": 14,
        "bg": "#808080",
        "fg": "#0000FF",
        "text": "255.255.255.0"
      },
      {
        "x": 21,
        "y": 15,
        "bg": "#808080",
        "fg": "#0000FF",
        "text": "06: 00.1C.F3.0A.20.CC"
      },
      {
        "x": 21,
        "y": 18,
        "bg": "#808080",
        "fg": "#0000FF",
        "text": "'eth-1gb1'"
      },
      {
        "x": 21,
        "y": 19,
        "bg": "#808080",
        "fg": "#0000FF",
        "text": "10.129.172.112"
      },
      {
        "x": 21,
        "y": 20,
        "bg": "#808080",
        "fg": "#0000FF",
        "text": "10.129.171.254"
      },
      {
        "x": 21,
        "y": 21,
        "bg": "#808080",
        "fg": "#0000FF",
        "text": "255.255.255.0"
      },
      {
        "x": 21,
        "y": 22,
        "bg": "#808080",
        "fg": "#0000FF",
        "text": "06: 00.1C.F3.0A.20.CD"
      },
      {
        "x": 60,
        "y": 4,
        "bg": "#FF0000",
        "fg": "#0000FF",
        "text": "00000000"
      }
    ]
  }
}
*/
