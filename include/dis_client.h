/*
 * Copyright (c) 2023-2033, Onbings. All rights reserved.
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * This module defines a dis client using emscripten and imgui
 *
 * History:
 *
 * V 1.00  Dec 24 2023  Bernard HARMEL: onbings@gmail.com : Initial release
 */
#pragma once

#include "bof_imgui.h"
#include <bofstd/bofrawcircularbuffer.h>
#include <nlohmann/json.hpp>

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>
EM_BOOL WebSocket_OnMessage(int _Event_i, const EmscriptenWebSocketMessageEvent *_pWebsocketEvent_X, void *_pUserData);
#else
#include "dis_client_websocket.h"
#include "webrpc/websocket.h"
class DisClientWebSocket;
#endif

constexpr int32_t DIS_CLIENT_INVALID_INDEX = -1;

enum class DIS_CLIENT_STATE : uint32_t
{
  DIS_CLIENT_STATE_IDLE = 0,
  DIS_CLIENT_STATE_CREATE_WS,
  DIS_CLIENT_STATE_OPEN_WS,
  DIS_CLIENT_STATE_CONNECT,
  DIS_CLIENT_STATE_GET_DIS_SERVICE,
  DIS_CLIENT_STATE_GET_DEBUG_PAGE_LAYOUT,
  DIS_CLIENT_STATE_GET_DEBUG_PAGE_INFO,
  DIS_CLIENT_STATE_MAX,
};
struct DIS_CLIENT_PARAM
{
  uint32_t PollTimeInMs_U32;
  uint32_t FontSize_U32;
  uint32_t ConsoleWidth_U32;
  uint32_t ConsoleHeight_U32;
//  std::string TargetName_S;
  std::string DisServerEndpoint_S;

  BOF::BOF_IMGUI_PARAM ImguiParam_X;

  DIS_CLIENT_PARAM()
  {
    Reset();
  }
  void Reset()
  {
    PollTimeInMs_U32 = 0;
    FontSize_U32 = 0;
    ConsoleWidth_U32 = 0;
    ConsoleHeight_U32 = 0;
//    TargetName_S = "";
    DisServerEndpoint_S = "";
    ImguiParam_X.Reset();
  }
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
struct DIS_DBG_PAGE_LAYOUT
{
  std::string Label_S;
  uint32_t Page_U32;
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
    MaxChannel_U32 = 0;
    DisDbgSubPageCollection.clear();
  }
};
struct DIS_DBG_PAGE_PARAM
{
  uint32_t Channel_U32 = 0;
  uint32_t SubPageIndex_U32 = 0;
  DIS_DBG_PAGE_PARAM()
  {
    Reset();
  }
  void Reset()
  {
    Channel_U32 = 0;
    SubPageIndex_U32 = 0;
  }
};
class DisClient;
struct DIS_DBG_SERVICE;
struct WS_CALLBBACK_PARAM
{
  DisClient *pDisClient;
  DIS_DBG_SERVICE *pDisService_X;

  WS_CALLBBACK_PARAM()
  {
    Reset();
  }
  void Reset()
  {
    pDisClient = nullptr;
    pDisService_X = nullptr;
  }
};
struct DIS_DBG_SERVICE
{
  std::string Name_S;
  std::string IpAddress_S;   //ws://10.129.171.112:8080
//WebSocket
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
  bool IsWebSocketConnected_B;
  std::unique_ptr<BOF::BofRawCircularBuffer> puRxBufferCollection;
  WS_CALLBBACK_PARAM WsCbParam_X;

//State machine
  DIS_CLIENT_STATE DisClientState_E;
  DIS_CLIENT_STATE DisClientLastState_E;

  //Page Layout
  nlohmann::json PageLayoutJsonData;
  bool IsPageLayoutVisible_B;

  std::vector<DIS_DBG_PAGE_LAYOUT> PageLayoutCollection;
  int32_t PageLayoutIndex_S32;
  int32_t SubPageLayoutIndex_S32;

