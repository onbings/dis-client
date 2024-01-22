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
#include <bofstd/boffs.h>
#include "dis_client.h"

#include <bofstd/bofenum.h>
#include <bofstd/bofhttprequest.h>
#include <regex>

#if defined(_WIN32)
#include <Windows.h> //OutputDebugString
#endif

uint32_t DisClient::S_mSeqId_U32 = 1;
#define DIS_CLIENT_WEB_TIMEOUT 2000
#define IMGUI_WINDOW_TITLEBAR_HEIGHT 20

static BOF::BofEnum<DIS_CLIENT_STATE> S_DisClientStateTypeEnumConverter(
    {
        {DIS_CLIENT_STATE::DIS_CLIENT_STATE_IDLE, "Idle"},
        {DIS_CLIENT_STATE::DIS_CLIENT_STATE_OPEN_WS, "OpenWs"},
        {DIS_CLIENT_STATE::DIS_CLIENT_STATE_CONNECT, "Connect"},
        {DIS_CLIENT_STATE::DIS_CLIENT_STATE_GET_DEBUG_SERVICE_ROUTE, "GetDbgSrvRoute"},
        {DIS_CLIENT_STATE::DIS_CLIENT_STATE_GET_DEBUG_PAGE_LAYOUT, "GetDbgPageLayout"},
        {DIS_CLIENT_STATE::DIS_CLIENT_STATE_GET_DEBUG_PAGE_INFO, "GetDbgPageInfo"},
        {DIS_CLIENT_STATE::DIS_CLIENT_STATE_MAX, "Max"},
    },
    DIS_CLIENT_STATE::DIS_CLIENT_STATE_MAX);

#define DIS_CLIENT_END_OF_COMMAND()  \
  {                                  \
    mLastCmdSentTimeoutInMs_U32 = 0; \
    mWaitForReplyId_U32 = 0;         \
  }

#if defined(__EMSCRIPTEN__)
EM_BOOL WebSocket_OnOpen(int _Event_i, const EmscriptenWebSocketOpenEvent *_pWebsocketEvent_X, void *_pUserData)
{
  EM_BOOL Rts_B = EM_FALSE;
  DisClient *pDisClient = (DisClient *)_pUserData;
  if (pDisClient)
  {
    Rts_B = (pDisClient->OnWebSocketOpenEvent(_pWebsocketEvent_X) == BOF_ERR_NO_ERROR) ? EM_TRUE : EM_FALSE;
  }
  return Rts_B;
}

EM_BOOL WebSocket_OnError(int _Event_i, const EmscriptenWebSocketErrorEvent *_pWebsocketEvent_X, void *_pUserData)
{
  EM_BOOL Rts_B = EM_FALSE;
  DisClient *pDisClient = (DisClient *)_pUserData;
  if (pDisClient)
  {
    Rts_B = (pDisClient->OnWebSocketErrorEvent(_pWebsocketEvent_X) == BOF_ERR_NO_ERROR) ? EM_TRUE : EM_FALSE;
  }
  return Rts_B;
}
EM_BOOL WebSocket_OnClose(int _Event_i, const EmscriptenWebSocketCloseEvent *_pWebsocketEvent_X, void *_pUserData)
{
  EM_BOOL Rts_B = EM_FALSE;
  DisClient *pDisClient = (DisClient *)_pUserData;
  if (pDisClient)
  {
    Rts_B = (pDisClient->OnWebSocketCloseEvent(_pWebsocketEvent_X) == BOF_ERR_NO_ERROR) ? EM_TRUE : EM_FALSE;
  }
  return Rts_B;
}
EM_BOOL WebSocket_OnMessage(int _Event_i, const EmscriptenWebSocketMessageEvent *_pWebsocketEvent_X, void *_pUserData)
{
  EM_BOOL Rts_B = EM_FALSE;
  DisClient *pDisClient = (DisClient *)_pUserData;
  if (pDisClient)
  {
    Rts_B = (pDisClient->OnWebSocketMessageEvent(_pWebsocketEvent_X) == BOF_ERR_NO_ERROR) ? EM_TRUE : EM_FALSE;
  }
  return Rts_B;
}

#endif

