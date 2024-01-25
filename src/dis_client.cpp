/*
 * Copyright (c) 2023-2033, Onbings. All rights reserved.
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * This module implements a dis client using emscripten and imgui
 *
 * History:
 *
 * V 1.00  Dec 24 2023  Bernard HARMEL: onbings@gmail.com : Initial release
 *
 * See data flow in comment at the end of this file
 *
 */
#include "dis_client.h"
#include <bofstd/boffs.h>

#include <bofstd/bofenum.h>
#include <bofstd/bofhttprequest.h>
#include <regex>

#if defined(_WIN32)
#include <Windows.h> //OutputDebugString
#endif

uint32_t DisClient::S_mSeqId_U32 = 1;

#define IMGUI_WINDOW_TITLEBAR_HEIGHT 20
#define DIS_CLIENT_MAIN_LOOP_IDLE_TIME 100                          // 100ms We no state machine action during this time (keep imgui main loop cpu friendly)
#define DIS_CLIENT_WS_TIMEOUT (DIS_CLIENT_MAIN_LOOP_IDLE_TIME * 10) // 1000ms Timeout fo ws connect, read or write op
#define DIS_CLIENT_STATE_TIMEOUT (DIS_CLIENT_WS_TIMEOUT * 3)        // 3000ms Maximum time in a given state before returning to DIS_CLIENT_STATE_OPEN_WS
static_assert(DIS_CLIENT_WS_TIMEOUT >= (DIS_CLIENT_MAIN_LOOP_IDLE_TIME * 10), "DIS_CLIENT_WS_TIMEOUT must be >= (DIS_CLIENT_MAIN_LOOP_IDLE_TIME * 10)");
static_assert(DIS_CLIENT_STATE_TIMEOUT >= (DIS_CLIENT_WS_TIMEOUT * 3), "DIS_CLIENT_STATE_TIMEOUT must be >= (DIS_CLIENT_WS_TIMEOUT * 3 )");
#define DIS_CLIENT_RX_BUFFER_SIZE 0x100000
#define DIS_CLIENT_NB_MAX_BUFFER_ENTRY 128

#define IS_DIS_SERVICE_VALID() ((mDisServiceIndex_S32 >= 0) && (mDisServiceIndex_S32 < mDisServiceCollection.size()))
#define IS_PAGE_SUBPAGE_LAYOUT_VALID(pDisService) (((pDisService)->PageLayoutIndex_S32 >= 0) && ((pDisService)->PageLayoutIndex_S32 < (pDisService)->PageLayoutCollection.size()) && ((pDisService)->SubPageLayoutIndex_S32 >= 0) && ((pDisService)->SubPageLayoutIndex_S32 < (pDisService)->PageLayoutCollection[(pDisService)->SubPageLayoutIndex_S32].DisDbgSubPageCollection.size()))

static BOF::BofEnum<DIS_CLIENT_STATE> S_DisClientStateTypeEnumConverter(
    {
        {DIS_CLIENT_STATE::DIS_CLIENT_STATE_IDLE, "Idle"},
        {DIS_CLIENT_STATE::DIS_CLIENT_STATE_CREATE_WS, "CreateWs"},
        {DIS_CLIENT_STATE::DIS_CLIENT_STATE_OPEN_WS, "OpenWs"},
        {DIS_CLIENT_STATE::DIS_CLIENT_STATE_CONNECT, "Connect"},
        {DIS_CLIENT_STATE::DIS_CLIENT_STATE_GET_DIS_SERVICE, "GetDisService"},
        {DIS_CLIENT_STATE::DIS_CLIENT_STATE_GET_DEBUG_PAGE_LAYOUT, "GetDbgPageLayout"},
        {DIS_CLIENT_STATE::DIS_CLIENT_STATE_GET_DEBUG_PAGE_INFO, "GetDbgPageInfo"},
        {DIS_CLIENT_STATE::DIS_CLIENT_STATE_MAX, "Max"},
    },
    DIS_CLIENT_STATE::DIS_CLIENT_STATE_MAX);

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

  if ((pWsCallbackParam_X) && (pWsCallbackParam_X->pDisClient))
  {
    Rts_B = (pWsCallbackParam_X->pDisClient->OnWebSocketOpenEvent(pWsCallbackParam_X->pDisService_X, _pWebsocketEvent_X) == BOF_ERR_NO_ERROR) ? EM_TRUE : EM_FALSE;
  }
  return Rts_B;
}

EM_BOOL WebSocket_OnError(int _Event_i, const EmscriptenWebSocketErrorEvent *_pWebsocketEvent_X, void *_pUserData)
{
  EM_BOOL Rts_B = EM_FALSE;
  WS_CALLBBACK_PARAM *pWsCallbackParam_X = (WS_CALLBBACK_PARAM *)_pUserData;

  if ((pWsCallbackParam_X) && (pWsCallbackParam_X->pDisClient))
  {
    Rts_B = (pWsCallbackParam_X->pDisClient->OnWebSocketErrorEvent(pWsCallbackParam_X->pDisService_X, _pWebsocketEvent_X) == BOF_ERR_NO_ERROR) ? EM_TRUE : EM_FALSE;
  }
  return Rts_B;
}

EM_BOOL WebSocket_OnClose(int _Event_i, const EmscriptenWebSocketCloseEvent *_pWebsocketEvent_X, void *_pUserData)
{
  EM_BOOL Rts_B = EM_FALSE;
  WS_CALLBBACK_PARAM *pWsCallbackParam_X = (WS_CALLBBACK_PARAM *)_pUserData;
  if ((pWsCallbackParam_X) && (pWsCallbackParam_X->pDisClient))
  {
    Rts_B = (pWsCallbackParam_X->pDisClient->OnWebSocketCloseEvent(pWsCallbackParam_X->pDisService_X, _pWebsocketEvent_X) == BOF_ERR_NO_ERROR) ? EM_TRUE : EM_FALSE;
  }
  return Rts_B;
}
EM_BOOL WebSocket_OnMessage(int _Event_i, const EmscriptenWebSocketMessageEvent *_pWebsocketEvent_X, void *_pUserData)
{
  EM_BOOL Rts_B = EM_FALSE;
  WS_CALLBBACK_PARAM *pWsCallbackParam_X = (WS_CALLBBACK_PARAM *)_pUserData;
  if ((pWsCallbackParam_X) && (pWsCallbackParam_X->pDisClient))
  {
    Rts_B = (pWsCallbackParam_X->pDisClient->OnWebSocketMessageEvent(pWsCallbackParam_X->pDisService_X, _pWebsocketEvent_X) == BOF_ERR_NO_ERROR) ? EM_TRUE : EM_FALSE;
  }
  return Rts_B;
}

#endif

DisClient::DisClient(const DIS_CLIENT_PARAM &_rDisClientParam_X)
    : BOF::Bof_ImGui(_rDisClientParam_X.ImguiParam_X)
{
  mDisClientParam_X = _rDisClientParam_X;
  // mDisServiceCollection entry 0 is reserved to create a ws and get DIS_DBG_SERVICE list
  mDisServiceCollection.emplace_back(); // because we must do: rDisDbgService_X.WsCbParam_X.pDisService_X = &rDisDbgService_X
  DIS_DBG_SERVICE &rDisDbgService_X = mDisServiceCollection.back();
  rDisDbgService_X.Reset();
  rDisDbgService_X.Name_S = "Dis_Client_Master_Ws";
  rDisDbgService_X.IpAddress_S = mDisClientParam_X.DisServerEndpoint_S; // ws://10.129.171.112:8080
  rDisDbgService_X.StateTimer_U32 = BOF::Bof_GetMsTickCount();
  rDisDbgService_X.DisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_IDLE;
  rDisDbgService_X.WsCbParam_X.pDisClient = this;
  rDisDbgService_X.WsCbParam_X.pDisService_X = &rDisDbgService_X;
}

DisClient::~DisClient()
{
  for (DIS_DBG_SERVICE &rDisService_X : mDisServiceCollection)
  {
    CloseWebSocket(&rDisService_X);
  }
}
void DisClient::S_Log(const char *_pFormat_c, ...)
{
  char pDbg_c[0x4000];
  va_list Arg;
  va_start(Arg, _pFormat_c);
  int Sts_i = vsnprintf(pDbg_c, sizeof(pDbg_c), _pFormat_c, Arg);
  va_end(Arg);
  if ((Sts_i >= 0) && (Sts_i < sizeof(pDbg_c)))
  {
#if defined(_WIN32)
    OutputDebugString(pDbg_c);
#else
    printf("%s", pDbg_c);
#endif
  }
}

