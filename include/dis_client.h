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
  uint32_t RxBufferSize_U32;
  uint32_t NbMaxBufferEntry_U32;

  BOF::BOF_IMGUI_PARAM ImguiParam_X;

  DIS_CLIENT_PARAM()
  {
    Reset();
  }
  void Reset()
  {
    RxBufferSize_U32 = 0;
    NbMaxBufferEntry_U32 = 0;
    ImguiParam_X.Reset();
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
  BOFERR LoadNavigation(std::string &_rNavJson_S, nlohmann::json &_rJsonData);
  void DrawNavigationTree(int32_t _x_U32, int32_t _y_U32, bool &_rIsNavigationVisible_B, const nlohmann::json &_rNavigationJsonData);
  BOFERR LoadPageInfo(std::string &_rPgInfoJson_S, nlohmann::json &_rJsonData);
  void DisplayPageInfo(int32_t _x_U32, int32_t _y_U32, bool &_rIsPgInfoVisible_B, const nlohmann::json &_rPgInfoJsonData);

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
#if defined(__EMSCRIPTEN__)
public:
  BOFERR
  OnWebSocketOpenEvent(const EmscriptenWebSocketOpenEvent *_pWebsocketEvent_X);
  BOFERR OnWebSocketErrorEvent(const EmscriptenWebSocketErrorEvent *_pWebsocketEvent_X);
  BOFERR OnWebSocketCloseEvent(const EmscriptenWebSocketCloseEvent *_pWebsocketEvent_X);
  BOFERR OnWebSocketMessageEvent(const EmscriptenWebSocketMessageEvent *_pWebsocketEvent_X);
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
  uint32_t mNbFontLoaded_U32 = 0;
  uint32_t mConsoleFontIndex_U32 = 0;
  ImFont *mpConsoleFont_X = nullptr;
  nlohmann::json mNavigationJsonData;
  bool mIsNavigationVisible_B = true;
  uint32_t mNavigationLeafIdSelected_U32 = -1;
  nlohmann::json mPageInfoJsonData;
  bool mIsPageInfoVisible_B = true;

  DIS_CLIENT_PARAM mDisClientParam_X;
  std::unique_ptr<BOF::BofRawCircularBuffer> mpuRxBufferCollection = nullptr;

#if defined(__EMSCRIPTEN__)
  EMSCRIPTEN_WEBSOCKET_T mWs = -1;
#endif
};