DisClient::DisClient(const DIS_CLIENT_PARAM &_rDisClientParam_X)
    : BOF::Bof_ImGui(_rDisClientParam_X.ImguiParam_X)
{
  mDisClientParam_X = _rDisClientParam_X;
  mDisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_OPEN_WS;
  mPollTimer_U32 = BOF::Bof_GetMsTickCount();
  BOF::BOF_RAW_CIRCULAR_BUFFER_PARAM RawCircularBufferParam_X;
  RawCircularBufferParam_X.MultiThreadAware_B = true;
  RawCircularBufferParam_X.BufferSizeInByte_U32 = mDisClientParam_X.RxBufferSize_U32;
  RawCircularBufferParam_X.SlotMode_B = false;
  RawCircularBufferParam_X.AlwaysContiguous_B = true; // false;
  RawCircularBufferParam_X.NbMaxBufferEntry_U32 = mDisClientParam_X.NbMaxBufferEntry_U32;
  RawCircularBufferParam_X.pData_U8 = nullptr;
  RawCircularBufferParam_X.Overwrite_B = false;
  RawCircularBufferParam_X.Blocking_B = true;
  mpuRxBufferCollection = std::make_unique<BOF::BofRawCircularBuffer>(RawCircularBufferParam_X);
  if ((mpuRxBufferCollection) && (mpuRxBufferCollection->LastErrorCode() == BOF_ERR_NO_ERROR))
  {
#if defined(__EMSCRIPTEN__)
#else
    WEBRPC::WEB_SOCKET_PARAM WebSocketParam_X;
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
    WebSocketParam_X.WebSocketThreadParam_X.Name_S = "DisClient";
    WebSocketParam_X.WebSocketThreadParam_X.ThreadSchedulerPolicy_E = BOF::BOF_THREAD_SCHEDULER_POLICY::BOF_THREAD_SCHEDULER_POLICY_OTHER;
    WebSocketParam_X.WebSocketThreadParam_X.ThreadPriority_E = BOF::BOF_THREAD_PRIORITY::BOF_THREAD_PRIORITY_000;
    mpuDisClientWebSocket = std::make_unique<DisClientWebSocket>(WebSocketParam_X);
    if (mpuDisClientWebSocket)
    {
      mpuDisClientWebSocket->Run();
#if 0
      BOFERR Sts_E;
      uint8_t pData_U8[0x2000];
      uint32_t Nb_U32,i_U32;
      Sts_E = mpuDisClientWebSocket->Connect(DIS_CLIENT_WEB_TIMEOUT, mDisClientParam_X.DisServerEndpoint_S, "DisClientWs");
      //Nb_U32 = sizeof(pData_U8);
      //Sts_E = mpuDisClientWebSocket->Read(DIS_CLIENT_WEB_TIMEOUT, Nb_U32, pData_U8);

      for (i_U32 = 0; i_U32 < 10; i_U32++)
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
#endif
  }
}
DisClient::~DisClient()
{
  CloseWebSocket();
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
    Rts["Label"] = mDisClientParam_X.Label_S;
    Rts["DisServerEndpoint"] = mDisClientParam_X.DisServerEndpoint_S;
    Rts["Page"] = mDisClientParam_X.Page_U32;
    Rts["SubPage"] = mDisClientParam_X.SubPage_U32;
    Rts["RxBufferSize"] = mDisClientParam_X.RxBufferSize_U32;
    Rts["NbMaxBufferEntry"] = mDisClientParam_X.NbMaxBufferEntry_U32;
    Rts["ImguiParam.AppName"] = mDisClientParam_X.ImguiParam_X.AppName_S;
    Rts["ImguiParam.Size.Width"] = mDisClientParam_X.ImguiParam_X.Size_X.Width;
    Rts["ImguiParam.Size.Height"] = mDisClientParam_X.ImguiParam_X.Size_X.Height;
    //Rts["ImguiParam.TheLogger"] = mDisClientParam_X.ImguiParam_X.TheLogger;
    Rts["ImguiParam.ShowDemoWindow"] = mDisClientParam_X.ImguiParam_X.ShowDemoWindow_B;
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
    mDisClientParam_X.PollTimeInMs_U32 = _rJson["PollTimeInMs"].get<uint32_t>();
    mDisClientParam_X.FontSize_U32 = _rJson["FontSize"].get<uint32_t>();
    mDisClientParam_X.ConsoleWidth_U32 = _rJson["ConsoleWidth"].get<uint32_t>();
    mDisClientParam_X.ConsoleHeight_U32 = _rJson["ConsoleHeight"].get<uint32_t>();
    mDisClientParam_X.Label_S = _rJson["Label"].get<std::string>();
    mDisClientParam_X.DisServerEndpoint_S = _rJson["DisServerEndpoint"].get<std::string>();
    mDisClientParam_X.Page_U32 = _rJson["Page"].get<uint32_t>();
    mDisClientParam_X.SubPage_U32 = _rJson["SubPage"].get<uint32_t>();
    mDisClientParam_X.RxBufferSize_U32 = _rJson["RxBufferSize"].get<uint32_t>();
    mDisClientParam_X.NbMaxBufferEntry_U32 = _rJson["NbMaxBufferEntry"].get<uint32_t>();
    mDisClientParam_X.ImguiParam_X.AppName_S = _rJson["ImguiParam.AppName"].get<std::string>();
    mDisClientParam_X.ImguiParam_X.Size_X.Width = _rJson["ImguiParam.Size.Width"].get<uint32_t>();
    mDisClientParam_X.ImguiParam_X.Size_X.Height = _rJson["ImguiParam.Size.Height"].get<uint32_t>();
    //mDisClientParam_X.ImguiParam_X.TheLogger = _rJson["ImguiParam.TheLogger"].get<uint32_t>();
    mDisClientParam_X.ImguiParam_X.ShowDemoWindow_B = _rJson["ImguiParam.ShowDemoWindow"].get<bool>();
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
//#if defined(__EMSCRIPTEN__)
//#else
  HelloImGui::SaveUserPref(APP_NAME, StateSerialized_S);
//#endif
  return Rts_E;
}

BOFERR DisClient::V_ReadSettings()
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;
//#if defined(__EMSCRIPTEN__)
//#else
  std::string StateSerialized_S = HelloImGui::LoadUserPref(APP_NAME);
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
        DisClient::S_Log("Failed to load %s state from user pref\n", APP_NAME);
      }
    }
    catch (const std::exception &e)
    {
      DisClient::S_Log("V_ReadSettings: %s\n", e.what());
    }
  }