// Serialization
nlohmann::json DisClient::ToJson() const
{
  nlohmann::json Rts;
  try
  {
    Rts["PollTimeInMs"] = mDisClientParam_X.PollTimeInMs_U32;
    Rts["FontSize"] = mDisClientParam_X.FontSize_U32;
    Rts["ConsoleWidth"] = mDisClientParam_X.ConsoleWidth_U32;
    Rts["ConsoleHeight"] = mDisClientParam_X.ConsoleHeight_U32;
    // Rts["TargetName"] = mDisClientParam_X.TargetName_S;

    Rts["DisServerEndpoint"] = mDisClientParam_X.DisServerEndpoint_S;
    // Rts["ImguiParam.AppName"] = mDisClientParam_X.ImguiParam_X.AppName_S;
    Rts["ImguiParam.Size.Width"] = mDisClientParam_X.ImguiParam_X.Size_X.Width;
    Rts["ImguiParam.Size.Height"] = mDisClientParam_X.ImguiParam_X.Size_X.Height;
    // Rts["ImguiParam.TheLogger"] = mDisClientParam_X.ImguiParam_X.TheLogger;
    Rts["ImguiParam.ShowDemoWindow"] = mDisClientParam_X.ImguiParam_X.ShowDemoWindow_B;
#if 0
    Rts["DisServiceIndex"] = mDisServiceIndex_S32;
    Rts["PageLayoutIndex"] = mPageLayoutIndex_S32;
    Rts["SubPageLayoutIndex"] = mSubPageLayoutIndex_S32;
#endif
  }
  catch (nlohmann::json::exception &re)
  {
    Rts.clear();
    DisClient::S_Log("ToJson: exception caught '%s'\n", re.what());
  }
  return Rts;
}

// Deserialization
void DisClient::FromJson(const nlohmann::json &_rJson)
{
  try
  {
    mDisClientParam_X.PollTimeInMs_U32 = _rJson.value("PollTimeInMs", 1000);
    mDisClientParam_X.FontSize_U32 = _rJson.value("FontSize", 14);
    mDisClientParam_X.ConsoleWidth_U32 = _rJson.value("ConsoleWidth", 80);
    mDisClientParam_X.ConsoleHeight_U32 = _rJson.value("ConsoleHeight", 25);
    // mDisClientParam_X.TargetName_S = _rJson.value("TargetName", "???");
    mDisClientParam_X.DisServerEndpoint_S = _rJson.value("DisServerEndpoint", "ws ://0.0.0.0:8080");

    //    mDisClientParam_X.ImguiParam_X.AppName_S = _rJson.value("ImguiParam.AppName","???");
    mDisClientParam_X.ImguiParam_X.Size_X.Width = _rJson.value("ImguiParam.Size.Width", 800);
    mDisClientParam_X.ImguiParam_X.Size_X.Height = _rJson.value("ImguiParam.Size.Height", 600);
    // mDisClientParam_X.ImguiParam_X.TheLogger = _rJson.value("ImguiParam.TheLogger"].get<uint32_t>();
    mDisClientParam_X.ImguiParam_X.ShowDemoWindow_B = _rJson.value("ImguiParam.ShowDemoWindow", false);
#if 0
    mDisServiceIndex_S32 = _rJson.value("DisServiceIndex", DIS_CLIENT_INVALID_INDEX);
    mPageLayoutIndex_S32 = _rJson.value("PageLayoutIndex", DIS_CLIENT_INVALID_INDEX);
    mSubPageLayoutIndex_S32 = _rJson.value("SubPageLayoutIndex", DIS_CLIENT_INVALID_INDEX);
#endif
  }
  catch (nlohmann::json::exception &re)
  {
    DisClient::S_Log("FromJson: exception caught '%s'\n", re.what());
  }
}

BOFERR DisClient::V_SaveSettings()
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;

  auto Json = ToJson();
  std::string StateSerialized_S = Json.dump();
  // #if defined(__EMSCRIPTEN__)
  // #else
  HelloImGui::SaveUserPref(mDisClientParam_X.ImguiParam_X.WindowTitle_S, StateSerialized_S);
  // #endif
  return Rts_E;
}

