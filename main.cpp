#include "bof_imgui.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdio.h>

#if defined(_WIN32)
#include <Windows.h>
#endif
// #define WITH_TREAD
#if defined(WITH_TREAD)
#include <pthread.h>
// #include <thread>
#endif

#if defined(__EMSCRIPTEN__)
#pragma message("__EMSCRIPTEN__")
//"wss://echo.websocket.org",
#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>

EM_BOOL WebSocket_OnOpen(int _Event_i, const EmscriptenWebSocketOpenEvent *_pWebsocketEvent_X, void *_pUserData);
EM_BOOL WebSocket_OnError(int _Event_i, const EmscriptenWebSocketErrorEvent *_pWebsocketEvent_X, void *_pUserData);
EM_BOOL WebSocket_OnClose(int _Event_i, const EmscriptenWebSocketCloseEvent *_pWebsocketEvent_X, void *_pUserData);
EM_BOOL WebSocket_OnMessage(int _Event_i, const EmscriptenWebSocketMessageEvent *_pWebsocketEvent_X, void *_pUserData);
#endif

constexpr const char *APP_NAME = "DisClient";
#if defined(WITH_TREAD)
void *thread_function(void *arg)
{
  int thread_id = *((int *)arg);
  int i;
  for (i = 0; i < 8; i++)
  {
    printf("Hello from thread %d i=%d\n", thread_id, i);
  }
  return nullptr;
}
#endif
class DisClient : public BOF::Bof_ImGui
{
public:
  DisClient(const BOF::BOF_IMGUI_PARAM &_rImguiParam_X)
      : BOF::Bof_ImGui(_rImguiParam_X)
  {
#if defined(__EMSCRIPTEN__)
    OpenWebSocket();
#endif
  }
  ~DisClient()
  {
  }
  static void S_Log(const char *_pFormat_c, ...)
  {
    char pDbg_c[0x400];
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
  nlohmann::json ToJson() const
  {
    nlohmann::json Rts;
    // Rts["Stack"] = Stack.to_json();
    // Rts["Input"] = Input;
    // Rts["ErrorMessage"] = ErrorMessage;
    // Rts["InverseMode"] = InverseMode;
    // Rts["AngleUnit"] = AngleUnit;
    // Rts["StoredValue"] = StoredValue;

    return Rts;
  }
  // Deserialization
  void FromJson(const nlohmann::json &_rJson)
  {
    // char p[128];
    //    DBG_LOG("ption caught\n");
    //    DBG_LOG("FromJson: exception caught '%s'\n", " p");

    try
    {
      // Stack.from_json(_rJson["Stack"]);
      // Input = _rJson["Input"].get<std::string>();
      // ErrorMessage = _rJson["ErrorMessage"].get<std::string>();
      // InverseMode = _rJson["InverseMode"].get<bool>();
      // AngleUnit = _rJson["AngleUnit"].get<AngleUnitType>();
      // if (_rJson.contains("StoredValue"))
      //   StoredValue = _rJson["StoredValue"].get<double>();
    }
    catch (nlohmann::json::type_error &re)
    {
      DisClient::S_Log("FromJson: exception caught '%s'\n", re.what());
    }
  }

  BOFERR V_SaveSettings() override
  {
    BOFERR Rts_E = BOF_ERR_NO_ERROR;

    auto Json = ToJson();
    std::string StateSerialized_S = Json.dump();
    HelloImGui::SaveUserPref(APP_NAME, StateSerialized_S);
    return Rts_E;
  }

  BOFERR V_ReadSettings() override
  {
    BOFERR Rts_E = BOF_ERR_NO_ERROR;

    std::string StateSerialized_S = HelloImGui::LoadUserPref(APP_NAME);
    if (!StateSerialized_S.empty())
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
    return Rts_E;
  }
  BOFERR LoadNavigation(std::string &_rNavJson_S, nlohmann::json &_rJsonData)
  {
    BOFERR Rts_E = BOF_ERR_NOT_FOUND;
    FILE *pIo_X;
    std::string NavJsonFilename_S = "data/DebugPageLayout.json";

    if (HelloImGui::AssetExists(NavJsonFilename_S))
    {
      Rts_E = BOF_ERR_NO_ERROR;
      pIo_X = fopen(NavJsonFilename_S.c_str(), "r");
      if (pIo_X)
      {
        try
        {
          _rJsonData = nlohmann::json::parse(pIo_X);
        }
        catch (const std::exception &e)
        {
          printf("Error parsing JSON: %s\n", e.what());
        }
        fclose(pIo_X);
      }
    }
    return Rts_E;
  }
  void DrawNavigationTree(int32_t _x_U32, int32_t _y_U32, bool &_rIsNavigationVisible_B, const nlohmann::json &_rNavigationJsonData)
  {
    ImGuiTreeNodeFlags NodeFlag = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanAllColumns;
    ImGuiTreeNodeFlags LeafFlag = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanAllColumns;
    ImGuiTreeNodeFlags ItemFlag;
    uint32_t i_U32, j_U32, LeafIndex_U32, NavigationLeafIdClicked_U32;
    bool IsNodeOpen_B;

    ImGui::Begin("##Navigation", &_rIsNavigationVisible_B, ImGuiWindowFlags_NoTitleBar); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    ImGui::SetWindowPos(ImVec2(_x_U32, _y_U32), ImGuiCond_FirstUseEver);
    // ImGuiWindow *window = ImGui::FindWindowByName("##Navigation");
    // ImGuiWindow *window1 = ImGui::GetCurrentWindow();
    // printf("FindWindowByName %p/%p\n", window, window1);
    // printf("-2->%d\n", window->IDStack.Size);
    if (ImGui::TreeNodeEx(_rNavigationJsonData.value("label", "Unknown").c_str(), NodeFlag))
    {
      S_BuildHelpMarker("This is the list of the different debug screen that are availablke from your current debug target.\n"
                        "Click to select, CTRL+Click to toggle, click on arrows or double-click to open.");
      // 'selection_mask' is dumb representation of what may be user-side selection state.
      //  You may retain selection state inside or outside your objects in whatever format you see fit.
      // 'NavigationLeafIdClicked_U32' is temporary storage of what node we have clicked to process selection at the end
      /// of the loop. May be a pointer to your own node type, etc.

      // Check if "page" exists and is an array
      auto PageIterator = _rNavigationJsonData.find("page");
      if (PageIterator != _rNavigationJsonData.end() && PageIterator->is_array())
      {
        const auto &PageCollection = *PageIterator;
        LeafIndex_U32 = 0;
        NavigationLeafIdClicked_U32 = -1;

        for (i_U32 = 0; i_U32 < PageCollection.size(); i_U32++)
        {
          const auto &CrtPage = PageCollection[i_U32];
          IsNodeOpen_B = ImGui::TreeNodeEx((void *)(intptr_t)i_U32, NodeFlag, "%s", CrtPage.value("label", "Page_NotFound_" + std::to_string(i_U32)).c_str());
          if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
          {
            // Not selectable NavigationLeafIdClicked_U32 = i_U32;
          }
          auto subPageIterator = CrtPage.find("subPage");
          // printf("Value returned by page.find(\"subPage\"): %s\n", subPageIterator != page.end() ? subPageIterator->dump(4).c_str() : "Not found");
          if ((subPageIterator != CrtPage.end()) && (subPageIterator->is_array()))
          {
            const auto &SubPageCollection = *subPageIterator;
            if (IsNodeOpen_B)
            {
              for (j_U32 = 0; j_U32 < SubPageCollection.size(); j_U32++)
              {
                const auto &CrtSubPage = SubPageCollection[j_U32];
                // Disable the default "open on single-click behavior" + set Selected flag according to our selection.
                // To alter selection we use IsItemClicked() && !IsItemToggledOpen(), so clicking on an arrow doesn't alter selection.
                ItemFlag = LeafFlag;
                if (ImGui::IsItemHovered())
                {
                  //;VcRun:Web:dis-client
                  ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                }
                if (mNavigationLeafIdSelected_U32 == LeafIndex_U32)
                {
                  ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
                  // ItemFlag |= ImGuiTreeNodeFlags_Selected;
                  //  printf("Id %d/%d i %d j %d:'%s' selected\n", mNavigationLeafIdSelected_U32, LeafIndex_U32, i_U32, j_U32, CrtSubPage.value("label", "SubPage_NotFound_" + std::to_string(j_U32)).c_str());
                  //  ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(0, 64, 0, 255));
                  //  ImGui::SameLine();
                }
                ImGui::TreeNodeEx((void *)(intptr_t)j_U32, ItemFlag, "%s", CrtSubPage.value("label", "SubPage_NotFound_" + std::to_string(j_U32)).c_str()); // Leaf so no ImGui::TreePop();
                if (mNavigationLeafIdSelected_U32 == LeafIndex_U32)
                {
                  ImGui::PopStyleColor();
                }
                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
                {
                  NavigationLeafIdClicked_U32 = LeafIndex_U32;
                  printf("click Id %d/%d i %d j %d:'%s' selected\n", mNavigationLeafIdSelected_U32, LeafIndex_U32, i_U32, j_U32, CrtSubPage.value("label", "SubPage_NotFound_" + std::to_string(j_U32)).c_str());
                }

                LeafIndex_U32++;
              }
              ImGui::TreePop();
            }
            else
            {
              LeafIndex_U32 += SubPageCollection.size();
            }
          } // if (IsNodeOpen_B)
          LeafIndex_U32++;
        } // for (i = 0; i < pages.size(); ++i)
        if (NavigationLeafIdClicked_U32 != -1)
        {
          printf("ctrl %d id %d/%d\n", ImGui::GetIO().KeyCtrl, mNavigationLeafIdSelected_U32, NavigationLeafIdClicked_U32);

          // Update selection state
          // (process outside of tree loop to avoid visual inconsistencies during the clicking frame)
          if ((ImGui::GetIO().KeyCtrl) && (mNavigationLeafIdSelected_U32 == NavigationLeafIdClicked_U32))
          {
            mNavigationLeafIdSelected_U32 = -1;
          }
          else // Depending on selection behavior you want, may want to preserve selection when clicking on item that is part of the selection
          {
            mNavigationLeafIdSelected_U32 = NavigationLeafIdClicked_U32;
            // selection_mask = (1 6<< mNavigationLeafIdSelected_U32); // Click to single-select
          }
          printf("Final is %d\n", mNavigationLeafIdSelected_U32);
        }
      } // if (pageIterator != _rNavigationJsonData.end() && pageIterator->is_array())
      ImGui::TreePop();
    } // if (ImGui::TreeNodeEx(_rNavigationJsonData.value("label", "Unknown").c_str(), NodeFlag))
    ImGui::End();
  }

  BOFERR LoadPageInfo(std::string &_rPgInfoJson_S, nlohmann::json &_rJsonData)
  {
    BOFERR Rts_E = BOF_ERR_NOT_FOUND;
    FILE *pIo_X;
    std::string PgInfoJsonFilename_S = "data/DebugPageInfo.json";
    if (HelloImGui::AssetExists(PgInfoJsonFilename_S))
    {
      Rts_E = BOF_ERR_NO_ERROR;
      FILE *pIo_X = fopen(PgInfoJsonFilename_S.c_str(), "r");
      if (pIo_X)
      {
        try
        {
          _rJsonData = nlohmann::json::parse(pIo_X);
        }
        catch (const std::exception &e)
        {
          printf("Error parsing JSON: %s\n", e.what());
        }
        fclose(pIo_X);
      }
    }
    return Rts_E;
  }

  void DisplayPageInfo(int32_t _x_U32, int32_t _y_U32, bool &_rIsPgInfoVisible_B, const nlohmann::json &_rPgInfoJsonData)
  {
    BOFERR Sts_E;
    int x, y;
    uint32_t Len_U32;
    std::string texte, bg_color_str, fg_color_str;
    ImU32 couleurFond, couleurTexte;
    BOF::Bof_ImGui_ImTextCustomization TextCustomization_X;
    BOF::BOF_SIZE<uint32_t> TextSize_X;
    char pText_c[128];

    ImGui::PushFont(GetFont(0));
    TextSize_X = GetTextSize("A");
    // float TitleBarHeight_f = ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().FramePadding.y * 2.0f;
    //  Now titleBarHeight contains an approximation of the window title bar height

    SetNextWindowSize(TextSize_X.Width * 80, TextSize_X.Height * 25);

    bg_color_str = _rPgInfoJsonData["background"]["bg"];
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(std::stoi(bg_color_str.substr(1, 2), nullptr, 16), std::stoi(bg_color_str.substr(3, 2), nullptr, 16), std::stoi(bg_color_str.substr(5, 2), nullptr, 16), 255));
    ImGui::Begin("##Console", &_rIsPgInfoVisible_B, ImGuiWindowFlags_NoTitleBar); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    ImGui::SetWindowPos(ImVec2(_x_U32, _y_U32), ImGuiCond_FirstUseEver);

    auto lines = _rPgInfoJsonData["background"]["line"];
    for (const auto &line : lines)
    {
      x = line["x"];
      y = line["y"];
      texte = line["text"];
      bg_color_str = line["bg"];
      fg_color_str = line["fg"];

      couleurFond = IM_COL32(std::stoi(bg_color_str.substr(1, 2), nullptr, 16), std::stoi(bg_color_str.substr(3, 2), nullptr, 16), std::stoi(bg_color_str.substr(5, 2), nullptr, 16), 255);
      couleurTexte = IM_COL32(std::stoi(fg_color_str.substr(1, 2), nullptr, 16), std::stoi(fg_color_str.substr(3, 2), nullptr, 16), std::stoi(fg_color_str.substr(5, 2), nullptr, 16), 255);
      Len_U32 = sprintf(pText_c, "v %s line at %d,%d,%s", IMGUI_VERSION, x, y, texte.c_str());
      Sts_E = DisplayText(nullptr, pText_c, true, false, &TextCustomization_X.Clear().Range(pText_c).TextColor_X(couleurTexte).Highlight(couleurFond));
    }

    std::string KeyState_S = S_GetKeyboardState();
    static std::string S_LastKeyState_S;
    if (KeyState_S != "")
    {
      S_LastKeyState_S = KeyState_S;
    }
    ImGui::TextColored(ImVec4(1.0, 0, 0, 1.0), "%s", S_LastKeyState_S.c_str());

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopFont();
  }