//#endif
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
void DisClient::DisplayServiceRoute(int32_t _x_U32, int32_t _y_U32, bool &_rIsServiceRouteVisible_B, const nlohmann::json &_rServiceRouteJsonData, int32_t &_rDisDbgRouteIndex_S32)
{
  ImGuiTreeNodeFlags NodeFlag = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanAllColumns;
  ImGuiTreeNodeFlags LeafFlag = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanAllColumns;
  ImGuiTreeNodeFlags ItemFlag;
  uint32_t i_U32;
  int32_t ServiceRouteNodeIdClicked_S32;
  bool IsNodeOpen_B;
  DIS_DBG_ROUTE DisDbgRoute_X;

  _rDisDbgRouteIndex_S32 = -1;
  mDisDbgRouteCollection.clear();
  ServiceRouteNodeIdClicked_S32 = -1;

  try
  {
    ImGui::Begin("Service Route", &_rIsServiceRouteVisible_B, ImGuiWindowFlags_None); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    ImGui::SetWindowPos(ImVec2(_x_U32, _y_U32), ImGuiCond_FirstUseEver);
    if (ImGui::TreeNodeEx("Service Route", NodeFlag))
    {
      S_BuildHelpMarker("This is the list of the different target to which you can connect.\n"
                        "Click to select, CTRL+Click to toggle, click on arrows or double-click to open.");
      auto RouteIterator = _rServiceRouteJsonData.find("route");
      if ((RouteIterator != _rServiceRouteJsonData.end()) && (RouteIterator->is_array()))
      {
        const auto &RouteCollection = *RouteIterator;
        ServiceRouteNodeIdClicked_S32 = -1;
        for (i_U32 = 0; i_U32 < RouteCollection.size(); i_U32++)
        {
          const auto &CrtRoute = RouteCollection[i_U32];
          DisDbgRoute_X.Reset();
          DisDbgRoute_X.Name_S = CrtRoute.value("name", "Name_NotFound_" + std::to_string(i_U32));
          DisDbgRoute_X.IpAddress_S = CrtRoute.value("ip", "Ip_NotFound_" + std::to_string(i_U32));
          mDisDbgRouteCollection.push_back(DisDbgRoute_X);

          if (mServiceRouteGuiSelected_S32 == i_U32)
          {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
          }
          IsNodeOpen_B = ImGui::TreeNodeEx((void *)(intptr_t)i_U32, NodeFlag, "%s", DisDbgRoute_X.Name_S.c_str());
          if (mServiceRouteGuiSelected_S32 == i_U32)
          {
            ImGui::PopStyleColor();
          }
          if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
          {
            ServiceRouteNodeIdClicked_S32 = i_U32;
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
            ImGui::TreeNodeEx((void *)(intptr_t)i_U32, ItemFlag, "%s", DisDbgRoute_X.IpAddress_S.c_str()); // Leaf so no ImGui::TreePop();
            // ImGui::TreePop(); no it is a child
          } // if (IsNodeOpen_B)
          ImGui::TreePop();
        } // for (i_U32 = 0; i_U32 < RouteCollection.size(); i_U32++)
        if (ServiceRouteNodeIdClicked_S32 != -1)
        {
          // Update selection state
          // (process outside of tree loop to avoid visual inconsistencies during the clicking frame)
          if ((ImGui::GetIO().KeyCtrl) && (mServiceRouteGuiSelected_S32 == ServiceRouteNodeIdClicked_S32))
          {
            mServiceRouteGuiSelected_S32 = -1;
          }
          else // Depending on selection behavior you want, may want to preserve selection when clicking on item that is part of the selection
          {
            mServiceRouteGuiSelected_S32 = ServiceRouteNodeIdClicked_S32;
            // selection_mask = (1 6<< mServiceRouteNodeIdSelected_U32); // Click to single-select
          }
        }
        if (mServiceRouteGuiSelected_S32 != -1)
        {
          _rDisDbgRouteIndex_S32 = mServiceRouteGuiSelected_S32;
        }
      } // if (RouteIterator != _rServiceRouteJsonData.end() && RouteIterator->is_array())
      ImGui::TreePop();
    } // if (ImGui::TreeNodeEx("Service Route", NodeFlag))
    ImGui::End();
  }
  catch (nlohmann::json::exception &re)
  {
    DisClient::S_Log("DisplayServiceRoute: exception caught '%s'\n", re.what());
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

//_rSubPageIndex_S32 return the SubPageIndex value selected or -1.
// _rSubPageIndex_S32 is different from Index_S32 (below) and is between 0 and max number of sub page of the currently selected page returned in _rPageIndex_S32
void DisClient::DisplayPageLayout(int32_t _x_U32, int32_t _y_U32, bool &_rIsPageLayoutVisible_B, const nlohmann::json &_rPageLayoutJsonData, int32_t &_rPageIndex_S32, int32_t &_rSubPageIndex_S32)
{
  ImGuiTreeNodeFlags NodeFlag = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanAllColumns;
  ImGuiTreeNodeFlags LeafFlag = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanAllColumns;
  ImGuiTreeNodeFlags ItemFlag;
  uint32_t i_U32, j_U32;
  int32_t PageLayoutLeafIdClicked_S32, Index_S32, PageIndex_S32, SubPageIndex_S32;
  bool IsNodeOpen_B;
  DIS_DBG_PAGE DisDbgPage_X;
  DIS_DBG_SUB_PAGE DisDbgSubPage_X;
  DIS_DBG_PAGE_PARAM DisDbgPageParam_X;

  _rPageIndex_S32 = -1;
  _rSubPageIndex_S32 = -1;
  PageIndex_S32 = -1;
  SubPageIndex_S32 = -1;
  mDisDbgPageCollection.clear();
  PageLayoutLeafIdClicked_S32 = -1;

  try
  {
    ImGui::Begin("Page Layout", &_rIsPageLayoutVisible_B, ImGuiWindowFlags_None); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    ImGui::SetWindowPos(ImVec2(_x_U32, _y_U32), ImGuiCond_FirstUseEver);
    // ImGuiWindow *window = ImGui::FindWindowByName("##PageLayout");
    // ImGuiWindow *window1 = ImGui::GetCurrentWindow();
    // DisClient::S_Log("FindWindowByName %p/%p\n", window, window1);
    // DisClient::S_Log("-2->%d\n", window->IDStack.Size);
    if (ImGui::TreeNodeEx(_rPageLayoutJsonData.value("label", "Unknown").c_str(), NodeFlag))
    {
      S_BuildHelpMarker("This is the list of the different debug screen that are availablke from your current debug target.\n"
                        "Click to select, CTRL+Click to toggle, click on arrows or double-click to open.");
      // 'selection_mask' is dumb representation of what may be user-side selection state.
      //  You may retain selection state inside or outside your objects in whatever format you see fit.
      // 'PageLayoutLeafIdClicked_U32' is temporary storage of what node we have clicked to process selection at the end
      /// of the loop. May be a pointer to your own node type, etc.

      // Check if "page" exists and is an array
      auto PageIterator = _rPageLayoutJsonData.find("page");
      if ((PageIterator != _rPageLayoutJsonData.end()) && (PageIterator->is_array()))
      {
        const auto &PageCollection = *PageIterator;
        PageLayoutLeafIdClicked_S32 = -1;
        Index_S32 = 0;

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
              DisDbgSubPage_X.Label_S = CrtPage.value("label", "Label_NotFound_" + std::to_string(j_U32));
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
                if (mPageLayoutGuiSelected_S32 == Index_S32)
                {
                  PageIndex_S32 = i_U32;
                  SubPageIndex_S32 = j_U32;
                  ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
                }
                ImGui::TreeNodeEx((void *)(intptr_t)j_U32, ItemFlag, "%s", CrtSubPage.value("label", "SubPage_NotFound_" + std::to_string(j_U32)).c_str()); // Leaf so no ImGui::TreePop();
                if (mPageLayoutGuiSelected_S32 == Index_S32)
                {
                  ImGui::PopStyleColor();
                }
                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
                {
                  PageIndex_S32 = i_U32;
                  SubPageIndex_S32 = j_U32;
                  PageLayoutLeafIdClicked_S32 = Index_S32;
                }
              }
              Index_S32++;
            } // for (j_U32 = 0; j_U32 < SubPageCollection.size(); j_U32++)
            if (IsNodeOpen_B)
            {
              ImGui::TreePop();
            }
          } // if ((SubPageIterator != CrtPage.end()) && (SubPageIterator->is_array()))
          mDisDbgPageCollection.push_back(DisDbgPage_X);
          Index_S32++;
        } // for (i = 0; i < pages.size(); ++i)
        if (PageLayoutLeafIdClicked_S32 != -1)
        {
          // Update selection state
          // (process outside of tree loop to avoid visual inconsistencies during the clicking frame)
          if ((ImGui::GetIO().KeyCtrl) && (mPageLayoutGuiSelected_S32 == PageLayoutLeafIdClicked_S32))
          {
            mPageLayoutGuiSelected_S32 = -1;
            PageIndex_S32 = -1;
            SubPageIndex_S32 = -1;
          }
          else // Depending on selection behavior you want, may want to preserve selection when clicking on item that is part of the selection
          {
            mPageLayoutGuiSelected_S32 = PageLayoutLeafIdClicked_S32;
            // selection_mask = (1 6<< mPageLayoutLeafIdSelected_U32); // Click to single-select
          }
        }
        if (mPageLayoutGuiSelected_S32 != -1)
        {
          _rPageIndex_S32 = PageIndex_S32;
          _rSubPageIndex_S32 = SubPageIndex_S32;
        }
        if (mDisDbgPageParamCollection.size() != mDisDbgPageCollection.size())
        {
          mDisDbgPageParamCollection.clear();
          for (i_U32 = 0; i_U32 < mDisDbgPageCollection.size(); i_U32++)
          {
            DisDbgPageParam_X.Reset();
            if (i_U32 == PageIndex_S32)
            {
              DisDbgPageParam_X.SubPageIndex_U32 = SubPageIndex_S32;
            }
            mDisDbgPageParamCollection.push_back(DisDbgPageParam_X);
          }
        }
        else
        {
          for (i_U32 = 0; i_U32 < mDisDbgPageParamCollection.size(); i_U32++)
          {
            DisDbgPageParam_X = mDisDbgPageParamCollection[i_U32];
            if (DisDbgPageParam_X.Channel_U32 < mDisDbgPageCollection[i_U32].MaxChannel_U32)
            {
              mDisDbgPageParamCollection[i_U32].Channel_U32 = 0;
            }
            if (i_U32 == PageIndex_S32)
            {
              DisDbgPageParam_X.SubPageIndex_U32 = SubPageIndex_S32;
            }
            mDisDbgPageParamCollection[i_U32] = DisDbgPageParam_X;
            i_U32++;
          }
        }
      } // if (pageIterator != _rPageLayoutJsonData.end() && pageIterator->is_array())
      ImGui::TreePop();
    } // if (ImGui::TreeNodeEx(_rPageLayoutJsonData.value("label", "Unknown").c_str(), NodeFlag))
    ImGui::End();
  }
  catch (nlohmann::json::exception &re)
  {
    DisClient::S_Log("DisplayPageLayout: exception caught '%s'\n", re.what());
  }
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
void DisClient::DisplayPageInfo(int32_t _x_U32, int32_t _y_U32, bool &_rIsPageInfoVisible_B, bool _IsBackground_B, const nlohmann::json &_rPageInfoJsonData)
{
  nlohmann::json *pLineInfoJsonData;
  std::string ForeColor_S, BackColor_S, Text_S;
  uint32_t x_U32, y_U32;
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
      if (_IsBackground_B)
      {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, mPageInfoBackColor_X);
      }
      ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar);
      ImGui::SetWindowPos(ImVec2(_x_U32, _y_U32), ImGuiCond_FirstUseEver);
      ImGui::SetWindowSize(ImVec2(Width_f, Height_f), ImGuiCond_FirstUseEver);