BOFERR DisClient::V_ReadSettings()
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;
  // #if defined(__EMSCRIPTEN__)
  // #else
  std::string StateSerialized_S = HelloImGui::LoadUserPref(mDisClientParam_X.ImguiParam_X.WindowTitle_S);
  if (!StateSerialized_S.empty())
  {
    try
    {
      auto Json = nlohmann::json::parse(StateSerialized_S, nullptr, false);
      if (!Json.is_null())
      {
        FromJson(Json);
      }
      else
      {
        DisClient::S_Log("Failed to load %s state from user pref\n", mDisClientParam_X.ImguiParam_X.WindowTitle_S.c_str());
      }
    }
    catch (const std::exception &e)
    {
      DisClient::S_Log("V_ReadSettings: %s\n", e.what());
    }
  }
  // #endif
  return Rts_E;
}
void DisClient::V_LoadAdditionalFonts()
{
  std::string FontFilename_S = "./assets/fonts/cour.ttf";
  // if (HelloImGui::AssetExists(FontFilename_S))
  {
    mpConsoleFont_X = LoadFont(FontFilename_S.c_str(), 16);
    if (mpConsoleFont_X)
    {
    }
  }
  DisClient::S_Log("Try to load font %s ->ptr %p\n", FontFilename_S.c_str(), mpConsoleFont_X);
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

void DisClient::DisplayDisService(int32_t _x_U32, int32_t _y_U32, bool &_rIsDisServiceVisible_B,
                                  const nlohmann::json &_rDisServiceJsonData, int32_t &_rDisServiceIndex_S32)
{
  ImGuiTreeNodeFlags NodeFlag = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanAllColumns;
  ImGuiTreeNodeFlags LeafFlag = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanAllColumns;
  ImGuiTreeNodeFlags ItemFlag;
  uint32_t i_U32;
  int32_t DisServiceIndexClicked_S32;
  bool IsNodeOpen_B;
  ;

  mDisServiceCollection.resize(1); // mDisServiceCollection entry 0 is reserved to create a ws and get DIS_DBG_SERVICE list
  DisServiceIndexClicked_S32 = DIS_CLIENT_INVALID_INDEX;

  try
  {
    ImGui::Begin("##Dis Service", &_rIsDisServiceVisible_B, ImGuiWindowFlags_None); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    ImGui::SetWindowPos(ImVec2(_x_U32, _y_U32), ImGuiCond_FirstUseEver);
    if (ImGui::TreeNodeEx("Dis Service", NodeFlag))
    {
      S_BuildHelpMarker("This is the list of the different target to which you can connect.\n"
                        "Click to select, CTRL+Click to toggle, click on arrows or double-click to open.");
      auto DisServiceRouteIterator = _rDisServiceJsonData.find("route");
      if ((DisServiceRouteIterator != _rDisServiceJsonData.end()) && (DisServiceRouteIterator->is_array()))
      {
        i_U32 = 1; // mDisServiceCollection entry 0 is reserved to create a ws and get DIS_DBG_SERVICE list
        for (const auto &rRoute : *DisServiceRouteIterator)
        {
          mDisServiceCollection.emplace_back(); // because we must do: rDisDbgService_X.WsCbParam_X.pDisService_X = &rDisDbgService_X
          DIS_DBG_SERVICE &rDisDbgService_X = mDisServiceCollection.back();
          rDisDbgService_X.Reset();
          rDisDbgService_X.Name_S = rRoute.value("name", "Unknown");
          rDisDbgService_X.IpAddress_S = rRoute.value("ip", "0.0.0.0:0");
          rDisDbgService_X.StateTimer_U32 = BOF::Bof_GetMsTickCount();
          rDisDbgService_X.DisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_IDLE;
          rDisDbgService_X.WsCbParam_X.pDisClient = this;
          rDisDbgService_X.WsCbParam_X.pDisService_X = &rDisDbgService_X;

          if (_rDisServiceIndex_S32 == i_U32)
          {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
          }
          IsNodeOpen_B = ImGui::TreeNodeEx((void *)(intptr_t)i_U32, NodeFlag, "%s", rDisDbgService_X.Name_S.c_str());
          if (_rDisServiceIndex_S32 == i_U32)
          {
            ImGui::PopStyleColor();
          }
          if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
          {
            DisServiceIndexClicked_S32 = i_U32;
          }
          if (ImGui::IsItemHovered())
          {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
          }

          if (IsNodeOpen_B)
          {
            // Disable the default "open on single-click behavior" + set Selected flag according to our selection.
            // To alter selection we use IsItemClicked() && !IsItemToggledOpen(), so clicking on an arrow doesn't alter selection.
            ItemFlag = LeafFlag;
            ImGui::TreeNodeEx((void *)(intptr_t)i_U32, ItemFlag, "%s", rDisDbgService_X.IpAddress_S.c_str()); // Leaf so no ImGui::TreePop();
            // ImGui::TreePop(); no it is a child
          } // if (IsNodeOpen_B)
          ImGui::TreePop();
          i_U32++;
        } // for (const auto &rRoute : *DisServiceRouteIterator)
        if (DisServiceIndexClicked_S32 != DIS_CLIENT_INVALID_INDEX)
        {
          // Update selection state
          // (process outside of tree loop to avoid visual inconsistencies during the clicking frame)
          if ((ImGui::GetIO().KeyCtrl) && (_rDisServiceIndex_S32 == DisServiceIndexClicked_S32))
          {
            _rDisServiceIndex_S32 = DIS_CLIENT_INVALID_INDEX;
          }
          else // Depending on selection behavior you want, may want to preserve selection when clicking on item that is part of the selection
          {
            _rDisServiceIndex_S32 = DisServiceIndexClicked_S32;
            // selection_mask = (1 6<< mDisServiceSelected_U32); // Click to single-select
          }
        }
      } // if ((DisServiceIterator != _rDisServiceJsonData.end()) && (DisServiceIterator->is_array()))
      ImGui::TreePop();
    } // if (ImGui::TreeNodeEx("Dis Service", NodeFlag))
    ImGui::End();
  }
  catch (nlohmann::json::exception &re)
  {
    DisClient::S_Log("[%u] DisplayDisService: exception caught '%s'\n", BOF::Bof_GetMsTickCount(), re.what());
  }
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

//_rSubPageIndex_S32 return the SubPageIndex value selected or DIS_CLIENT_INVALID_INDEX.
// _rSubPageIndex_S32 is different from Index_S32 (below) and is between 0 and max number of sub page of the currently selected page returned in _rPageIndex_S32
void DisClient::DisplayPageLayout(int32_t _x_U32, int32_t _y_U32, DIS_DBG_SERVICE &_rDisService_X, int32_t &_rPageIndex_S32, int32_t &_rSubPageIndex_S32)
{
  ImGuiTreeNodeFlags NodeFlag = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanAllColumns;
  ImGuiTreeNodeFlags LeafFlag = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanAllColumns;
  ImGuiTreeNodeFlags ItemFlag;
  uint32_t i_U32, j_U32;
  int32_t PageLayoutPageIndexClicked_S32, PageLayoutSubPageIndexClicked_S32;
  bool IsNodeOpen_B;
  DIS_DBG_PAGE_LAYOUT DisDbgPage_X;
  DIS_DBG_SUB_PAGE_LAYOUT DisDbgSubPage_X;
  DIS_DBG_PAGE_PARAM DisDbgPageParam_X;

  _rDisService_X.PageLayoutCollection.clear();
  PageLayoutPageIndexClicked_S32 = DIS_CLIENT_INVALID_INDEX;
  PageLayoutSubPageIndexClicked_S32 = DIS_CLIENT_INVALID_INDEX;

  try
  {
    ImGui::Begin("##Page Layout", &_rDisService_X.IsPageLayoutVisible_B, ImGuiWindowFlags_None); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    ImGui::SetWindowPos(ImVec2(_x_U32, _y_U32), ImGuiCond_FirstUseEver);
    // ImGuiWindow *window = ImGui::FindWindowByName("##PageLayout");
    // ImGuiWindow *window1 = ImGui::GetCurrentWindow();
    // DisClient::S_Log("FindWindowByName %p/%p\n", window, window1);
    // DisClient::S_Log("-2->%d\n", window->IDStack.Size);
    if (ImGui::TreeNodeEx(_rDisService_X.Name_S.c_str(), NodeFlag))
    {
      S_BuildHelpMarker("This is the list of the different debug screen that are availablke from your current debug target.\n"
                        "Click to select, CTRL+Click to toggle, click on arrows or double-click to open.");
      // 'selection_mask' is dumb representation of what may be user-side selection state.
      //  You may retain selection state inside or outside your objects in whatever format you see fit.
      // 'PageLayoutLeafIdClicked_U32' is temporary storage of what node we have clicked to process selection at the end
      /// of the loop. May be a pointer to your own node type, etc.

      // Check if "page" exists and is an array
      auto PageIterator = _rDisService_X.PageLayoutJsonData.find("page");
      if ((PageIterator != _rDisService_X.PageLayoutJsonData.end()) && (PageIterator->is_array()))
      {
        const auto &PageCollection = *PageIterator;
        for (i_U32 = 0; i_U32 < PageCollection.size(); i_U32++)
        {
          const auto &CrtPage = PageCollection[i_U32];
          DisDbgPage_X.Reset();
          DisDbgPage_X.Label_S = CrtPage.value("label", "Label_NotFound_" + std::to_string(i_U32));
          DisDbgPage_X.Page_U32 = CrtPage.value("page", 0);
          DisDbgPage_X.MaxChannel_U32 = CrtPage.value("maxChnl", 0);
          IsNodeOpen_B = ImGui::TreeNodeEx((void *)(intptr_t)i_U32, NodeFlag, "%s", DisDbgPage_X.Label_S.c_str());
          if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
          {
            // Not selectable PageLayoutLeafIdClicked_U32 = i_U32;
          }
          DisDbgPage_X.DisDbgSubPageCollection.clear();
          auto SubPageIterator = CrtPage.find("subPage");
          if ((SubPageIterator != CrtPage.end()) && (SubPageIterator->is_array()))
          {
            const auto &SubPageCollection = *SubPageIterator;
            for (j_U32 = 0; j_U32 < SubPageCollection.size(); j_U32++)
            {
              const auto &CrtSubPage = SubPageCollection[j_U32];
              DisDbgSubPage_X.Reset();
              DisDbgSubPage_X.Label_S = CrtPage.value("label", "Label_NotFound_" + std::to_string(j_U32)) + '[' + std::to_string(DisDbgPage_X.MaxChannel_U32) + ']';
              DisDbgSubPage_X.Help_S = CrtPage.value("help", "Help_NotFound_" + std::to_string(j_U32));
              DisDbgPage_X.DisDbgSubPageCollection.push_back(DisDbgSubPage_X);
              if (IsNodeOpen_B)
              {
                // Disable the default "open on single-click behavior" + set Selected flag according to our selection.
                // To alter selection we use IsItemClicked() && !IsItemToggledOpen(), so clicking on an arrow doesn't alter selection.
                ItemFlag = LeafFlag;
                if (ImGui::IsItemHovered())
                {
                  ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                }
                if ((_rPageIndex_S32 == i_U32) && (_rSubPageIndex_S32 == j_U32))
                {
                  ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
                }
                ImGui::TreeNodeEx((void *)(intptr_t)j_U32, ItemFlag, "%s", CrtSubPage.value("label", "SubPage_NotFound_" + std::to_string(j_U32)).c_str()); // Leaf so no ImGui::TreePop();
                if ((_rPageIndex_S32 == i_U32) && (_rSubPageIndex_S32 == j_U32))
                {
                  ImGui::PopStyleColor();
                }
                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
                {
                  PageLayoutPageIndexClicked_S32 = i_U32;
                  PageLayoutSubPageIndexClicked_S32 = j_U32;
                }
              }
            } // for (j_U32 = 0; j_U32 < SubPageCollection.size(); j_U32++)
            if (IsNodeOpen_B)
            {
              ImGui::TreePop();
            }
          } // if ((SubPageIterator != CrtPage.end()) && (SubPageIterator->is_array()))
          _rDisService_X.PageLayoutCollection.push_back(DisDbgPage_X);
        } // for (i = 0; i < pages.size(); ++i)
        if (PageLayoutPageIndexClicked_S32 != DIS_CLIENT_INVALID_INDEX)
        {
          // Update selection state
          // (process outside of tree loop to avoid visual inconsistencies during the clicking frame)
          if ((ImGui::GetIO().KeyCtrl) && (_rPageIndex_S32 == PageLayoutPageIndexClicked_S32) && (_rSubPageIndex_S32 == PageLayoutSubPageIndexClicked_S32))
          {
            _rPageIndex_S32 = DIS_CLIENT_INVALID_INDEX;
            _rSubPageIndex_S32 = DIS_CLIENT_INVALID_INDEX;
          }
          else // Depending on selection behavior you want, may want to preserve selection when clicking on item that is part of the selection
          {
            _rPageIndex_S32 = PageLayoutPageIndexClicked_S32;
            _rSubPageIndex_S32 = PageLayoutSubPageIndexClicked_S32;
            // selection_mask = (1 6<< mPageLayoutLeafIdSelected_U32); // Click to single-select
          }
        }
        /*
        if ((PageLayoutPageIndexClicked_S32 != DIS_CLIENT_INVALID_INDEX) || (PageLayoutSubPageIndexClicked_S32 != DIS_CLIENT_INVALID_INDEX))
        {
          _rPageIndex_S32 = PageLayoutPageIndexClicked_S32;
          _rSubPageIndex_S32 = PageLayoutSubPageIndexClicked_S32;
        }
        */
        if (_rDisService_X.PageParamCollection.size() != _rDisService_X.PageLayoutCollection.size())
        {
          _rDisService_X.PageParamCollection.clear();
          for (i_U32 = 0; i_U32 < _rDisService_X.PageLayoutCollection.size(); i_U32++)
          {
            DisDbgPageParam_X.Reset();
            if (i_U32 == _rPageIndex_S32)
            {
              DisDbgPageParam_X.SubPageIndex_U32 = _rSubPageIndex_S32;
            }
            _rDisService_X.PageParamCollection.push_back(DisDbgPageParam_X);
          }
        }
        else
        {
          for (i_U32 = 0; i_U32 < _rDisService_X.PageParamCollection.size(); i_U32++)
          {
            DisDbgPageParam_X = _rDisService_X.PageParamCollection[i_U32];
            if (DisDbgPageParam_X.Channel_U32 < _rDisService_X.PageLayoutCollection[i_U32].MaxChannel_U32)
            {
              _rDisService_X.PageParamCollection[i_U32].Channel_U32 = 0;
            }
            if (i_U32 == _rPageIndex_S32)
            {
              DisDbgPageParam_X.SubPageIndex_U32 = _rSubPageIndex_S32;
            }
            _rDisService_X.PageParamCollection[i_U32] = DisDbgPageParam_X;
            i_U32++;
          }
        }
      } // if (pageIterator != _rDisService_X.PageLayoutJsonData.end() && PageIterator->is_array())
      ImGui::TreePop();
    } // if (ImGui::TreeNodeEx(_rDisService_X.Name_S.c_str(), NodeFlag))
    ImGui::End();
  }
  catch (nlohmann::json::exception &re)
  {
    DisClient::S_Log("[%u] DisplayPageLayout: %s exception caught '%s'\n", BOF::Bof_GetMsTickCount(), _rDisService_X.Name_S.c_str(), re.what());
  }
}
void DisClient::UpdateConsoleMenubar(DIS_DBG_SERVICE &_rDisService_X)
{
  std::string MenuBar_S, Text_S;
  DIS_DBG_PAGE_LAYOUT DisDbgPage_X;

  MenuBar_S = std::string(mDisClientParam_X.ConsoleWidth_U32, ' ');
  if (IS_PAGE_SUBPAGE_LAYOUT_VALID(&_rDisService_X))
  {
    DisDbgPage_X = _rDisService_X.PageLayoutCollection[_rDisService_X.PageLayoutIndex_S32];
    if (DisDbgPage_X.MaxChannel_U32)
    {
      Text_S = BOF::Bof_Sprintf("%03d: %-16s | %-24s | %02d/%02d",
                                DisDbgPage_X.Page_U32, DisDbgPage_X.Label_S.c_str(),
                                DisDbgPage_X.DisDbgSubPageCollection[_rDisService_X.SubPageLayoutIndex_S32].Label_S.c_str(),
                                _rDisService_X.PageParamCollection[_rDisService_X.PageLayoutIndex_S32].Channel_U32, DisDbgPage_X.MaxChannel_U32);
    }
    else
    {
      Text_S = BOF::Bof_Sprintf("%03d: %-16s | %-24s",
                                DisDbgPage_X.Page_U32, DisDbgPage_X.Label_S.c_str(),
                                DisDbgPage_X.DisDbgSubPageCollection[_rDisService_X.SubPageLayoutIndex_S32].Label_S.c_str());
    }
  }
  else
  {
    Text_S = "No data available";
  }
  MenuBar_S.replace(0, Text_S.size(), Text_S);
  PrintAt(0, 0, "#FFFFFF", "#0000FF", MenuBar_S);
  PrintAt(0, mDisClientParam_X.ConsoleHeight_U32 - 1, "#FFFFFF", "#0000FF", "Pg:[PgUp/PgDw]  SubPg:[Up/Dw]  Rst:[R]  Chnl:[+/-]  Inp:[!]  Hlp:[?]            ");
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

void DisClient::DisplayPageInfo(int32_t _x_U32, int32_t _y_U32, DIS_DBG_SERVICE &_rDisService_X)
{
  nlohmann::json *pLineInfoJsonData;
  std::string ForeColor_S, BackColor_S, Text_S;
  uint32_t i_U32, x_U32, y_U32;
  float Width_f, Height_f;

  if ((mConsoleCharSize_X.Width == 0) || (mConsoleCharSize_X.Height == 0))
  {
    ImGui::PushFont(mpConsoleFont_X);
    mConsoleCharSize_X = GetTextSize("A");
    ImGui::PopFont();
  }
  if ((mConsoleCharSize_X.Width) && (mConsoleCharSize_X.Height))
  {
    try
    {
      Width_f = mDisClientParam_X.ConsoleWidth_U32 * mConsoleCharSize_X.Width;
      Height_f = (mDisClientParam_X.ConsoleHeight_U32 * mConsoleCharSize_X.Height) + IMGUI_WINDOW_TITLEBAR_HEIGHT;
      // ImGui::SetNextWindowSize(ImVec2(Width_f, Height_f));
      // ImGui::SetNextWindowSizeConstraints(ImVec2(Width_f, Height_f), ImVec2(Width_f, Height_f));
      ImGui::PushStyleColor(ImGuiCol_WindowBg, _rDisService_X.PageInfoBackColor_X); // Clear all
      ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar);
      ImGui::SetWindowPos(ImVec2(_x_U32, _y_U32), ImGuiCond_FirstUseEver);
      ImGui::SetWindowSize(ImVec2(Width_f, Height_f), ImGuiCond_FirstUseEver);

      // Screen
      for (i_U32 = 0; i_U32 < 2; i_U32++) // i_U32==0: Draw background i_U32==1: Draw foreground
      {
        if (i_U32 == 0)
        {
          UpdateConsoleMenubar(_rDisService_X);

          pLineInfoJsonData = &_rDisService_X.PageInfoBackJsonData;
        }
        else
        {
          pLineInfoJsonData = &_rDisService_X.PageInfoForeJsonData;
        }
        for (const auto &rLine : (*pLineInfoJsonData))
        {
          x_U32 = rLine.value("x", 0);
          y_U32 = rLine.value("y", 0);
          ForeColor_S = rLine.value("fg", "#FFFFFF");
          BackColor_S = rLine.value("bg", "#000000");
          Text_S = rLine.value("text", "");
          PrintAt(x_U32, y_U32, ForeColor_S, BackColor_S, Text_S);
        }
      }
      // Keyboard
      std::string KeyState_S = S_GetKeyboardState();
      static std::string S_LastKeyState_S;
      if (KeyState_S != "")
      {
        S_LastKeyState_S = KeyState_S;
      }
      ImGui::SetCursorPos(ImVec2(500, 60));
      ImGui::TextColored(ImVec4(255, 255, 0, 1.0), "%s", S_LastKeyState_S.c_str());
      ImGui::End();
      ImGui::PopStyleColor();
    }
    catch (nlohmann::json::exception &re)
    {
      DisClient::S_Log("[%u] DisplayPageInfo: %s exception caught '%s'\n", BOF::Bof_GetMsTickCount(), _rDisService_X.Name_S.c_str(), re.what());
    }
  } // if ((mConsoleCharSize_X.Width) && (mConsoleCharSize_X.Height))
}
void DisClient::V_PreNewFrame()
{
  BOFERR Sts_E, StsCmd_E;
  bool Rts_B = false;
  uint32_t i_U32, DeltaIdle_U32, DeltaState_U32, DeltaPageInfo_U32;
  nlohmann::json PageInfoJsonData;
  BOF::BOF_RAW_CIRCULAR_BUFFER_PARAM RawCircularBufferParam_X;
#if defined(__EMSCRIPTEN__)
#else
  WEBRPC::WEB_SOCKET_PARAM WebSocketParam_X;
#endif

  DeltaIdle_U32 = BOF::Bof_ElapsedMsTime(mIdleTimer_U32);
  if (DeltaIdle_U32 > DIS_CLIENT_MAIN_LOOP_IDLE_TIME)
  {
    mIdleTimer_U32 = BOF::Bof_GetMsTickCount();
    i_U32 = 0; // mDisServiceCollection entry 0 is reserved to create a ws and get DIS_DBG_SERVICE list
    BOF_ASSERT(mDisServiceCollection.size());
    for (DIS_DBG_SERVICE &rDisService_X : mDisServiceCollection)
    {
      try
      {
        DeltaState_U32 = BOF::Bof_ElapsedMsTime(rDisService_X.StateTimer_U32);
        DeltaPageInfo_U32 = BOF::Bof_ElapsedMsTime(rDisService_X.PageInfoTimer_U32);
        Sts_E = BOF_ERR_NO_ERROR;
        StsCmd_E = BOF_ERR_NO;
        if (rDisService_X.DisClientState_E != rDisService_X.DisClientLastState_E)
        {
          DisClient::S_Log("[%u] ENTER: %s Sz %zd State change %s->%s: %s\n", BOF::Bof_GetMsTickCount(), rDisService_X.Name_S.c_str(), mDisServiceCollection.size(),
                           S_DisClientStateTypeEnumConverter.ToString(rDisService_X.DisClientLastState_E).c_str(),
                           S_DisClientStateTypeEnumConverter.ToString(rDisService_X.DisClientState_E).c_str(), rDisService_X.IsWebSocketConnected_B ? "Connected" : "Disconnected");
          rDisService_X.DisClientLastState_E = rDisService_X.DisClientState_E;
          rDisService_X.StateTimer_U32 = BOF::Bof_GetMsTickCount();
        }
        if (rDisService_X.IsWebSocketConnected_B)
        {
          StsCmd_E = IsCommandRunning(&rDisService_X) ? CheckForCommandReply(&rDisService_X, rDisService_X.LastWebSocketMessage_S) : BOF_ERR_NO;
        }
        switch (rDisService_X.DisClientState_E)
        {
          case DIS_CLIENT_STATE::DIS_CLIENT_STATE_IDLE:
            RawCircularBufferParam_X.MultiThreadAware_B = true;
            RawCircularBufferParam_X.SlotMode_B = false;
            RawCircularBufferParam_X.AlwaysContiguous_B = true;
            RawCircularBufferParam_X.BufferSizeInByte_U32 = DIS_CLIENT_RX_BUFFER_SIZE;
            RawCircularBufferParam_X.NbMaxBufferEntry_U32 = DIS_CLIENT_NB_MAX_BUFFER_ENTRY;
            RawCircularBufferParam_X.pData_U8 = nullptr;
            RawCircularBufferParam_X.Overwrite_B = false;
            RawCircularBufferParam_X.Blocking_B = true;
            rDisService_X.puRxBufferCollection = std::make_unique<BOF::BofRawCircularBuffer>(RawCircularBufferParam_X);
            if ((rDisService_X.puRxBufferCollection) && (rDisService_X.puRxBufferCollection->LastErrorCode() == BOF_ERR_NO_ERROR))
            {
              rDisService_X.DisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_CREATE_WS;
            }
            break;

          case DIS_CLIENT_STATE::DIS_CLIENT_STATE_CREATE_WS:
            rDisService_X.WsCbParam_X.pDisClient = this;
            rDisService_X.WsCbParam_X.pDisService_X = &rDisService_X;
#if defined(__EMSCRIPTEN__)
#else
            WebSocketParam_X.pUser = &rDisService_X.WsCbParam_X;

            DisClient::S_Log("ptr mDisServiceCollection %p pDisClient %p pDisService_X %p\n", WebSocketParam_X.pUser, rDisService_X.WsCbParam_X.pDisClient, rDisService_X.WsCbParam_X.pDisService_X);

            WebSocketParam_X.NbMaxOperationPending_U32 = 4;
            WebSocketParam_X.RxBufferSize_U32 = 0x10000;
            WebSocketParam_X.NbMaxBufferEntry_U32 = 128;
            WebSocketParam_X.OnOperation = nullptr;
            WebSocketParam_X.OnOpen = BOF_BIND_1_ARG_TO_METHOD(this, DisClient::OnWebSocketOpenEvent);
            WebSocketParam_X.OnClose = BOF_BIND_1_ARG_TO_METHOD(this, DisClient::OnWebSocketCloseEvent);
            WebSocketParam_X.OnError = BOF_BIND_2_ARG_TO_METHOD(this, DisClient::OnWebSocketErrorEvent);
            WebSocketParam_X.OnMessage = BOF_BIND_4_ARG_TO_METHOD(this, DisClient::OnWebSocketMessageEvent);
            WebSocketParam_X.NbMaxClient_U32 = 0; // 0: client
            // WebSocketParam_X.ServerIp_X = BOF::BOF_SOCKET_ADDRESS(mDisClientParam_X.DisServerEndpoint_S);
            WebSocketParam_X.WebSocketThreadParam_X.Name_S = "DisClient_" + rDisService_X.Name_S;
            WebSocketParam_X.WebSocketThreadParam_X.ThreadSchedulerPolicy_E = BOF::BOF_THREAD_SCHEDULER_POLICY::BOF_THREAD_SCHEDULER_POLICY_OTHER;
            WebSocketParam_X.WebSocketThreadParam_X.ThreadPriority_E = BOF::BOF_THREAD_PRIORITY::BOF_THREAD_PRIORITY_000;
            rDisService_X.puDisClientWebSocket = std::make_unique<DisClientWebSocket>(WebSocketParam_X);
            if (rDisService_X.puDisClientWebSocket)
            {
              rDisService_X.puDisClientWebSocket->Run();
              rDisService_X.DisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_OPEN_WS;
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
              rDisService_X.DisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_IDLE;
            }
#endif
            break;

          case DIS_CLIENT_STATE::DIS_CLIENT_STATE_OPEN_WS:
            Sts_E = OpenWebSocket(&rDisService_X);
            if (Sts_E == BOF_ERR_NO_ERROR)
            {
              rDisService_X.DisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_CONNECT;
            }
            break;

          case DIS_CLIENT_STATE::DIS_CLIENT_STATE_CONNECT:
            if (rDisService_X.IsWebSocketConnected_B)
            {
              if (i_U32 == 0) // mDisServiceCollection entry 0 is reserved to create a ws and get DIS_DBG_SERVICE list
              {
                rDisService_X.DisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_GET_DIS_SERVICE;
              }
              else
              {
                rDisService_X.DisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_GET_DEBUG_PAGE_LAYOUT;
              }
            }
            break;

          case DIS_CLIENT_STATE::DIS_CLIENT_STATE_GET_DIS_SERVICE:
            if (StsCmd_E == BOF_ERR_NO)
            {
              rDisService_X.PageLayoutJsonData.clear();
              rDisService_X.PageInfoBackJsonData.clear();
              rDisService_X.PageInfoForeJsonData.clear();

              rDisService_X.PageParamCollection.clear();
              rDisService_X.PageLayoutCollection.clear();
              rDisService_X.PageLayoutIndex_S32 = DIS_CLIENT_INVALID_INDEX;
              rDisService_X.SubPageLayoutIndex_S32 = DIS_CLIENT_INVALID_INDEX;

              rDisService_X.CtrlFlag_U32 = DIS_CTRL_FLAG_NONE;
              rDisService_X.FctKeyFlag_U32 = DIS_KEY_FLAG_NONE;
              rDisService_X.UserInput_S = "";

              Sts_E = SendCommand(&rDisService_X, DIS_CLIENT_WS_TIMEOUT, "GET /DebugServiceRoute");
              if (Sts_E != BOF_ERR_NO_ERROR)
              {
                rDisService_X.DisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_OPEN_WS;
              }
            }
            else
            {
              if (StsCmd_E == BOF_ERR_NO_ERROR)
              {
                mDisServiceJsonData = nlohmann::json::parse(rDisService_X.LastWebSocketMessage_S);
                rDisService_X.DisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_GET_DEBUG_PAGE_LAYOUT;
              }
            }
            break;

          case DIS_CLIENT_STATE::DIS_CLIENT_STATE_GET_DEBUG_PAGE_LAYOUT:
            if (IS_DIS_SERVICE_VALID())
            {
              if (StsCmd_E == BOF_ERR_NO)
              {
                Sts_E = SendCommand(&rDisService_X, DIS_CLIENT_WS_TIMEOUT, "GET " + mDisServiceCollection[mDisServiceIndex_S32].Name_S + "DebugPageLayout");
                if (Sts_E != BOF_ERR_NO_ERROR)
                {
                  rDisService_X.DisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_OPEN_WS;
                }
              }
              else
              {
                if (StsCmd_E == BOF_ERR_NO_ERROR)
                {
                  rDisService_X.PageLayoutJsonData = nlohmann::json::parse(rDisService_X.LastWebSocketMessage_S);
                  rDisService_X.CtrlFlag_U32 = (DIS_CTRL_FLAG_BACK | DIS_CTRL_FLAG_FORE);
                  rDisService_X.DisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_GET_DEBUG_PAGE_INFO;
                }
              }
            }
            break;

          case DIS_CLIENT_STATE::DIS_CLIENT_STATE_GET_DEBUG_PAGE_INFO:
            if (IS_PAGE_SUBPAGE_LAYOUT_VALID(&rDisService_X))
            {
              if (StsCmd_E == BOF_ERR_NO)
              {
                if (DeltaPageInfo_U32 > mDisClientParam_X.PollTimeInMs_U32)
                {
                  rDisService_X.PageInfoTimer_U32 = BOF::Bof_GetMsTickCount();
                  //"GET /Gbe/DebugPageInfo/800/0/?chnl=0&flag=3&key=0&user_input=");
                  Sts_E = SendCommand(&rDisService_X, DIS_CLIENT_WS_TIMEOUT, "GET " + mDisServiceCollection[mDisServiceIndex_S32].Name_S + "DebugPageInfo/" + std::to_string(rDisService_X.PageLayoutCollection[rDisService_X.PageLayoutIndex_S32].Page_U32) + "/" + std::to_string(rDisService_X.PageParamCollection[rDisService_X.PageLayoutIndex_S32].SubPageIndex_U32) + "/?" + "chnl=" + std::to_string(rDisService_X.PageParamCollection[rDisService_X.PageLayoutIndex_S32].Channel_U32) + "&flag=" + std::to_string(rDisService_X.CtrlFlag_U32) + "&key=" + std::to_string(rDisService_X.FctKeyFlag_U32) + "&user_input=" + rDisService_X.UserInput_S);
                  if (Sts_E != BOF_ERR_NO_ERROR)
                  {
                    rDisService_X.DisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_OPEN_WS;
                  }
                }
              }
              else
              {
                if (StsCmd_E == BOF_ERR_NO_ERROR)
                {
                  rDisService_X.StateTimer_U32 = BOF::Bof_GetMsTickCount(); // Reset state timer as all is ok

                  if ((rDisService_X.CtrlFlag_U32 & (DIS_CTRL_FLAG_BACK | DIS_CTRL_FLAG_FORE)) == (DIS_CTRL_FLAG_BACK | DIS_CTRL_FLAG_FORE))
                  {
                    PageInfoJsonData = nlohmann::json::parse(rDisService_X.LastWebSocketMessage_S);
                    rDisService_X.PageInfoBackJsonData = PageInfoJsonData["background"]["line"];
                    rDisService_X.PageInfoForeJsonData = PageInfoJsonData["foreground"]["line"];
                    rDisService_X.PageInfoBackColor_X = S_HexaColor(PageInfoJsonData["background"].value("bg", "#000000"));
                    rDisService_X.CtrlFlag_U32 &= (0xFFFFFFFF - DIS_CTRL_FLAG_BACK);
                  }
                  else
                  {
                    if (rDisService_X.CtrlFlag_U32 & DIS_CTRL_FLAG_BACK)
                    {
                      rDisService_X.PageInfoBackJsonData = nlohmann::json::parse(rDisService_X.LastWebSocketMessage_S)["background"]["line"];
                    }
                    if (rDisService_X.CtrlFlag_U32 & DIS_CTRL_FLAG_FORE)
                    {
                      rDisService_X.PageInfoForeJsonData = nlohmann::json::parse(rDisService_X.LastWebSocketMessage_S)["foreground"]["line"];
                    }
                  }

                  if (rDisService_X.CtrlFlag_U32 & DIS_CTRL_FLAG_RESET)
                  {
                    rDisService_X.CtrlFlag_U32 &= (0xFFFFFFFF - DIS_CTRL_FLAG_RESET);
                  }
                  rDisService_X.DisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_GET_DEBUG_PAGE_INFO;
                }
              }
            }

            break;

          default:
            break;
        }

        if (rDisService_X.DisClientState_E != rDisService_X.DisClientLastState_E)
        {
          DisClient::S_Log("[%u] LEAVE: %s Sz %zd State change %s->%s: %s\n", BOF::Bof_GetMsTickCount(), rDisService_X.Name_S.c_str(), mDisServiceCollection.size(),
                           S_DisClientStateTypeEnumConverter.ToString(rDisService_X.DisClientLastState_E).c_str(),
                           S_DisClientStateTypeEnumConverter.ToString(rDisService_X.DisClientState_E).c_str(), rDisService_X.IsWebSocketConnected_B ? "Connected" : "Disconnected");
          rDisService_X.DisClientLastState_E = rDisService_X.DisClientState_E;
          rDisService_X.StateTimer_U32 = BOF::Bof_GetMsTickCount();
        }
        if (DeltaState_U32 > DIS_CLIENT_STATE_TIMEOUT)
        {
          rDisService_X.DisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_OPEN_WS;
          DisClient::S_Log("[%u] ERROR: %s Sz %zd State timeout (%d/%d)  %s->%s: %s\n", BOF::Bof_GetMsTickCount(), rDisService_X.Name_S.c_str(), mDisServiceCollection.size(),
                           DeltaState_U32, DIS_CLIENT_STATE_TIMEOUT, S_DisClientStateTypeEnumConverter.ToString(rDisService_X.DisClientLastState_E).c_str(),
                           S_DisClientStateTypeEnumConverter.ToString(rDisService_X.DisClientState_E).c_str(), rDisService_X.IsWebSocketConnected_B ? "Connected" : "Disconnected");
          DIS_CLIENT_END_OF_COMMAND(&rDisService_X);
          StsCmd_E = BOF_ERR_ENOTCONN;
          rDisService_X.StateTimer_U32 = BOF::Bof_GetMsTickCount();
        }
        i_U32++;
      }
      catch (nlohmann::json::exception &re)
      {
        DisClient::S_Log("[%u] V_PreNewFrame: %s %zd exception caught '%s'\n", BOF::Bof_GetMsTickCount(), rDisService_X.Name_S.c_str(), mDisServiceCollection.size(), re.what());
      }
    } // for (DIS_DBG_SERVICE &rDisService_X : mDisServiceCollection)
  }
}

BOFERR DisClient::V_RefreshGui()
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;
  int32_t DisServiceIndex_S32, PageLayoutIndex_S32, SubPageLayoutIndex_S32;
  uint32_t i_U32;

  if (mDisServiceJsonData.is_null())
  {
    mDisServiceIndex_S32 = DIS_CLIENT_INVALID_INDEX;
  }
  DisServiceIndex_S32 = mDisServiceIndex_S32;
  DisplayDisService(0, 0, mIsDisServiceVisible_B, mDisServiceJsonData, DisServiceIndex_S32);
  BOF_ASSERT(mDisServiceCollection.size()); // mDisServiceCollection entry 0 is reserved to create a ws and get DIS_DBG_SERVICE list
  if (DisServiceIndex_S32 != mDisServiceIndex_S32)
  {
    mDisServiceIndex_S32 = DisServiceIndex_S32;
    if (mDisServiceIndex_S32 == DIS_CLIENT_INVALID_INDEX)
    {
      mDisServiceCollection[0].DisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_OPEN_WS; // mDisServiceCollection entry 0 is reserved to create a ws and get DIS_DBG_SERVICE list
      mIdleTimer_U32 = 0;                                                                     // Start asap
    }
  }
  if (IS_DIS_SERVICE_VALID())
  {
    i_U32 = 0;
    for (DIS_DBG_SERVICE &rDisService_X : mDisServiceCollection)
    {
      if (i_U32) // mDisServiceCollection entry 0 is reserved to create a ws and get DIS_DBG_SERVICE list
      {
        if (rDisService_X.PageLayoutJsonData.is_null())
        {
          rDisService_X.PageLayoutIndex_S32 = DIS_CLIENT_INVALID_INDEX;
          rDisService_X.SubPageLayoutIndex_S32 = DIS_CLIENT_INVALID_INDEX;
        }
        else
        {
          PageLayoutIndex_S32 = rDisService_X.PageLayoutIndex_S32;
          SubPageLayoutIndex_S32 = rDisService_X.SubPageLayoutIndex_S32;
          DisplayPageLayout(200, 50, rDisService_X, PageLayoutIndex_S32, SubPageLayoutIndex_S32);
          if ((PageLayoutIndex_S32 != rDisService_X.PageLayoutIndex_S32) || (SubPageLayoutIndex_S32 != rDisService_X.SubPageLayoutIndex_S32))
          {
            rDisService_X.PageLayoutIndex_S32 = PageLayoutIndex_S32;
            rDisService_X.SubPageLayoutIndex_S32 = SubPageLayoutIndex_S32;
            if (IS_PAGE_SUBPAGE_LAYOUT_VALID(&rDisService_X))
            {
              rDisService_X.CtrlFlag_U32 = (DIS_CTRL_FLAG_BACK | DIS_CTRL_FLAG_FORE);
              mIdleTimer_U32 = 0; // Start asap
            }
          }
        }
        if (IS_PAGE_SUBPAGE_LAYOUT_VALID(&rDisService_X))
        {
          if ((rDisService_X.PageInfoBackJsonData.is_null()) || (rDisService_X.PageInfoForeJsonData.is_null()))
          {
          }
          else
          {
            DisplayPageInfo(600, 700, rDisService_X);
          }
        }
      }
      i_U32++;
    }
  }
  return Rts_E;
}
ImColor DisClient::S_HexaColor(const std::string &_rHexaColor_S)
{
  ImColor Rts_X(0, 0, 0, 0);
  static const std::regex S_RegExHexaColor("#([0-9A-Fa-f]{2})([0-9A-Fa-f]{2})([0-9A-Fa-f]{2})");
  std::smatch MatchRes;

  if (std::regex_match(_rHexaColor_S, MatchRes, S_RegExHexaColor))
  {
    if (MatchRes.size() == 4)
    {
      Rts_X = ImColor((int)BOF::Bof_StrToBin(16, MatchRes.str(1).c_str()), (int)BOF::Bof_StrToBin(16, MatchRes.str(2).c_str()), (int)BOF::Bof_StrToBin(16, MatchRes.str(3).c_str()), 255);
    }
  }
  return Rts_X;
}
BOFERR DisClient::PrintAt(uint32_t _x_U32, uint32_t _y_U32, const std::string &_rHexaForeColor_S, const std::string &_rHexaBackColor_S, const std::string &_rText_S)
{
  BOFERR Rts_E = BOF_ERR_EINVAL;
  BOF::Bof_ImGui_ImTextCustomization TextCustomization_X;
  BOF::BOF_POINT_2D<int32_t> CursorPos_X;
  ImColor Fore_X, Back_X;

  if ((_x_U32 < mDisClientParam_X.ConsoleWidth_U32) && (_y_U32 < mDisClientParam_X.ConsoleHeight_U32) && ((_x_U32 + _rText_S.size()) <= mDisClientParam_X.ConsoleWidth_U32))
  {
    Fore_X = S_HexaColor(_rHexaForeColor_S);
    Back_X = S_HexaColor(_rHexaBackColor_S);
    // TextCustomization_X.Highlight
    CursorPos_X.x = _x_U32 * mConsoleCharSize_X.Width;
    CursorPos_X.y = (_y_U32 * mConsoleCharSize_X.Height) + IMGUI_WINDOW_TITLEBAR_HEIGHT;
    Rts_E = DisplayText(&CursorPos_X, _rText_S.c_str(), false, false, &TextCustomization_X.Clear().Range(_rText_S.c_str()).TextColor(Fore_X).Highlight(Back_X));
  }
  return Rts_E;
}

bool DisClient::IsCommandRunning(DIS_DBG_SERVICE *_pDisService_X)
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
BOFERR DisClient::SendCommand(DIS_DBG_SERVICE *_pDisService_X, uint32_t _TimeoutInMsForReply_U32, const std::string &_rCmd_S)
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
      if (Rts_E == BOF_ERR_NO_ERROR)
      {
        _pDisService_X->LastCmdSentTimeoutInMs_U32 = _TimeoutInMsForReply_U32;
      }
    }
  }
  return Rts_E;
}
BOFERR DisClient::CheckForCommandReply(DIS_DBG_SERVICE *_pDisService_X, std::string &_rReply_S)
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
    if (Rts_E == BOF_ERR_NO_ERROR)
    {
      Rts_E = BOF_ERR_EMPTY;
      if (MaxSize_U32)
      {
        pData_c[MaxSize_U32] = 0;
        Rts_E = BOF_ERR_EBADF;
        try
        {
          // DisClient::S_Log("===================>b\n%s\n", pData_c);
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
          DisClient::S_Log("[%u] CheckForCommandReply: %s exception caught '%s'\n", BOF::Bof_GetMsTickCount(), _pDisService_X ? _pDisService_X->Name_S.c_str() : "", re.what());
        }
      }
    }
  }
  return Rts_E;
}
BOFERR DisClient::ReadWebSocket(DIS_DBG_SERVICE *_pDisService_X, uint32_t &_rMaxSize_U32, char *_pData_c)
{
  BOFERR Rts_E = BOF_ERR_INTERNAL;

  if (_pDisService_X)
  {
    Rts_E = _pDisService_X->puRxBufferCollection->PopBuffer(0, &_rMaxSize_U32, (uint8_t *)_pData_c);
  }
  return Rts_E;
}

