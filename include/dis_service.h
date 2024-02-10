/*
 * Copyright (c) 2023-2033, Onbings. All rights reserved.
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * This module defines a dis service
 *
 * History:
 *
 * V 1.00  Jan 25 2024  Bernard HARMEL: b.harmel@evs.com : Initial release
 */
#pragma once

#include <bofstd/bofrawcircularbuffer.h>
#include <nlohmann/json.hpp>
#include "dis_service.h"
#include "dis_discovery.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>
EM_BOOL WebSocket_OnMessage(int _Event_i, const EmscriptenWebSocketMessageEvent *_pWebsocketEvent_X, void *_pUserData);
#else
#include "dis_client_websocket.h"
#include "webrpc/websocket.h"
class DisClientWebSocket;
#endif
  enum class DIS_SERVICE_STATE : uint32_t
  {
    DIS_SERVICE_STATE_IDLE = 0,
    DIS_SERVICE_STATE_CREATE_WS,
    DIS_SERVICE_STATE_OPEN_WS,
    DIS_SERVICE_STATE_CONNECT,
    DIS_SERVICE_STATE_GET_DIS_SERVICE,
    DIS_SERVICE_STATE_GET_DEBUG_PAGE_LAYOUT,
    DIS_SERVICE_STATE_GET_DEBUG_PAGE_INFO,
    DIS_SERVICE_STATE_MAX,
  };

  enum DIS_CTRL_FLAG : uint32_t
  {
    DIS_CTRL_FLAG_NONE = 0x00000000,
    DIS_CTRL_FLAG_BACK = 0x00000001,
    DIS_CTRL_FLAG_FORE = 0x00000002,
    DIS_CTRL_FLAG_RESET = 0x80000000
  };
  enum DIS_KEY_FLAG : uint32_t
  {
    DIS_KEY_FLAG_NONE = 0x00000000,
    DIS_KEY_FLAG_SHIFT = 0x00000001,
    DIS_KEY_FLAG_CTRL = 0x00000002,
    DIS_KEY_FLAG_ALT = 0x00000004,
  };
  struct DIS_SERVICE_PARAM
  {
    std::string DisServerEndpoint_S;
    uint32_t QueryServerPollTimeInMs_U32;
    DIS_DEVICE DisDevice_X;

    DIS_SERVICE_PARAM()
    {
      Reset();
    }
    void Reset()
    {
      DisServerEndpoint_S = "";
      QueryServerPollTimeInMs_U32 = 0;
      DisDevice_X.Reset();
    }
  };
/*
      + Ip configuration
*/
  struct DIS_DBG_SUB_PAGE_LAYOUT
  {
    std::string Label_S;
    std::string Help_S;
    DIS_DBG_SUB_PAGE_LAYOUT()
    {
      Reset();
    }
    void Reset()
    {
      Label_S = "";
      Help_S = "";
    }
  };
  /*
    + Gigabit - Configuration
      |
      + Ip configuration
      + Filesystem configuration
      + Ftp server configuration
  */
  struct DIS_DBG_PAGE_LAYOUT
  {
    std::string Label_S;
    uint32_t Page_U32;
    uint32_t Channel_U32;
    uint32_t MaxChannel_U32;
    std::vector<DIS_DBG_SUB_PAGE_LAYOUT> DisDbgSubPageCollection;
    DIS_DBG_PAGE_LAYOUT()
    {
      Reset();
    }
    void Reset()
    {
      Label_S = "";
      Page_U32 = 0;
      Channel_U32 = 0;
      MaxChannel_U32 = 0;
      DisDbgSubPageCollection.clear();
    }
  };
  struct DIS_SERVICE_PAGE_INFO_LINE
  {
    uint32_t x_U32;
    uint32_t y_U32;
    std::string ForeColor_S;
    std::string BackColor_S;
    std::string Text_S;

    DIS_SERVICE_PAGE_INFO_LINE()
    {
      Reset();
    }
    void Reset()
    {
      x_U32 = 0;
      y_U32 = 0;
      ForeColor_S = "";
      BackColor_S = "";
      Text_S = "";
    }
  };
  struct DIS_SERVICE_PAGE_INFO
  {
    std::string BackColor_S;
    std::vector<DIS_SERVICE_PAGE_INFO_LINE> BackPageInfoLineCollection;
    std::vector<DIS_SERVICE_PAGE_INFO_LINE> ForePageInfoLineCollection;

    DIS_SERVICE_PAGE_INFO()
    {
      Reset();
    }
    void Reset()
    {
      BackColor_S = "";
    }
  };