#if 0
      uint32_t x_U32, y_U32;

      x_U32 = 0;
      for (y_U32 = 0; y_U32 < mDisClientParam_X.ConsoleHeight_U32; y_U32++)
      {
        if (y_U32 == (mDisClientParam_X.ConsoleHeight_U32 / 2))
        {
          x_U32 = mDisClientParam_X.ConsoleWidth_U32 - strlen(("Hello from line " + std::to_string(y_U32)).c_str());
        }
        if (y_U32 < (mDisClientParam_X.ConsoleHeight_U32 / 2))
        {
          PrintAt(x_U32, y_U32, "#FF0000", "#00FF00", "Hello from line " + std::to_string(y_U32));
          x_U32++;
        }
        else
        {
          PrintAt(x_U32, y_U32, "#00FF00", "#0000FF", "Hello from line " + std::to_string(y_U32));
          x_U32--;
        }
      }
#endif
      // std::string g = _rPageInfoJsonData.dump(3);
      // DisClient::S_Log("IsBackground %d Json:\n%s\n", _IsBackground_B,g.c_str());
      if (_IsBackground_B)
      {
        pLineInfoJsonData = &mPageInfoBackJsonData;
      }
      else
      {
        pLineInfoJsonData = &mPageInfoForeJsonData;
      }