#if defined(__EMSCRIPTEN__)

BOFERR DisClient::OpenWebSocket(DIS_DBG_SERVICE *_pDisService_X)
{
  BOFERR Rts_E = BOF_ERR_INTERNAL;
  // EmscriptenWebSocketCreateAttributes WebSocketCreateAttribute_X = {"wss://echo.websocket.org", nullptr, EM_TRUE};
  // EmscriptenWebSocketCreateAttributes WebSocketCreateAttribute_X = {"ws://10.129.171.112:8080", nullptr, EM_TRUE};
  if (_pDisService_X)
  {
    Rts_E = BOF_ERR_NOT_SUPPORTED;
    EmscriptenWebSocketCreateAttributes WebSocketCreateAttribute_X = {_pDisService_X->IpAddress_S.c_str(), nullptr, EM_TRUE};

    if (emscripten_websocket_is_supported())
    {
      CloseWebSocket(_pDisService_X);
      _pDisService_X->Ws = emscripten_websocket_new(&WebSocketCreateAttribute_X);

      DisClient::S_Log("CREATE Ws %d\n", _pDisService_X->Ws);
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
BOFERR DisClient::CloseWebSocket(DIS_DBG_SERVICE *_pDisService_X)
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
BOFERR DisClient::WriteWebSocket(DIS_DBG_SERVICE *_pDisService_X, const char *_pData_c)
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

BOFERR DisClient::OnWebSocketOpenEvent(void *_pWsCbParam, const EmscriptenWebSocketOpenEvent *_pWebsocketEvent_X)
{
  BOFERR Rts_E = BOF_ERR_EINVAL;
  WS_CALLBBACK_PARAM *pWsCbParam_X = (WS_CALLBBACK_PARAM *)_pWsCbParam;

  if ((pWsCbParam_X) && (pWsCbParam_X->pDisClient) && (pWsCbParam_X->pDisService_X) && (_pWebsocketEvent_X))
  {
    pWsCbParam_X->pDisService_X->IsWebSocketConnected_B = true;
    Rts_E = BOF_ERR_NO_ERROR;
  }
  return Rts_E;
}
BOFERR DisClient::OnWebSocketErrorEvent(void *_pWsCbParam, const EmscriptenWebSocketErrorEvent *_pWebsocketEvent_X)
{
  BOFERR Rts_E = BOF_ERR_INTERNAL;
  WS_CALLBBACK_PARAM *pWsCbParam_X = (WS_CALLBBACK_PARAM *)_pWsCbParam;

  if ((pWsCbParam_X) && (pWsCbParam_X->pDisClient) && (pWsCbParam_X->pDisService_X) && (_pWebsocketEvent_X))
  {
    Rts_E = CloseWebSocket(pWsCbParam_X->pDisService_X);
    Rts_E = BOF_ERR_NO_ERROR;
  }
  return Rts_E;
}
BOFERR DisClient::OnWebSocketCloseEvent(void *_pWsCbParam, const EmscriptenWebSocketCloseEvent *_pWebsocketEvent_X)
{
  BOFERR Rts_E = BOF_ERR_INTERNAL;
  WS_CALLBBACK_PARAM *pWsCbParam_X = (WS_CALLBBACK_PARAM *)_pWsCbParam;

  if ((pWsCbParam_X) && (pWsCbParam_X->pDisClient) && (pWsCbParam_X->pDisService_X) && (_pWebsocketEvent_X))
  {
    pWsCbParam_X->pDisService_X->IsWebSocketConnected_B = false;
    pWsCbParam_X->pDisService_X->Ws = -1;
    DIS_CLIENT_END_OF_COMMAND(pWsCbParam_X->pDisService_X);
    Rts_E = BOF_ERR_NO_ERROR;
  }
  return Rts_E;
}
BOFERR DisClient::OnWebSocketMessageEvent(void *_pWsCbParam, const EmscriptenWebSocketMessageEvent *_pWebsocketEvent_X)
{
  BOFERR Rts_E = BOF_ERR_INTERNAL;
  WS_CALLBBACK_PARAM *pWsCbParam_X = (WS_CALLBBACK_PARAM *)_pWsCbParam;
  EMSCRIPTEN_RESULT Sts;
  BOF::BOF_RAW_BUFFER *pRawBuffer_X;
  bool FinalFragment_B;

  if ((pWsCbParam_X) && (pWsCbParam_X->pDisClient) && (pWsCbParam_X->pDisService_X) && (_pWebsocketEvent_X))
  {
    if (_pWebsocketEvent_X->isText)
    {
      // For only ascii chars.
      // DisClient::S_Log("----------------->TXT message from %d: %d:%s\n", _pWebsocketEvent_X->socket, _pWebsocketEvent_X->numBytes, _pWebsocketEvent_X->data);
    }
    pWsCbParam_X->pDisService_X->puRxBufferCollection->SetAppendMode(0, true, nullptr); // By default we append
    FinalFragment_B = true;
    Rts_E = pWsCbParam_X->pDisService_X->puRxBufferCollection->PushBuffer(0, _pWebsocketEvent_X->numBytes, (const uint8_t *)_pWebsocketEvent_X->data, &pRawBuffer_X);
    pWsCbParam_X->pDisService_X->puRxBufferCollection->SetAppendMode(0, !FinalFragment_B, &pRawBuffer_X); // If it was the final one, we stop append->Result is pushed and committed
    // DisClient::S_Log("err %d data %d:%p Rts:indx %d slot %x 1: %x:%p 2: %x:%p\n", Rts_E, _pWebsocketEvent_X->numBytes, (const uint8_t *)_pWebsocketEvent_X->data, pRawBuffer_X->IndexInBuffer_U32, pRawBuffer_X->SlotEmpySpace_U32, pRawBuffer_X->Size1_U32, pRawBuffer_X->pData1_U8, pRawBuffer_X->Size2_U32, pRawBuffer_X->pData2_U8);
    if (Rts_E != BOF_ERR_NO_ERROR)
    {
      CloseWebSocket(pWsCbParam_X->pDisService_X);
    }
  }
  return Rts_E;
}
#else
BOFERR DisClient::OpenWebSocket(DIS_DBG_SERVICE *_pDisService_X)
{
  BOFERR Rts_E = BOF_ERR_INIT;
  if ((_pDisService_X) && (_pDisService_X->puDisClientWebSocket))
  {
    Rts_E = _pDisService_X->puDisClientWebSocket->ResetWebSocketOperation();
    Rts_E = _pDisService_X->puDisClientWebSocket->Connect(DIS_CLIENT_WS_TIMEOUT, mDisClientParam_X.DisServerEndpoint_S, "DisClientWs");
  }
  return Rts_E;
}
BOFERR DisClient::CloseWebSocket(DIS_DBG_SERVICE *_pDisService_X)
{
  BOFERR Rts_E = BOF_ERR_INIT;
  if ((_pDisService_X) && (_pDisService_X->puDisClientWebSocket))
  {
    Rts_E = _pDisService_X->puDisClientWebSocket->Disconnect(DIS_CLIENT_WS_TIMEOUT);
  }
  return Rts_E;
}
BOFERR DisClient::WriteWebSocket(DIS_DBG_SERVICE *_pDisService_X, const char *_pData_c)
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
BOFERR DisClient::OnWebSocketOpenEvent(void *_pWsCbParam)
{
  BOFERR Rts_E = BOF_ERR_INTERNAL;
  WS_CALLBBACK_PARAM *pWsCbParam_X = (WS_CALLBBACK_PARAM *)_pWsCbParam;

  if ((pWsCbParam_X) && (pWsCbParam_X->pDisClient) && (pWsCbParam_X->pDisService_X))
  {
    pWsCbParam_X->pDisService_X->IsWebSocketConnected_B = true;
    Rts_E = BOF_ERR_NO_ERROR;
  }
  return Rts_E;
}
BOFERR DisClient::OnWebSocketErrorEvent(void *_pWsCbParam, BOFERR _Sts_E)
{
  BOFERR Rts_E = BOF_ERR_INTERNAL;
  WS_CALLBBACK_PARAM *pWsCbParam_X = (WS_CALLBBACK_PARAM *)_pWsCbParam;

  if ((pWsCbParam_X) && (pWsCbParam_X->pDisClient) && (pWsCbParam_X->pDisService_X))
  {
    Rts_E = CloseWebSocket(pWsCbParam_X->pDisService_X);
    pWsCbParam_X->pDisService_X->Ws = reinterpret_cast<uintptr_t>(pWsCbParam_X->pDisService_X);
    Rts_E = BOF_ERR_NO_ERROR;
  }
  return Rts_E;
}
BOFERR DisClient::OnWebSocketCloseEvent(void *_pWsCbParam)
{
  BOFERR Rts_E = BOF_ERR_INTERNAL;
  WS_CALLBBACK_PARAM *pWsCbParam_X = (WS_CALLBBACK_PARAM *)_pWsCbParam;

  if ((pWsCbParam_X) && (pWsCbParam_X->pDisClient) && (pWsCbParam_X->pDisService_X))
  {
    pWsCbParam_X->pDisService_X->IsWebSocketConnected_B = false;
    pWsCbParam_X->pDisService_X->Ws = -1;
    DIS_CLIENT_END_OF_COMMAND(pWsCbParam_X->pDisService_X);
    Rts_E = BOF_ERR_NO_ERROR;
  }
  return Rts_E;
}
BOFERR DisClient::OnWebSocketMessageEvent(void *_pWsCbParam, uint32_t _Nb_U32, uint8_t *_pData_U8, BOF::BOF_BUFFER &_rReply_X)
{
  BOFERR Rts_E = BOF_ERR_INTERNAL;
  WS_CALLBBACK_PARAM *pWsCbParam_X = (WS_CALLBBACK_PARAM *)_pWsCbParam;
  BOF::BOF_RAW_BUFFER *pRawBuffer_X;
  bool FinalFragment_B;

  if ((pWsCbParam_X) && (pWsCbParam_X->pDisClient) && (pWsCbParam_X->pDisService_X) && (_pData_U8))
  {
    pWsCbParam_X->pDisService_X->puRxBufferCollection->SetAppendMode(0, true, nullptr); // By default we append
    FinalFragment_B = true;
    Rts_E = pWsCbParam_X->pDisService_X->puRxBufferCollection->PushBuffer(0, _Nb_U32, _pData_U8, &pRawBuffer_X);
    pWsCbParam_X->pDisService_X->puRxBufferCollection->SetAppendMode(0, !FinalFragment_B, &pRawBuffer_X); // If it was the final one, we stop append->Result is pushed and committed
    // DisClient::S_Log("err %d data %d:%p Rts:indx %d slot %x 1: %x:%p 2: %x:%p\n", Rts_E, _Nb_U32, _pData_U8, pRawBuffer_X->IndexInBuffer_U32, pRawBuffer_X->SlotEmpySpace_U32, pRawBuffer_X->Size1_U32, pRawBuffer_X->pData1_U8, pRawBuffer_X->Size2_U32, pRawBuffer_X->pData2_U8);
    if (Rts_E != BOF_ERR_NO_ERROR)
    {
      CloseWebSocket(pWsCbParam_X->pDisService_X);
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