  //Page Info
  uint32_t PageInfoTimer_U32;
  nlohmann::json PageInfoBackJsonData;
  nlohmann::json PageInfoForeJsonData;
  bool IsPageInfoVisible_B;
  ImVec4 PageInfoBackColor_X;

  std::vector<DIS_DBG_PAGE_PARAM> PageParamCollection; // Same size than DisDbgPageCollection with dynamic param (user selection)

  uint32_t CtrlFlag_U32;
  uint32_t FctKeyFlag_U32;
  std::string UserInput_S;

  DIS_DBG_SERVICE()
  {
    Reset();
  }
  void Reset()
  {
#if defined(__EMSCRIPTEN__)
    Ws = -1;
#else
    puDisClientWebSocket = nullptr;
    Ws = -1;
#endif
    StateTimer_U32 = 0;
    LastCmdSentTimeoutInMs_U32 = 0;
    WaitForReplyId_U32 = 0;
    LastWebSocketMessage_S="";
    LastCmdSentTimer_U32 = 0;
    IsWebSocketConnected_B = false;
    puRxBufferCollection = nullptr;
    WsCbParam_X.Reset();

    DisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_MAX;
    DisClientLastState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_MAX;

    Name_S = "";
    IpAddress_S = "";

    PageLayoutJsonData.clear();
    IsPageLayoutVisible_B = false;

    PageLayoutCollection.clear();
    PageLayoutIndex_S32 = DIS_CLIENT_INVALID_INDEX;
    SubPageLayoutIndex_S32 = DIS_CLIENT_INVALID_INDEX;

    PageInfoTimer_U32 = 0;
    PageInfoBackJsonData.clear();
    PageInfoForeJsonData.clear();
    IsPageInfoVisible_B = false;
    PageInfoBackColor_X = ImVec4();
    PageParamCollection.clear();

    CtrlFlag_U32 = DIS_CTRL_FLAG_NONE;
    FctKeyFlag_U32 = DIS_KEY_FLAG_NONE;
    UserInput_S="";
  }
};

class DisClient : public BOF::Bof_ImGui
{
public:
  DisClient(const DIS_CLIENT_PARAM &_rDisClientParam_X);
  ~DisClient();
  static void S_Log(const char *_pFormat_c, ...);

private:
  BOFERR V_SaveSettings() override;
  BOFERR V_ReadSettings() override;
  void V_LoadAdditionalFonts() override;
  void V_PreNewFrame() override;
  BOFERR V_RefreshGui() override;

  nlohmann::json ToJson() const;
  void FromJson(const nlohmann::json &_rJson);
  void DisplayDisService(int32_t _x_U32, int32_t _y_U32, bool &_rIsDisServiceVisible_B, const nlohmann::json &_rDisServiceJsonData, int32_t &_rDisServiceIndex_S32);
  void DisplayPageLayout(int32_t _x_U32, int32_t _y_U32, DIS_DBG_SERVICE &_rDisService_X, int32_t &_rPageIndex_S32, int32_t &_rSubPageIndex_S32);
  void UpdateConsoleMenubar(DIS_DBG_SERVICE &_rDisService_X);
  void DisplayPageInfo(int32_t _x_U32, int32_t _y_U32, DIS_DBG_SERVICE &_rDisService_X);


private:
  static ImColor S_HexaColor(const std::string &_rHexaColor_S);
  BOFERR PrintAt(uint32_t _x_U32, uint32_t _y_U32, const std::string &_rHexaForeColor_S, const std::string &_rHexaBackColor_S, const std::string &_rText_S);

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
  static uint32_t S_mSeqId_U32;
  uint32_t mIdleTimer_U32 = 0;
  ImFont *mpConsoleFont_X = nullptr;
  BOF::BOF_SIZE<uint32_t> mConsoleCharSize_X;

//Dis Service
  nlohmann::json mDisServiceJsonData;
  bool mIsDisServiceVisible_B = true;
  std::vector<DIS_DBG_SERVICE> mDisServiceCollection;
  int32_t mDisServiceIndex_S32 = -1;

  DIS_CLIENT_PARAM mDisClientParam_X;
};