//      std::string g = (*pLineInfoJsonData).dump(3);
//      DisClient::S_Log("Json:\n%s\n", g.c_str());
      for (const auto &rLine : (*pLineInfoJsonData))
      {
        x_U32 = rLine.value("x", 0);
        y_U32 = rLine.value("y", 0);
        ForeColor_S = rLine.value("fg", "#FFFFFF");
        BackColor_S = rLine.value("bg", "#000000");
        Text_S = rLine.value("text", "");
        // DisClient::S_Log("==========================> %d Pos %d,%d F %s B %s T %s Bck %d mp %p it %p S %p E %p\n", t, x_U32, y_U32, ForeColor_S.c_str(), BackColor_S.c_str(), Text_S.c_str(),
        //                  _IsBackground_B,  pLineInfoJsonData, It, pLineInfoJsonData->begin(), pLineInfoJsonData->end());
        PrintAt(x_U32, y_U32, ForeColor_S, BackColor_S, Text_S);
      }
      std::string KeyState_S = S_GetKeyboardState();
      static std::string S_LastKeyState_S;
      if (KeyState_S != "")
      {
        S_LastKeyState_S = KeyState_S;
      }
      ImGui::SetCursorPos(ImVec2(500, 60));
      ImGui::TextColored(ImVec4(255, 255, 0, 1.0), "%s", S_LastKeyState_S.c_str());
      ImGui::End();
      if (_IsBackground_B)
      {
        ImGui::PopStyleColor();
      }
    }
    catch (nlohmann::json::exception &re)
    {
      DisClient::S_Log("DisplayPageInfo: exception caught '%s'\n", re.what());
    }
  } //if ((mConsoleCharSize_X.Width) && (mConsoleCharSize_X.Height))
}
void DisClient::V_PreNewFrame()
{
  BOFERR Sts_E, StsCmd_E;
  bool Rts_B = false;
  uint32_t Delta_U32, DeltaPoll_U32;
  nlohmann::json PageInfoJsonData;

  try
  {
    DeltaPoll_U32 = BOF::Bof_ElapsedMsTime(mPollTimer_U32);
    Sts_E = BOF_ERR_NO_ERROR;
    StsCmd_E = BOF_ERR_NO;

    if (mDisClientState_E != mDisClientLastState_E)
    {
      DisClient::S_Log("ENTER: State change %s->%s: %s\n", S_DisClientStateTypeEnumConverter.ToString(mDisClientLastState_E).c_str(), S_DisClientStateTypeEnumConverter.ToString(mDisClientState_E).c_str(), IsWebSocketConnected() ? "Connected" : "Disconnected");
      mDisClientLastState_E = mDisClientState_E;
    }
    if (IsWebSocketConnected())
    {
      StsCmd_E = IsCommandRunning() ? CheckForCommandReply(mLastWebSocketMessage_S) : BOF_ERR_NO;
    }
    switch (mDisClientState_E)
    {
      case DIS_CLIENT_STATE::DIS_CLIENT_STATE_OPEN_WS:
        Sts_E = OpenWebSocket();
        if (Sts_E == BOF_ERR_NO_ERROR)
        {
          mWsConnectTimer_U32 = BOF::Bof_GetMsTickCount();
          mDisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_CONNECT;
        }
        break;

      case DIS_CLIENT_STATE::DIS_CLIENT_STATE_CONNECT:
        if (IsWebSocketConnected())
        {
          mDisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_GET_DEBUG_SERVICE_ROUTE;
        }
        else
        {
          Delta_U32 = BOF::Bof_ElapsedMsTime(mWsConnectTimer_U32);
          if (Delta_U32 > DIS_CLIENT_WEB_TIMEOUT)
          {
            mDisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_OPEN_WS;
            DIS_CLIENT_END_OF_COMMAND();
            StsCmd_E = BOF_ERR_ENOTCONN;
          }
        }
        break;

      case DIS_CLIENT_STATE::DIS_CLIENT_STATE_GET_DEBUG_SERVICE_ROUTE:
        if (StsCmd_E == BOF_ERR_NO)
        {
          //No re-evaluated in DIS_CLIENT_STATE_GET_DEBUG_SERVICE_ROUTE mServiceRouteJsonData.clear();
          //No done in DisClient::DisplayServiceRoute mServiceRouteGuiSelected_S32 = -1;

          mPageLayoutJsonData.clear();
          mPageLayoutGuiSelected_S32 = -1;

          mPageInfoBackJsonData.clear();
          mPageInfoForeJsonData.clear();

          //No done in DisClient::DisplayServiceRoute mDisDbgRouteCollection.clear();
          //No done in DisClient::V_RefreshGui mDisDbgRouteIndex_S32 = -1;

          mDisDbgPageCollection.clear();
          mDisDbgPageIndex_S32 = -1;
          mDisDbgSubPageIndex_S32 = -1;

          mDisDbgPageParamCollection.clear();

          mCtrlFlag_U32 = DIS_CTRL_FLAG_NONE;
          mFctKeyFlag_U32 = DIS_KEY_FLAG_NONE;
          mUserInput_S = "";

          Sts_E = SendCommand(DIS_CLIENT_WEB_TIMEOUT, "GET /DebugServiceRoute");
        }
        else
        {
          if (StsCmd_E == BOF_ERR_NO_ERROR)
          {
            mServiceRouteJsonData = nlohmann::json::parse(mLastWebSocketMessage_S);
            mDisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_GET_DEBUG_PAGE_LAYOUT;
          }
        }
        break;

      case DIS_CLIENT_STATE::DIS_CLIENT_STATE_GET_DEBUG_PAGE_LAYOUT:
        if (mDisDbgRouteCollection.size() == 0)
        {
          mDisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_GET_DEBUG_SERVICE_ROUTE;
        }
        else
        {
          if ((mDisDbgRouteIndex_S32 >= 0) && (mDisDbgRouteIndex_S32 < mDisDbgRouteCollection.size()))
          {
            if (StsCmd_E == BOF_ERR_NO)
            {
              Sts_E = SendCommand(DIS_CLIENT_WEB_TIMEOUT, "GET " + mDisDbgRouteCollection[mDisDbgRouteIndex_S32].Name_S + "DebugPageLayout");
            }
            else
            {
              if (StsCmd_E == BOF_ERR_NO_ERROR)
              {
                mPageLayoutJsonData = nlohmann::json::parse(mLastWebSocketMessage_S);
                mCtrlFlag_U32 = (DIS_CTRL_FLAG_BACK | DIS_CTRL_FLAG_FORE);
                mDisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_GET_DEBUG_PAGE_INFO;
              }
            }
          }
        }
        break;

      case DIS_CLIENT_STATE::DIS_CLIENT_STATE_GET_DEBUG_PAGE_INFO:
        if (mDisDbgPageCollection.size() == 0)
        {
          mDisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_GET_DEBUG_PAGE_LAYOUT;
        }
        else
        {
          if ((mDisDbgPageIndex_S32 >= 0) && (mDisDbgSubPageIndex_S32 >= 0) && (mDisDbgPageIndex_S32 < mDisDbgPageCollection.size()) && (mDisDbgSubPageIndex_S32 < mDisDbgPageCollection[mDisDbgPageIndex_S32].DisDbgSubPageCollection.size()) && (mDisDbgSubPageIndex_S32 < mDisDbgPageParamCollection.size()))
          {
            if (StsCmd_E == BOF_ERR_NO)
            {
              if (DeltaPoll_U32 > mDisClientParam_X.PollTimeInMs_U32)
              {
                //"GET /Gbe/DebugPageInfo/800/0/?chnl=0&flag=3&key=0&user_input=");
                Sts_E = SendCommand(DIS_CLIENT_WEB_TIMEOUT, "GET " + mDisDbgRouteCollection[mDisDbgRouteIndex_S32].Name_S + "DebugPageInfo/" + std::to_string(mDisDbgPageCollection[mDisDbgPageIndex_S32].Page_U32) + "/" + std::to_string(mDisDbgPageParamCollection[mDisDbgPageIndex_S32].SubPageIndex_U32) + "/?" + "chnl=" + std::to_string(mDisDbgPageParamCollection[mDisDbgPageIndex_S32].Channel_U32) + "&flag=" + std::to_string(mCtrlFlag_U32) + "&key=" + std::to_string(mFctKeyFlag_U32) + "&user_input=" + mUserInput_S);
              }
            }
            else
            {
              if (StsCmd_E == BOF_ERR_NO_ERROR)
              {
                if ((mCtrlFlag_U32 & (DIS_CTRL_FLAG_BACK | DIS_CTRL_FLAG_FORE)) == (DIS_CTRL_FLAG_BACK | DIS_CTRL_FLAG_FORE))
                {
                  PageInfoJsonData = nlohmann::json::parse(mLastWebSocketMessage_S);
                  mPageInfoBackJsonData = PageInfoJsonData["background"]["line"];
                  mPageInfoForeJsonData = PageInfoJsonData["foreground"]["line"];
                  mPageInfoBackColor_X = S_HexaColor(PageInfoJsonData["background"].value("bg", "#000000"));
                  mCtrlFlag_U32 &= (0xFFFFFFFF - DIS_CTRL_FLAG_BACK);
                }
                else
                {
                  if (mCtrlFlag_U32 & DIS_CTRL_FLAG_BACK)
                  {
                    mPageInfoBackJsonData = nlohmann::json::parse(mLastWebSocketMessage_S)["background"]["line"];
                  }
                  if (mCtrlFlag_U32 & DIS_CTRL_FLAG_FORE)
                  {
                    mPageInfoForeJsonData = nlohmann::json::parse(mLastWebSocketMessage_S)["foreground"]["line"];
                  }
                }

                if (mCtrlFlag_U32 & DIS_CTRL_FLAG_RESET)
                {
                  mCtrlFlag_U32 &= (0xFFFFFFFF - DIS_CTRL_FLAG_RESET);
                }

                mDisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_GET_DEBUG_PAGE_INFO;
              }
            }
          }
        }
        break;

      default:
        break;
    }
    if (StsCmd_E == BOF_ERR_NO_ERROR)
    {
      // DisClient::S_Log("Reply:\n%s\n", mLastWebSocketMessage_S.c_str());
    }
    if (mDisClientState_E != mDisClientLastState_E)
    {
      DisClient::S_Log("LEAVE: State change %s->%s: %s Sts %s StsCmd %s\n", S_DisClientStateTypeEnumConverter.ToString(mDisClientLastState_E).c_str(), S_DisClientStateTypeEnumConverter.ToString(mDisClientState_E).c_str(), IsWebSocketConnected() ? "Connected" : "Disconnected", BOF::Bof_ErrorCode(Sts_E), BOF::Bof_ErrorCode(StsCmd_E));
      mDisClientLastState_E = mDisClientState_E;
    }
    if (DeltaPoll_U32 > mDisClientParam_X.PollTimeInMs_U32)
    {
      mPollTimer_U32 = BOF::Bof_GetMsTickCount();
    }
  }
  catch (nlohmann::json::exception &re)
  {
    DisClient::S_Log("V_PreNewFrame: exception caught '%s'\n", re.what());
  }
}