  BOFERR V_RefreshGui() override
  {
    BOFERR Rts_E = BOF_ERR_NO_ERROR, Sts_E;
    bool BoolVar_B;
    ImVec2 windowPos;
#if 0
    uint32_t i_U32, Len_U32;
    char pText_c[128];
    BOF::BOF_SIZE<uint32_t> TextSize_X;
    ImColor pCol_X[] = {
        ImColor(255, 0, 0, 255),   // Red
        ImColor(0, 255, 0, 255),   // Green
        ImColor(0, 0, 255, 255),   // Blue
        ImColor(255, 255, 0, 255), // Yellow
    };
    BOF::Bof_ImGui_ImTextCustomization tc;

    BoolVar_B = true;
    ImGui::PushFont(GetFont(0));
    TextSize_X = GetTextSize("A");

    SetNextWindowSize(TextSize_X.Width * 80, TextSize_X.Height * 25);
    ImGui::Begin("##TextConsole", &BoolVar_B, ImGuiWindowFlags_NoTitleBar); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    windowPos = ImVec2(20.0f, 500.0f);
    ImGui::SetWindowPos(windowPos, ImGuiCond_FirstUseEver);

    for (i_U32 = 0; i_U32 < 16; i_U32++)
    {
      SetCursorPos(TextSize_X.Width * i_U32, TextSize_X.Height * i_U32);
      Len_U32 = sprintf(pText_c, "Hello i equal %d", i_U32);
      DisplayText(nullptr, pText_c, true, false, &tc.Clear().Range(pText_c).Highlight(pCol_X[i_U32 % 4]));
    }
    // Restore the default font
    ImGui::PopFont();

    ImGui::End();
#endif

#if defined(__EMSCRIPTEN__)
    ImGui::Begin("##Control", &BoolVar_B, ImGuiWindowFlags_NoTitleBar); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    windowPos = ImVec2(20.0f, 300.0f);
    ImGui::SetWindowPos(windowPos, ImGuiCond_FirstUseEver);

    // EM_ASM(console.log("Hello from C program"););
    if (ImGui::Button("Open WebSocket"))
    {
      Sts_E = OpenWebSocket();
    }
    if (ImGui::Button("Send WebSocket Msg"))
    {
      std::string NavJson_S, PgInfoJson_S;
      EMSCRIPTEN_RESULT Sts;
      Sts_E = LoadNavigation(NavJson_S, mNavigationJsonData);
      // Sts_E = BOF_ERR_NO;
      if (Sts_E == BOF_ERR_NO_ERROR)
      {
        Sts_E = LoadPageInfo(PgInfoJson_S, mPageInfoJsonData);
        // Sts_E = BOF_ERR_NO;
        if (Sts_E == BOF_ERR_NO_ERROR)
        {
          /*
                  static int S_Cpt_i = 0;
                  char pData_c[128];
                  sprintf(pData_c, "Msg %d from ws\n", S_Cpt_i++);
                  Sts_E = WriteWebSocket((const char *)pData_c);
                  // JsonData.value("title", "Unknown")

                  //Sts_E = WriteWebSocket(NavJson_S.c_str());
                  */
        }
      }
    }
    if (ImGui::Button("Close WebSocket"))
    {
      Sts_E = CloseWebSocket();
    }

#if defined(WITH_TREAD)
    if (ImGui::Button("Test thread"))
    {
      // std::thread t1(thread_function, 0);
      // t1.join();
#if 1
      const int num_threads = 3;
      pthread_t threads[num_threads];
      int i, sts, thread_ids[num_threads];

      // Create threads
      for (i = 0; i < num_threads; ++i)
      {
        thread_ids[i] = i;
        sts = pthread_create(&threads[i], nullptr, thread_function, &thread_ids[i]);
        printf("thread%d hndl %x created sts %d e %d\n", i, (int)threads[i], sts, errno);
      }
      // emscripten_sleep(1000);
      //  Wait for threads to finish
      for (i = 0; i < num_threads; ++i)
      {
        printf("join thread%d hndl %x\n", i, (int)threads[i]);
        sts = pthread_join(threads[i], nullptr);
        printf("join status %d e %d\n", sts, errno);
      }
#endif
    }
#endif

    ImGui::Text("Ws is now %d\n", mWs);
    ImGui::Text("Last msg %zd:%s\n", mLastWebSocketMessage_S.size(), mLastWebSocketMessage_S.c_str());
    ImGui::End();

    if (!mNavigationJsonData.is_null())
    {
      DrawNavigationTree(20, 500, mIsNavigationVisible_B, mNavigationJsonData);
    }
    if (!mPageInfoJsonData.is_null())
    {
      DisplayPageInfo(100, 700, mIsPageInfoVisible_B, mPageInfoJsonData);
    }
#endif

    return Rts_E;
  }

#if defined(__EMSCRIPTEN__)
  BOFERR OpenWebSocket()
  {
    BOFERR Rts_E = BOF_ERR_NOT_SUPPORTED;
    EmscriptenWebSocketCreateAttributes ws_attrs = {"wss://echo.websocket.org", nullptr, EM_TRUE};

    if (emscripten_websocket_is_supported())
    {
      CloseWebSocket();
      mWs = emscripten_websocket_new(&ws_attrs);
      printf("CREATE Ws %d\n", mWs);
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
  BOFERR CloseWebSocket()
  {
    BOFERR Rts_E = BOF_ERR_CLOSE;
    EMSCRIPTEN_RESULT Sts;
    Sts = emscripten_websocket_close(mWs, 1000, "CloseWebSocket");
    printf("STOP Ws %d res %d\n", mWs, Sts);
    if (Sts == 0)
    {
      mWs = -1;
      Rts_E = BOF_ERR_NO_ERROR;
    }
    return Rts_E;
  }
  BOFERR WriteWebSocket(const char *_pData_c)
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
  BOFERR OnWebSocketOpenEvent(const EmscriptenWebSocketOpenEvent *_pWebsocketEvent_X)
  {
    BOFERR Rts_E = BOF_ERR_EINVAL;

    if (_pWebsocketEvent_X)
    {
      Rts_E = BOF_ERR_NO_ERROR;
    }
    return Rts_E;
  }
  BOFERR OnWebSocketErrorEvent(const EmscriptenWebSocketErrorEvent *_pWebsocketEvent_X)
  {
    BOFERR Rts_E = BOF_ERR_EINVAL;

    if (_pWebsocketEvent_X)
    {
      Rts_E = BOF_ERR_NO_ERROR;
    }
    return Rts_E;
  }
  BOFERR OnWebSocketCloseEvent(const EmscriptenWebSocketCloseEvent *_pWebsocketEvent_X)
  {
    BOFERR Rts_E = BOF_ERR_EINVAL;

    if (_pWebsocketEvent_X)
    {
      Rts_E = BOF_ERR_NO_ERROR;
    }
    return Rts_E;
  }
  BOFERR OnWebSocketMessageEvent(const EmscriptenWebSocketMessageEvent *_pWebsocketEvent_X)
  {
    BOFERR Rts_E = BOF_ERR_EINVAL;
    EMSCRIPTEN_RESULT Sts;

    if (_pWebsocketEvent_X)
    {
      if (_pWebsocketEvent_X->isText)
      {
        // For only ascii chars.
        printf("----------------->TXT message from %d: %d:%s\n", _pWebsocketEvent_X->socket, _pWebsocketEvent_X->numBytes, _pWebsocketEvent_X->data);
      }
      else
      {
        printf("----------------->BIN message from %d: %d:%p\n", _pWebsocketEvent_X->socket, _pWebsocketEvent_X->numBytes, _pWebsocketEvent_X->data);
      }
      mLastWebSocketMessage_S = std::string((const char *)_pWebsocketEvent_X->data, _pWebsocketEvent_X->numBytes);
      Rts_E = BOF_ERR_NO_ERROR;
    }
    return Rts_E;
  }
#endif

private:
  bool mVisible_B = true;
  ImFont *mpButtonFont_X = nullptr;
  ImFont *mpLDCFont_X = nullptr;
  ImFont *mpSmallFont_X = nullptr;
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

int main(int _Argc_i, char *_pArgv[])
{
  int Rts_i;

  BOF::BOF_IMGUI_PARAM ImguiParam_X;
  ImguiParam_X.AppName_S = APP_NAME;
  ImguiParam_X.Size_X = BOF::BOF_SIZE<uint32_t>(800, 600);
  ImguiParam_X.Log = DisClient::S_Log;
  ImguiParam_X.ShowDemoWindow_B = true;
  std::unique_ptr<DisClient> puDisClient = std::make_unique<DisClient>(ImguiParam_X);
  Rts_i = (int)puDisClient->MainLoop();
  return Rts_i;
}