/*
+ Gbe
  |
  + Gigabit - Configuration
  | |
  | + Ip configuration
  | + Filesystem configuration
  | + Ftp server configuration
  |
  + Gigabit - Comram
    |
    + PC to uC Comram info
    + PC to uC Comram Gbe Cmd
*/
  struct DIS_DBG_SERVICE_ITEM
  {
    std::string Name_S;
    std::string IpAddress_S;
    std::vector<DIS_DBG_PAGE_LAYOUT> DisDbgPageLayoutCollection;

    DIS_DBG_SERVICE_ITEM()
    {
      Reset();
    }
    void Reset()
    {
      Name_S = "";
      IpAddress_S = "";
      DisDbgPageLayoutCollection.clear();
    }
  };

  class DisService;
  struct DIS_DBG_SERVICE;
  struct WS_CALLBBACK_PARAM
  {
    DisService *pDisService;
    DIS_DBG_SERVICE *pDisService_X;

    WS_CALLBBACK_PARAM()
    {
      Reset();
    }
    void Reset()
    {
      pDisService = nullptr;
      pDisService_X = nullptr;
    }
  };
  /*
   |
    + Gbe
    | |
    | + Gigabit - Configuration
    | | |
    | | + Ip configuration
    | | + Filesystem configuration
    | | + Ftp server configuration
    | |
    | + Gigabit - Comram
    |   |
    |   + PC to uC Comram info
    |   + PC to uC Comram Gbe Cmd
    + V6X
    | |
    | + Video
    | | |
    | | + Codec
    | | + Perf
    | | + Error
    | |
    | + Audio
    |   |
    |   + Channel
    |   + Bit rate
*/
  struct DIS_DBG_SERVICE
  {
    std::string Name_S;
    std::string IpAddress_S;   //ws://10.129.171.112:8080
    DIS_SERVICE_STATE DisClientState_E;
    DIS_SERVICE_STATE DisClientLastState_E;

#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_WEBSOCKET_T Ws;
#else
    std::unique_ptr<DisClientWebSocket> puDisClientWebSocket;
    uint32_t Ws;
#endif
    uint32_t StateTimer_U32;
    uint32_t LastCmdSentTimeoutInMs_U32;
    uint32_t WaitForReplyId_U32;
    std::string LastWebSocketMessage_S;
    uint32_t LastCmdSentTimer_U32;
    std::unique_ptr<BOF::BofRawCircularBuffer> puRxBufferCollection;
    bool IsWebSocketConnected_B;
    WS_CALLBBACK_PARAM WsCbParam_X;
    uint32_t PageInfoTimer_U32;
    uint32_t DisDbgServicePageLayoutIndex_U32;
    std::vector<DIS_DBG_SERVICE_ITEM> DisDbgServiceItemCollection;
    DIS_SERVICE_PAGE_INFO PageInfo_X;

    DIS_DBG_SERVICE()
    {
      Reset();
    }
    void Reset()
    {
      Name_S = "";
      IpAddress_S = "";
      DisClientState_E = DIS_SERVICE_STATE::DIS_SERVICE_STATE_MAX;
      DisClientLastState_E = DIS_SERVICE_STATE::DIS_SERVICE_STATE_MAX;

#if defined(__EMSCRIPTEN__)
      Ws = -1;
#else
      puDisClientWebSocket = nullptr;
      Ws = -1;
#endif
      StateTimer_U32 = 0;
      LastCmdSentTimeoutInMs_U32 = 0;
      WaitForReplyId_U32 = 0;
      LastWebSocketMessage_S = "";
      LastCmdSentTimer_U32 = 0;
      IsWebSocketConnected_B = false;
      puRxBufferCollection = nullptr;
      WsCbParam_X.Reset();
      PageInfoTimer_U32 = 0;
      DisDbgServicePageLayoutIndex_U32 = 0;
      DisDbgServiceItemCollection.clear();
    }
  };
  struct DIS_DBG_STATE_MACHINE
  {
    std::string DisService_S;
    uint32_t Page_U32;
    uint32_t SubPage_U32;
    uint32_t Channel_U32;
    uint32_t CtrlFlag_U32;
    uint32_t FctKeyFlag_U32;
    std::string UserInput_S;
    DIS_DBG_STATE_MACHINE()
    {
      Reset();
    }
    void Reset()
    {
      DisService_S = "";
      Page_U32 = 0;
      SubPage_U32 = 0;
      Channel_U32 = 0;
      CtrlFlag_U32 = 0;
      FctKeyFlag_U32 = 0;
      UserInput_S = "";
    }
  };
  class DisService
  {
  public:
    DisService(const DIS_SERVICE_PARAM &_rDisServiceParam_X);
    ~DisService();
    DIS_SERVICE_STATE StateMachine(const DIS_DBG_STATE_MACHINE &_rStateMachine_X);
    const DIS_DBG_SERVICE &GetDbgDisService();
    BOFERR Run();
    BOFERR Stop();
  private:
    BOFERR DecodeDisService(const nlohmann::json &_rDisServiceJsonData);
    BOFERR DecodePageLayout(const nlohmann::json &_rPageLayoutJsonData, DIS_DBG_SERVICE_ITEM  &_rDisDbgServiceItem);
    BOFERR DecodePageInfo(const nlohmann::json &_rPageInfoJsonData);

    bool IsCommandRunning(DIS_DBG_SERVICE *_pDisService_X);
    BOFERR OpenWebSocket(DIS_DBG_SERVICE *_pDisService_X);
    BOFERR CloseWebSocket(DIS_DBG_SERVICE *_pDisService_X);
    BOFERR WriteWebSocket(DIS_DBG_SERVICE *_pDisService_X, const char *_pData_c);
    BOFERR ReadWebSocket(DIS_DBG_SERVICE *_pDisService_X, uint32_t &_rMaxSize_U32, char *_pData_c);
    BOFERR SendCommand(DIS_DBG_SERVICE *_pDisService_X, uint32_t _TimeoutInMsForReply_U32, const std::string &_rCmd_S);
    BOFERR CheckForCommandReply(DIS_DBG_SERVICE *_pDisService_X, std::string &_rReply_S);
#if defined(__EMSCRIPTEN__)
  public:
    BOFERR OnWebSocketOpenEvent(void *_pWsCbParam, const EmscriptenWebSocketOpenEvent *_pWebsocketEvent_X);
    BOFERR OnWebSocketErrorEvent(void *_pWsCbParam, const EmscriptenWebSocketErrorEvent *_pWebsocketEvent_X);
    BOFERR OnWebSocketCloseEvent(void *_pWsCbParam, const EmscriptenWebSocketCloseEvent *_pWebsocketEvent_X);
    BOFERR OnWebSocketMessageEvent(void *_pWsCbParam, const EmscriptenWebSocketMessageEvent *_pWebsocketEvent_X);
#else
  private:
    BOFERR OnWebSocketOpenEvent(void *_pWsCbParam);
    BOFERR OnWebSocketErrorEvent(void *_pWsCbParam, BOFERR _Sts_E);
    BOFERR OnWebSocketCloseEvent(void *_pWsCbParam);
    BOFERR OnWebSocketMessageEvent(void *_pWsCbParam, uint32_t _Nb_U32, uint8_t *_pData_U8, BOF::BOF_BUFFER &_rReply_X);
#endif

  private:
    DIS_SERVICE_PARAM mDisServiceParam_X;
    static uint32_t S_mSeqId_U32;
    DIS_DBG_SERVICE mDisService_X;
  };