BOFERR DisClient::V_RefreshGui()
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;
  int32_t DisDbgRouteIndex_S32, DisDbgPageIndex_S32, DisDbgSubPageIndex_S32;

  if (!mServiceRouteJsonData.is_null())
  {
    DisplayServiceRoute(0, 0, mIsServiceRouteVisible_B, mServiceRouteJsonData, DisDbgRouteIndex_S32);
    if (DisDbgRouteIndex_S32 != mDisDbgRouteIndex_S32)
    {
      mDisDbgRouteIndex_S32 = DisDbgRouteIndex_S32;
      mDisClientState_E = DIS_CLIENT_STATE::DIS_CLIENT_STATE_OPEN_WS;
      mPollTimer_U32 = BOF::Bof_GetMsTickCount();
    }
  }
  if (!mPageLayoutJsonData.is_null())
  {
    DisplayPageLayout(200, 50, mIsPageLayoutVisible_B, mPageLayoutJsonData, DisDbgPageIndex_S32, DisDbgSubPageIndex_S32);
    if ((DisDbgPageIndex_S32 != mDisDbgPageIndex_S32) || (DisDbgSubPageIndex_S32!=mDisDbgSubPageIndex_S32))
    {
      mDisDbgPageIndex_S32 = DisDbgPageIndex_S32;
      mDisDbgSubPageIndex_S32 = DisDbgSubPageIndex_S32;
      mCtrlFlag_U32 = (DIS_CTRL_FLAG_BACK | DIS_CTRL_FLAG_FORE);
      mPollTimer_U32 = 0;
    }
  }
  if (!mPageInfoBackJsonData.is_null())
  {
    DisplayPageInfo(600, 700, mIsPageInfoVisible_B, true, mPageInfoBackJsonData);

  }
  if (!mPageInfoForeJsonData.is_null())
  {
    DisplayPageInfo(600, 700, mIsPageInfoVisible_B, false, mPageInfoForeJsonData);
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

  if ((_x_U32 < mDisClientParam_X.ConsoleWidth_U32) && (_y_U32 < mDisClientParam_X.ConsoleHeight_U32) && ((_x_U32 + _rText_S.size()) < mDisClientParam_X.ConsoleWidth_U32))
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

bool DisClient::IsWebSocketConnected()
{
  return mWsConnected_B;
}
bool DisClient::IsCommandRunning()
{
  bool Rts_B = false;
  uint32_t Delta_U32;

  if (IsWebSocketConnected())
  {
    if (mLastCmdSentTimeoutInMs_U32)
    {
      Delta_U32 = BOF::Bof_ElapsedMsTime(mLastCmdSentTimer_U32);
      if (Delta_U32 >= mLastCmdSentTimeoutInMs_U32)
      {
        DIS_CLIENT_END_OF_COMMAND(); // Cancel previous one
      }
    }
    if (mLastCmdSentTimeoutInMs_U32)
    {
      Rts_B = true;
    }
  }
  return Rts_B;
}
BOFERR DisClient::SendCommand(uint32_t _TimeoutInMsForReply_U32, const std::string &_rCmd_S)
{
  BOFERR Rts_E = BOF_ERR_ENOTCONN;
  std::string Command_S;

  if (!IsCommandRunning())
  {
    Rts_E = BOF_ERR_EINVAL;
    if (_TimeoutInMsForReply_U32)
    {
      mWaitForReplyId_U32 = S_mSeqId_U32;
      BOF_INC_TICKET_NUMBER(S_mSeqId_U32);

      if (_rCmd_S.find('?') != std::string::npos)
      {
        Command_S = _rCmd_S + "&seq=" + std::to_string(mWaitForReplyId_U32);
      }
      else
      {
        Command_S = _rCmd_S + "?seq=" + std::to_string(mWaitForReplyId_U32);
      }
      mLastCmdSentTimer_U32 = BOF::Bof_GetMsTickCount();
      Rts_E = WriteWebSocket(Command_S.c_str());
      if (Rts_E == BOF_ERR_NO_ERROR)
      {
        mLastCmdSentTimeoutInMs_U32 = _TimeoutInMsForReply_U32;
      }
    }
  }
  return Rts_E;
}
BOFERR DisClient::CheckForCommandReply(std::string &_rReply_S)
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

  if (IsCommandRunning())
  {
    MaxSize_U32 = sizeof(pData_c) - 1;
    Rts_E = ReadWebSocket(MaxSize_U32, pData_c);
    if (Rts_E == BOF_ERR_NO_ERROR)
    {
      Rts_E = BOF_ERR_EMPTY;
      if (MaxSize_U32)
      {
        pData_c[MaxSize_U32] = 0;
        Rts_E = BOF_ERR_EBADF;
        try
        {
          //DisClient::S_Log("===================>b\n%s\n", pData_c);
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
            if (Seq_U32 == mWaitForReplyId_U32)
            {
              _rReply_S = pData_c;
              DIS_CLIENT_END_OF_COMMAND();
              Rts_E = BOF_ERR_NO_ERROR;
            }
          }
        }
        catch (nlohmann::json::exception &re)
        {
          Rts_E = BOF_ERR_INVALID_ANSWER;
          DisClient::S_Log("CheckForCommandReply: exception caught '%s'\n", re.what());
        }
      }
    }
  }
  return Rts_E;
}
BOFERR DisClient::ReadWebSocket(uint32_t &_rMaxSize_U32, char *_pData_c)
{
  BOFERR Rts_E;

  Rts_E = mpuRxBufferCollection->PopBuffer(0, &_rMaxSize_U32, (uint8_t *)_pData_c);
  return Rts_E;
}

