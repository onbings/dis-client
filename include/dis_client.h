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

constexpr const char *APP_NAME = "DisClient";

enum class DIS_CLIENT_STATE : uint32_t
{
  DIS_CLIENT_STATE_IDLE = 0,
  DIS_CLIENT_STATE_OPEN_WS,
  DIS_CLIENT_STATE_CONNECT,
  DIS_CLIENT_STATE_GET_DEBUG_SERVICE_ROUTE,
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
  std::string Label_S;
  std::string DisServerEndpoint_S;
  uint32_t Page_U32;
  uint32_t SubPage_U32;

  uint32_t RxBufferSize_U32;
  uint32_t NbMaxBufferEntry_U32;
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
    Label_S = "";
    DisServerEndpoint_S = "";
    Page_U32 = 0;
    SubPage_U32 = 0;
    RxBufferSize_U32 = 0;
    NbMaxBufferEntry_U32 = 0;
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

struct DIS_DBG_ROUTE
{
  std::string Name_S;
  std::string IpAddress_S;
  DIS_DBG_ROUTE()
  {
    Reset();
  }
  void Reset()
  {
    Name_S = "";
    IpAddress_S = "";
  }
};
struct DIS_DBG_SUB_PAGE
{
  std::string Label_S;
  std::string Help_S;
  DIS_DBG_SUB_PAGE()
  {
    Reset();
  }
  void Reset()
  {
    Label_S = "";
    Help_S = "";
  }
};
struct DIS_DBG_PAGE
{
  std::string Label_S;
  uint32_t Page_U32;
  uint32_t MaxChannel_U32;
  std::vector<DIS_DBG_SUB_PAGE> DisDbgSubPageCollection;
  DIS_DBG_PAGE()
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
  void DisplayServiceRoute(int32_t _x_U32, int32_t _y_U32, bool &_rIsServiceRouteVisible_B, const nlohmann::json &_rServiceRouteJsonData, int32_t &_rDisDbgRouteIndex_S32);
  void DisplayPageLayout(int32_t _x_U32, int32_t _y_U32, bool &_rIsPageLayoutVisible_B, const nlohmann::json &_rPageLayoutJsonData, int32_t &_rPageIndex_S32, int32_t &_rSubPageIndex_S32);
  void DisplayPageInfo(int32_t _x_U32, int32_t _y_U32, bool &_rIsPageInfoVisible_B, bool _IsBackground_B, const nlohmann::json &_rPageInfoJsonData);

private:
  bool IsWebSocketConnected();
  bool IsCommandRunning();
  BOFERR OpenWebSocket();
  BOFERR CloseWebSocket();
  BOFERR WriteWebSocket(const char *_pData_c);
  BOFERR ReadWebSocket(uint32_t &_rMaxSize_U32, char *_pData_c);
  BOFERR SendCommand(uint32_t _TimeoutInMsForReply_U32, const std::string &_rCmd_S);
  BOFERR CheckForCommandReply(std::string &_rReply_S);
  DIS_CLIENT_STATE mDisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_MAX;
  DIS_CLIENT_STATE mDisClientLastState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_MAX;
  static ImColor S_HexaColor(const std::string &_rHexaColor_S);
  BOFERR PrintAt(uint32_t _x_U32, uint32_t _y_U32, const std::string &_rHexaForeColor_S, const std::string &_rHexaBackColor_S, const std::string &_rText_S);
#if defined(__EMSCRIPTEN__)
public:
  BOFERR
  OnWebSocketOpenEvent(const EmscriptenWebSocketOpenEvent *_pWebsocketEvent_X);
  BOFERR OnWebSocketErrorEvent(const EmscriptenWebSocketErrorEvent *_pWebsocketEvent_X);
  BOFERR OnWebSocketCloseEvent(const EmscriptenWebSocketCloseEvent *_pWebsocketEvent_X);
  BOFERR OnWebSocketMessageEvent(const EmscriptenWebSocketMessageEvent *_pWebsocketEvent_X);
#else
private:
  BOFERR OnWebSocketOpenEvent(void *_pId);
  BOFERR OnWebSocketErrorEvent(void *_pId, BOFERR _Sts_E);
  BOFERR OnWebSocketCloseEvent(void *_pId);
  BOFERR OnWebSocketMessageEvent(void *_pId, uint32_t _Nb_U32, uint8_t *_pData_U8, BOF::BOF_BUFFER &_rReply_X);
#endif

private:
  static uint32_t S_mSeqId_U32;
  uint32_t mWsConnectTimer_U32 = 0;
  uint32_t mLastCmdSentTimeoutInMs_U32 = 0;
  uint32_t mWaitForReplyId_U32 = 0;
  std::string mLastWebSocketMessage_S;
  uint32_t mLastCmdSentTimer_U32 = 0;
  bool mWsConnected_B = false;
  bool mVisible_B = true;
  ImFont *mpConsoleFont_X = nullptr;
  BOF::BOF_SIZE<uint32_t> mConsoleCharSize_X;

  uint32_t mPollTimer_U32 = 0;
  nlohmann::json mServiceRouteJsonData;
  bool mIsServiceRouteVisible_B = true;
  int32_t mServiceRouteGuiSelected_S32 = -1; // Idem to mDisDbgRouteIndex_S32

  nlohmann::json mPageLayoutJsonData;
  bool mIsPageLayoutVisible_B = true;
  int32_t mPageLayoutGuiSelected_S32 = -1; // NOT Idem to mDisDbgPageIndex_S32/mDisDbgSubPageIndex_S32

  nlohmann::json mPageInfoBackJsonData;
  nlohmann::json mPageInfoForeJsonData;
  bool mIsPageInfoVisible_B = true;
  ImVec4 mPageInfoBackColor_X;

  //"GET /Gbe/DebugPageInfo/800/0/?chnl=0&flag=3&key=0&user_input=");
  std::vector<DIS_DBG_ROUTE> mDisDbgRouteCollection;
  int32_t mDisDbgRouteIndex_S32 = -1;

  std::vector<DIS_DBG_PAGE> mDisDbgPageCollection;
  int32_t mDisDbgPageIndex_S32 = -1;
  int32_t mDisDbgSubPageIndex_S32 = -1;

  std::vector<DIS_DBG_PAGE_PARAM> mDisDbgPageParamCollection; // Same size than mDisDbgPageCollection with dynamic param (use selection)

  uint32_t mCtrlFlag_U32 = DIS_CTRL_FLAG_NONE;
  uint32_t mFctKeyFlag_U32 = DIS_KEY_FLAG_NONE;
  std::string mUserInput_S;

  DIS_CLIENT_PARAM mDisClientParam_X;
  std::unique_ptr<BOF::BofRawCircularBuffer> mpuRxBufferCollection = nullptr;

#if defined(__EMSCRIPTEN__)
  EMSCRIPTEN_WEBSOCKET_T mWs = -1;
#else
  std::unique_ptr<DisClientWebSocket> mpuDisClientWebSocket = nullptr;
  uint32_t mWs = -1;
#endif
};
