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
#include <nlohmann/json.hpp>
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>
EM_BOOL WebSocket_OnMessage(int _Event_i, const EmscriptenWebSocketMessageEvent *_pWebsocketEvent_X, void *_pUserData);
#endif

constexpr const char *APP_NAME = "DisClient";

class DisClient : public BOF::Bof_ImGui
{
public:
  DisClient(const BOF::BOF_IMGUI_PARAM &_rImguiParam_X);
  ~DisClient();
  static void S_Log(const char *_pFormat_c, ...);

private:
  nlohmann::json ToJson() const;
  void FromJson(const nlohmann::json &_rJson);
  BOFERR V_SaveSettings() override;
  BOFERR V_ReadSettings() override;
  void V_LoadAdditionalFonts() override;
  BOFERR LoadNavigation(std::string &_rNavJson_S, nlohmann::json &_rJsonData);
  void DrawNavigationTree(int32_t _x_U32, int32_t _y_U32, bool &_rIsNavigationVisible_B, const nlohmann::json &_rNavigationJsonData);
  BOFERR LoadPageInfo(std::string &_rPgInfoJson_S, nlohmann::json &_rJsonData);
  void DisplayPageInfo(int32_t _x_U32, int32_t _y_U32, bool &_rIsPgInfoVisible_B, const nlohmann::json &_rPgInfoJsonData);
  BOFERR V_RefreshGui() override;

#if defined(__EMSCRIPTEN__)
private:
  BOFERR OpenWebSocket();
  BOFERR CloseWebSocket();
  BOFERR WriteWebSocket(const char *_pData_c);

public:
  BOFERR OnWebSocketOpenEvent(const EmscriptenWebSocketOpenEvent *_pWebsocketEvent_X);
  BOFERR OnWebSocketErrorEvent(const EmscriptenWebSocketErrorEvent *_pWebsocketEvent_X);
  BOFERR OnWebSocketCloseEvent(const EmscriptenWebSocketCloseEvent *_pWebsocketEvent_X);
  BOFERR OnWebSocketMessageEvent(const EmscriptenWebSocketMessageEvent *_pWebsocketEvent_X);
#endif

private:
  bool mVisible_B = true;
  uint32_t mNbFontLoaded_U32 = 0;
  uint32_t mConsoleFontIndex_U32 = 0;
  ImFont *mpConsoleFont_X = nullptr;
  nlohmann::json mNavigationJsonData;
  bool mIsNavigationVisible_B = true;
  uint32_t mNavigationLeafIdSelected_U32 = -1;
  nlohmann::json mPageInfoJsonData;
  bool mIsPageInfoVisible_B = true;

#if defined(__EMSCRIPTEN__)
  EMSCRIPTEN_WEBSOCKET_T mWs = -1;
  std::string mLastWebSocketMessage_S;
#endif
};