#if defined(__EMSCRIPTEN__)

BOFERR DisClient::OpenWebSocket()
{
  BOFERR Rts_E = BOF_ERR_NOT_SUPPORTED;
  // EmscriptenWebSocketCreateAttributes ws_attrs = {"wss://echo.websocket.org", nullptr, EM_TRUE};
  EmscriptenWebSocketCreateAttributes ws_attrs = {"ws://10.129.171.112:8080", nullptr, EM_TRUE};

  if (emscripten_websocket_is_supported())
  {
    CloseWebSocket();
    mWs = emscripten_websocket_new(&ws_attrs);

    DisClient::S_Log("CREATE Ws %d\n", mWs);
    if (mWs > 0)
    {
      emscripten_websocket_set_onopen_callback(mWs, this, WebSocket_OnOpen);
      emscripten_websocket_set_onerror_callback(mWs, this, WebSocket_OnError);
      emscripten_websocket_set_onclose_callback(mWs, this, WebSocket_OnClose);
      emscripten_websocket_set_onmessage_callback(mWs, this, WebSocket_OnMessage);
      Rts_E = BOF_ERR_NO_ERROR;
    }
  }
  return Rts_E;
}
BOFERR DisClient::CloseWebSocket()
{
  BOFERR Rts_E = BOF_ERR_CLOSE;
  EMSCRIPTEN_RESULT Sts;

  mpuRxBufferCollection->Reset();
  Sts = emscripten_websocket_close(mWs, 1000, "CloseWebSocket");
  DisClient::S_Log("STOP Ws %d res %d\n", mWs, Sts);
  if (Sts == 0)
  {
    mWs = -1;
    mWsConnected_B = false;
    Rts_E = BOF_ERR_NO_ERROR;
  }
  return Rts_E;
}
BOFERR DisClient::WriteWebSocket(const char *_pData_c)
{
  BOFERR Rts_E = BOF_ERR_WRITE;
  EMSCRIPTEN_RESULT Sts;
  Sts = emscripten_websocket_send_utf8_text(mWs, _pData_c);
  if (Sts == 0)
  {
    Rts_E = BOF_ERR_NO_ERROR;
  }
  return Rts_E;
}

BOFERR DisClient::OnWebSocketOpenEvent(const EmscriptenWebSocketOpenEvent *_pWebsocketEvent_X)
{
  BOFERR Rts_E = BOF_ERR_EINVAL;

  if (_pWebsocketEvent_X)
  {
    mWsConnected_B = true;
    Rts_E = BOF_ERR_NO_ERROR;
  }
  return Rts_E;
}
BOFERR DisClient::OnWebSocketErrorEvent(const EmscriptenWebSocketErrorEvent *_pWebsocketEvent_X)
{
  BOFERR Rts_E = BOF_ERR_EINVAL;

  if (_pWebsocketEvent_X)
  {
    Rts_E = CloseWebSocket();
    Rts_E = BOF_ERR_NO_ERROR;
  }
  return Rts_E;
}
BOFERR DisClient::OnWebSocketCloseEvent(const EmscriptenWebSocketCloseEvent *_pWebsocketEvent_X)
{
  BOFERR Rts_E = BOF_ERR_EINVAL;

  if (_pWebsocketEvent_X)
  {
    mWsConnected_B = false;
    mWs = -1;
    DIS_CLIENT_END_OF_COMMAND();
    Rts_E = BOF_ERR_NO_ERROR;
  }
  return Rts_E;
}
BOFERR DisClient::OnWebSocketMessageEvent(const EmscriptenWebSocketMessageEvent *_pWebsocketEvent_X)
{
  BOFERR Rts_E = BOF_ERR_EINVAL;
  EMSCRIPTEN_RESULT Sts;
  BOF::BOF_RAW_BUFFER *pRawBuffer_X;
  bool FinalFragment_B;

  if (_pWebsocketEvent_X)
  {
    if (_pWebsocketEvent_X->isText)
    {
      // For only ascii chars.
      // DisClient::S_Log("----------------->TXT message from %d: %d:%s\n", _pWebsocketEvent_X->socket, _pWebsocketEvent_X->numBytes, _pWebsocketEvent_X->data);
    }
    mpuRxBufferCollection->SetAppendMode(0, true, nullptr); // By default we append
    FinalFragment_B = true;
    Rts_E = mpuRxBufferCollection->PushBuffer(0, _pWebsocketEvent_X->numBytes, (const uint8_t *)_pWebsocketEvent_X->data, &pRawBuffer_X);
    mpuRxBufferCollection->SetAppendMode(0, !FinalFragment_B, &pRawBuffer_X); // If it was the final one, we stop append->Result is pushed and committed
    // DisClient::S_Log("err %d data %d:%p Rts:indx %d slot %x 1: %x:%p 2: %x:%p\n", Rts_E, _pWebsocketEvent_X->numBytes, (const uint8_t *)_pWebsocketEvent_X->data, pRawBuffer_X->IndexInBuffer_U32, pRawBuffer_X->SlotEmpySpace_U32, pRawBuffer_X->Size1_U32, pRawBuffer_X->pData1_U8, pRawBuffer_X->Size2_U32, pRawBuffer_X->pData2_U8);
    if (Rts_E != BOF_ERR_NO_ERROR)
    {
      CloseWebSocket();
    }
  }
  return Rts_E;
}
#else
BOFERR DisClient::OpenWebSocket()
{
  BOFERR Rts_E = BOF_ERR_INIT;
  if (mpuDisClientWebSocket)
  {
    Rts_E = mpuDisClientWebSocket->ResetWebSocketOperation();
    Rts_E = mpuDisClientWebSocket->Connect(DIS_CLIENT_WEB_TIMEOUT, mDisClientParam_X.DisServerEndpoint_S, "DisClientWs");
  }
  return Rts_E;
}
BOFERR DisClient::CloseWebSocket()
{
  BOFERR Rts_E = BOF_ERR_INIT;
  if (mpuDisClientWebSocket)
  {
    Rts_E = mpuDisClientWebSocket->Disconnect(DIS_CLIENT_WEB_TIMEOUT);
  }
  return Rts_E;
}
BOFERR DisClient::WriteWebSocket(const char *_pData_c)
{
  BOFERR Rts_E = BOF_ERR_INIT;
  uint32_t Nb_U32;

  if (mpuDisClientWebSocket)
  {
    Rts_E = BOF_ERR_EINVAL;
    if (_pData_c)
    {
      Nb_U32 = strlen(_pData_c);
      Rts_E = mpuDisClientWebSocket->Write(DIS_CLIENT_WEB_TIMEOUT, Nb_U32, (uint8_t *)_pData_c);
    }
  }
  return Rts_E;
}
BOFERR DisClient::OnWebSocketOpenEvent(void *_pId)
{
  BOFERR Rts_E = BOF_ERR_EINVAL;

  mWsConnected_B = true;
  Rts_E = BOF_ERR_NO_ERROR;

  return Rts_E;
}
BOFERR DisClient::OnWebSocketErrorEvent(void *_pId, BOFERR _Sts_E)
{
  BOFERR Rts_E = BOF_ERR_EINVAL;

  Rts_E = CloseWebSocket();
  mWs = reinterpret_cast<uintptr_t>(_pId);
  Rts_E = BOF_ERR_NO_ERROR;
  return Rts_E;
}
BOFERR DisClient::OnWebSocketCloseEvent(void *_pId)
{
  BOFERR Rts_E = BOF_ERR_EINVAL;

  mWsConnected_B = false;
  mWs = -1;
  DIS_CLIENT_END_OF_COMMAND();
  Rts_E = BOF_ERR_NO_ERROR;

  return Rts_E;
}
BOFERR DisClient::OnWebSocketMessageEvent(void *_pId, uint32_t _Nb_U32, uint8_t *_pData_U8, BOF::BOF_BUFFER &_rReply_X)
{
  BOFERR Rts_E = BOF_ERR_EINVAL;
  BOF::BOF_RAW_BUFFER *pRawBuffer_X;
  bool FinalFragment_B;

  if (_pData_U8)
  {
    mpuRxBufferCollection->SetAppendMode(0, true, nullptr); // By default we append
    FinalFragment_B = true;
    Rts_E = mpuRxBufferCollection->PushBuffer(0, _Nb_U32, _pData_U8, &pRawBuffer_X);
    mpuRxBufferCollection->SetAppendMode(0, !FinalFragment_B, &pRawBuffer_X); // If it was the final one, we stop append->Result is pushed and committed
    // DisClient::S_Log("err %d data %d:%p Rts:indx %d slot %x 1: %x:%p 2: %x:%p\n", Rts_E, _Nb_U32, _pData_U8, pRawBuffer_X->IndexInBuffer_U32, pRawBuffer_X->SlotEmpySpace_U32, pRawBuffer_X->Size1_U32, pRawBuffer_X->pData1_U8, pRawBuffer_X->Size2_U32, pRawBuffer_X->pData2_U8);
    if (Rts_E != BOF_ERR_NO_ERROR)
    {
      CloseWebSocket();
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
