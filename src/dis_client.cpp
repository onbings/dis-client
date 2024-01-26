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

#if defined(_WIN32)
#include <Windows.h> //OutputDebugString
#endif

#define IMGUI_WINDOW_TITLEBAR_HEIGHT 20
#define IS_DIS_SERVICE_VALID() ((mDisServiceIndex_S32 >= 0) && (mDisServiceIndex_S32 < mDisClientDbgServiceCollection.size()))
#define IS_PAGE_SUBPAGE_LAYOUT_VALID(DisClientDbgService) ((mDisServiceIndex_S32 >= 0) && (mDisServiceIndex_S32 < mDisClientDbgServiceCollection.size()) && ((DisClientDbgService).PageIndex_S32 >= 0) && ((DisClientDbgService).PageIndex_S32 < (DisClientDbgService).puDisService->GetDbgDisService().DisDbgServiceItemCollection[mDisServiceIndex_S32].DisDbgPageLayoutCollection.size()) && ((DisClientDbgService).SubPageIndex_S32 >= 0) && ((DisClientDbgService).SubPageIndex_S32 < (DisClientDbgService).puDisService->GetDbgDisService().DisDbgServiceItemCollection[mDisServiceIndex_S32].DisDbgPageLayoutCollection[(DisClientDbgService).SubPageIndex_S32].DisDbgSubPageCollection.size()))

DisClient::DisClient(const DIS_CLIENT_PARAM &_rDisClientParam_X)
    : BOF::Bof_ImGui(_rDisClientParam_X.ImguiParam_X), BOF::BofThread()
{
  DIS_DISCOVERY_PARAM DisDiscoveryParam_X;

  mDisClientParam_X = _rDisClientParam_X;

  DisDiscoveryParam_X.PollTimeInMs_U32 = 2000;
  mpuDisDiscovery = std::make_unique<DisDiscovery>(DisDiscoveryParam_X);
}

DisClient::~DisClient()
{
  Stop();
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

    Rts["ImguiParam.Size.Width"] = mDisClientParam_X.ImguiParam_X.Size_X.Width;
    Rts["ImguiParam.Size.Height"] = mDisClientParam_X.ImguiParam_X.Size_X.Height;
    // Rts["ImguiParam.TheLogger"] = mDisClientParam_X.ImguiParam_X.TheLogger;
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
    mDisClientParam_X.PollTimeInMs_U32 = _rJson.value("PollTimeInMs", 1000);
    mDisClientParam_X.FontSize_U32 = _rJson.value("FontSize", 14);
    mDisClientParam_X.ConsoleWidth_U32 = _rJson.value("ConsoleWidth", 80);
    mDisClientParam_X.ConsoleHeight_U32 = _rJson.value("ConsoleHeight", 25);

    mDisClientParam_X.ImguiParam_X.Size_X.Width = _rJson.value("ImguiParam.Size.Width", 800);
    mDisClientParam_X.ImguiParam_X.Size_X.Height = _rJson.value("ImguiParam.Size.Height", 600);
    // mDisClientParam_X.ImguiParam_X.TheLogger = _rJson.value("ImguiParam.TheLogger"].get<uint32_t>();
    mDisClientParam_X.ImguiParam_X.ShowDemoWindow_B = _rJson.value("ImguiParam.ShowDemoWindow", false);
  }
  catch (nlohmann::json::exception &re)
  {
    DisClient::S_Log("FromJson: exception caught '%s'\n", re.what());
  }
}

BOFERR DisClient::V_SaveSettings()
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;
  nlohmann::json Json = ToJson();
  std::string StateSerialized_S = Json.dump();

  HelloImGui::SaveUserPref(mDisClientParam_X.ImguiParam_X.WindowTitle_S, StateSerialized_S);
  return Rts_E;
}

BOFERR DisClient::V_ReadSettings()
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;
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
  return Rts_E;
}
void DisClient::V_LoadAdditionalFonts()
{
  std::string FontFilename_S = "./assets/fonts/cour.ttf";
  mpConsoleFont_X = LoadFont(FontFilename_S.c_str(), 16);
  if (mpConsoleFont_X)
  {
  }
  DisClient::S_Log("Try to load font %s ->ptr %p\n", FontFilename_S.c_str(), mpConsoleFont_X);
}

/*
  Xt1_DisService
  |
  + 10.129.171.112
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
    + P6X
      |
      + Cpu
        |
        + Temp
        + Task
        + Snmp
*/

void DisClient::DisplayDisService(int32_t _x_U32, int32_t _y_U32, DIS_CLIENT_DBG_SERVICE &_rDisClientDbgService_X, int32_t &_rDisServiceIndex_S32, int32_t &_rPageIndex_S32, int32_t &_rSubPageIndex_S32)
{
  ImGuiTreeNodeFlags NodeFlag = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanAllColumns;
  ImGuiTreeNodeFlags LeafFlag = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanAllColumns;
  ImGuiTreeNodeFlags ItemFlag;
  uint32_t PageIndex_U32, SubPageIndex_U32;
  int32_t PageLayoutPageIndexClicked_S32, SubPageLayoutPageIndexClicked_S32;
  bool IsNodeOpen_B;

  PageLayoutPageIndexClicked_S32 = DIS_CLIENT_INVALID_INDEX;
  SubPageLayoutPageIndexClicked_S32 = DIS_CLIENT_INVALID_INDEX;

  ImGui::Begin("##Dis Service", &_rDisClientDbgService_X.IsVisisble_B, ImGuiWindowFlags_None); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
  ImGui::SetWindowPos(ImVec2(_x_U32, _y_U32), ImGuiCond_FirstUseEver);
  IsNodeOpen_B = ImGui::TreeNodeEx("##Dis Service", NodeFlag);
  if (IsNodeOpen_B)
  {
    S_BuildHelpMarker("This is the list of the different target to which you can connect.\n"
                      "Click to select, CTRL+Click to toggle, click on arrows or double-click to open.");
    // for (DIS_DBG_SERVICE_ITEM &rDisDbgServiceItem : _rDisClientDbgService_X.puDisService->GetDbgDisService().DisDbgServiceItemCollection)
    for (const DIS_DBG_SERVICE_ITEM &rDisDbgServiceItem : _rDisClientDbgService_X.puDisService->GetDbgDisService().DisDbgServiceItemCollection)
    {
      IsNodeOpen_B = ImGui::TreeNodeEx("##rDisDbgServiceItem", NodeFlag, "%s", rDisDbgServiceItem.Name_S.c_str());
      if (IsNodeOpen_B)
      {
        PageIndex_U32 = 0;
        // Disable the default "open on single-click behavior" + set Selected flag according to our selection.
        // To alter selection we use IsItemClicked() && !IsItemToggledOpen(), so clicking on an arrow doesn't alter selection.
        ItemFlag = LeafFlag;
        ImGui::TreeNodeEx("##rDisDbgServiceItemData", ItemFlag, "%s", rDisDbgServiceItem.IpAddress_S.c_str());
        for (const DIS_DBG_PAGE_LAYOUT &rDisDbgPageLayout : rDisDbgServiceItem.DisDbgPageLayoutCollection)
        {
          SubPageIndex_U32 = 0;
          IsNodeOpen_B = ImGui::TreeNodeEx("##rDisDbgPageLayout", NodeFlag, "%s", rDisDbgPageLayout.Label_S.c_str());
          for (const DIS_DBG_SUB_PAGE_LAYOUT &rDisDbgSubPageLayout : rDisDbgPageLayout.DisDbgSubPageCollection)
          {
            if (IsNodeOpen_B)
            {
              // Disable the default "open on single-click behavior" + set Selected flag according to our selection.
              // To alter selection we use IsItemClicked() && !IsItemToggledOpen(), so clicking on an arrow doesn't alter selection.
              ItemFlag = LeafFlag;
              if (ImGui::IsItemHovered())
              {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
              }
              if ((_rPageIndex_S32 == PageIndex_U32) && (_rSubPageIndex_S32 == SubPageIndex_U32))
              {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
              }
              ImGui::TreeNodeEx("##rDisDbgPageLayoutData", ItemFlag, "%s", rDisDbgSubPageLayout.Label_S);
              if ((_rPageIndex_S32 == PageIndex_U32) && (_rSubPageIndex_S32 == SubPageIndex_U32))
              {
                ImGui::PopStyleColor();
              }
              if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
              {
                PageLayoutPageIndexClicked_S32 = PageIndex_U32;
                SubPageLayoutPageIndexClicked_S32 = SubPageIndex_U32;
              }
            }
            // ImGui::TreePop(); Leaf
            SubPageIndex_U32++;
          } // for (DIS_DBG_SUB_PAGE_LAYOUT &rDisDbgSubPageLayout : rDisDbgPageLayout.DisDbgSubPageCollection)
          ImGui::TreePop();
          PageIndex_U32++;
        } // for (DIS_DBG_PAGE_LAYOUT &rDisDbgPageLayout : rDisDbgServiceItem.DisDbgPageLayoutCollection)
        ImGui::TreePop();
      } // if (IsNodeOpen_B)
      ImGui::TreePop();
    } // for (DIS_DBG_SERVICE_ITEM &rDisDbgServiceItem : _rDisClientDbgService_X.puDisService->GetDbgDisService().DisDbgServiceItemCollection)
  }   // if (IsNodeOpen_B)
  ImGui::TreePop();
  if ((PageLayoutPageIndexClicked_S32 != DIS_CLIENT_INVALID_INDEX) && (SubPageLayoutPageIndexClicked_S32 != DIS_CLIENT_INVALID_INDEX))
  {
    // Update selection state
    // (process outside of tree loop to avoid visual inconsistencies during the clicking frame)
    if ((ImGui::GetIO().KeyCtrl) && (_rPageIndex_S32 == PageLayoutPageIndexClicked_S32) && (_rSubPageIndex_S32 == SubPageLayoutPageIndexClicked_S32))
    {
      _rPageIndex_S32 = DIS_CLIENT_INVALID_INDEX;
      _rSubPageIndex_S32 = DIS_CLIENT_INVALID_INDEX;
    }
    else // Depending on selection behavior you want, may want to preserve selection when clicking on item that is part of the selection
    {
      _rPageIndex_S32 = PageLayoutPageIndexClicked_S32;
      _rSubPageIndex_S32 = SubPageLayoutPageIndexClicked_S32;
      // selection_mask = (1 6<< mPageLayoutLeafIdSelected_U32); // Click to single-select
    }
  } // if (PageLayoutPageIndexClicked_S32 != DIS_CLIENT_INVALID_INDEX)
  ImGui::End();
}

void DisClient::UpdateConsoleMenubar(DIS_CLIENT_DBG_SERVICE &_rDisClientDbgService_X)
{
  std::string MenuBar_S, Text_S;
  DIS_DBG_PAGE_LAYOUT DisDbgPage_X;

  MenuBar_S = std::string(mDisClientParam_X.ConsoleWidth_U32, ' ');
  if (IS_PAGE_SUBPAGE_LAYOUT_VALID(_rDisClientDbgService_X))
  {
    DisDbgPage_X = _rDisClientDbgService_X.puDisService->GetDbgDisService().DisDbgServiceItemCollection[mDisServiceIndex_S32].DisDbgPageLayoutCollection[_rDisClientDbgService_X.PageIndex_S32];
    if (DisDbgPage_X.MaxChannel_U32)
    {
      Text_S = BOF::Bof_Sprintf("%03d: %-16s | %-24s | %02d/%02d",
                                DisDbgPage_X.Page_U32, DisDbgPage_X.Label_S.c_str(),
                                DisDbgPage_X.DisDbgSubPageCollection[_rDisClientDbgService_X.SubPageIndex_S32].Label_S.c_str(),
                                DisDbgPage_X.Channel_U32, DisDbgPage_X.MaxChannel_U32);
    }
    else
    {
      Text_S = BOF::Bof_Sprintf("%03d: %-16s | %-24s",
                                DisDbgPage_X.Page_U32, DisDbgPage_X.Label_S.c_str(),
                                DisDbgPage_X.DisDbgSubPageCollection[_rDisClientDbgService_X.SubPageIndex_S32].Label_S.c_str());
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
void DisClient::S_HexaColor(const std::string &_rHexaColor_S, uint8_t (&_rColor_U8)[4])
{
  uint32_t Rgba_U32;

  if ((_rHexaColor_S.size() == (1 + 6)) && (_rHexaColor_S[0] == '#'))
  {
    Rgba_U32 = (uint32_t)BOF::Bof_StrToBin(16, _rHexaColor_S.c_str());
    _rColor_U8[0] = (uint8_t)(Rgba_U32 >> 16);
    _rColor_U8[1] = (uint8_t)(Rgba_U32 >> 8);
    _rColor_U8[2] = (uint8_t)(Rgba_U32);
    _rColor_U8[3] = 0;
  }
}
void DisClient::DisplayPageInfo(int32_t _x_U32, int32_t _y_U32, DIS_CLIENT_DBG_SERVICE &_rDisClientDbgService_X)
{
  uint32_t i_U32; // x_U32, y_U32;
  float Width_f, Height_f;
  uint8_t pColor_U8[4];
  const std::vector<DIS_SERVICE_PAGE_INFO_LINE> *pPageInfoLineCollection;

  if ((mConsoleCharSize_X.Width == 0) || (mConsoleCharSize_X.Height == 0))
  {
    ImGui::PushFont(mpConsoleFont_X);
    mConsoleCharSize_X = GetTextSize("A");
    ImGui::PopFont();
  }
  if ((mConsoleCharSize_X.Width) && (mConsoleCharSize_X.Height))
  {
    Width_f = mDisClientParam_X.ConsoleWidth_U32 * mConsoleCharSize_X.Width;
    Height_f = (mDisClientParam_X.ConsoleHeight_U32 * mConsoleCharSize_X.Height) + IMGUI_WINDOW_TITLEBAR_HEIGHT;
    // ImGui::SetNextWindowSize(ImVec2(Width_f, Height_f));
    // ImGui::SetNextWindowSizeConstraints(ImVec2(Width_f, Height_f), ImVec2(Width_f, Height_f));

    S_HexaColor(_rDisClientDbgService_X.puDisService->GetDbgDisService().PageInfo_X.BackColor_S, pColor_U8);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(pColor_U8[0], pColor_U8[1], pColor_U8[2], pColor_U8[3])); // Clear all
    ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar);
    ImGui::SetWindowPos(ImVec2(_x_U32, _y_U32), ImGuiCond_FirstUseEver);
    ImGui::SetWindowSize(ImVec2(Width_f, Height_f), ImGuiCond_FirstUseEver);
    if (IS_PAGE_SUBPAGE_LAYOUT_VALID(_rDisClientDbgService_X))
    {
      // Screen
      for (i_U32 = 0; i_U32 < 2; i_U32++) // i_U32==0: Draw background i_U32==1: Draw foreground
      {
        if (i_U32 == 0)
        {
          UpdateConsoleMenubar(_rDisClientDbgService_X);
          pPageInfoLineCollection = &_rDisClientDbgService_X.puDisService->GetDbgDisService().PageInfo_X.BackPageInfoLineCollection;
        }
        else
        {
          pPageInfoLineCollection = &_rDisClientDbgService_X.puDisService->GetDbgDisService().PageInfo_X.ForePageInfoLineCollection;
        }
        for (const auto &rLine : (*pPageInfoLineCollection))
        {
          PrintAt(rLine.x_U32, rLine.y_U32, rLine.ForeColor_S, rLine.BackColor_S, rLine.Text_S);
        }
      } // for (i_U32 = 0; i_U32 < 2; i_U32++)
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
    } // if (IS_PAGE_SUBPAGE_LAYOUT_VALID(_rDisClientDbgService_X))
  }   // if ((mConsoleCharSize_X.Width) && (mConsoleCharSize_X.Height))
}

void DisClient::V_PreNewFrame()
{
  DIS_SERVICE_STATE Sts_E;
  DIS_DBG_STATE_MACHINE StateMachine_X;

  for (auto &rDisClientDbgService : mDisClientDbgServiceCollection)
  {
    StateMachine_X.Channel_U32 = 0;
    StateMachine_X.CtrlFlag_U32 = 0;
    StateMachine_X.DisService_S = "";
    StateMachine_X.FctKeyFlag_U32 = 0;
    StateMachine_X.Page_U32 = 0;
    StateMachine_X.SubPage_U32 = 0;
    StateMachine_X.UserInput_S = "";

    Sts_E = rDisClientDbgService.second.puDisService->StateMachine(StateMachine_X);
  }
}

BOFERR DisClient::V_RefreshGui()
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;
  int32_t DisServiceIndex_S32, PageIndex_S32, SubPageIndex_S32;
  uint32_t i_U32, x_U32, y_U32;

  x_U32 = 0;
  y_U32 = 0;
  std::lock_guard Lock(mDisDiscoveryServiceCollectionMtx);
  {
    for (auto &rDisClientDbgService_X : mDisClientDbgServiceCollection)
    {
      DisServiceIndex_S32 = mDisServiceIndex_S32;
      PageIndex_S32 = rDisClientDbgService_X.second.PageIndex_S32;
      SubPageIndex_S32 = rDisClientDbgService_X.second.SubPageIndex_S32;

      DisplayDisService(x_U32, y_U32, rDisClientDbgService_X.second, DisServiceIndex_S32, PageIndex_S32, SubPageIndex_S32);
      if (DisServiceIndex_S32 != mDisServiceIndex_S32)
      {
        mDisServiceIndex_S32 = DisServiceIndex_S32;
      }
      if (PageIndex_S32 != rDisClientDbgService_X.second.PageIndex_S32)
      {
        rDisClientDbgService_X.second.PageIndex_S32 = PageIndex_S32;
      }
      if (SubPageIndex_S32 != rDisClientDbgService_X.second.SubPageIndex_S32)
      {
        rDisClientDbgService_X.second.SubPageIndex_S32 = SubPageIndex_S32;
      }
    }
  }
  return Rts_E;
}

BOFERR DisClient::PrintAt(uint32_t _x_U32, uint32_t _y_U32, const std::string &_rHexaForeColor_S, const std::string &_rHexaBackColor_S, const std::string &_rText_S)
{
  BOFERR Rts_E = BOF_ERR_EINVAL;
  BOF::Bof_ImGui_ImTextCustomization TextCustomization_X;
  BOF::BOF_POINT_2D<int32_t> CursorPos_X;
  uint8_t pColor_U8[4];
  ImColor Fore_X, Back_X;

  if ((_x_U32 < mDisClientParam_X.ConsoleWidth_U32) && (_y_U32 < mDisClientParam_X.ConsoleHeight_U32) && ((_x_U32 + _rText_S.size()) <= mDisClientParam_X.ConsoleWidth_U32))
  {
    S_HexaColor(_rHexaForeColor_S, pColor_U8);
    Fore_X = ImColor(pColor_U8[0], pColor_U8[1], pColor_U8[2], pColor_U8[3]);
    S_HexaColor(_rHexaBackColor_S, pColor_U8);
    Back_X = ImColor(pColor_U8[0], pColor_U8[1], pColor_U8[2], pColor_U8[3]);
    // TextCustomization_X.Highlight
    CursorPos_X.x = _x_U32 * mConsoleCharSize_X.Width;
    CursorPos_X.y = (_y_U32 * mConsoleCharSize_X.Height) + IMGUI_WINDOW_TITLEBAR_HEIGHT;
    Rts_E = DisplayText(&CursorPos_X, _rText_S.c_str(), false, false, &TextCustomization_X.Clear().Range(_rText_S.c_str()).TextColor(Fore_X).Highlight(Back_X));
  }
  return Rts_E;
}

BOFERR DisClient::Run()
{
  BOFERR Rts_E;

  Rts_E = LaunchBofProcessingThread("DisDiscovery", false, mDisClientParam_X.PollTimeInMs_U32, BOF::BOF_THREAD_SCHEDULER_POLICY::BOF_THREAD_SCHEDULER_POLICY_OTHER,
                                    BOF::BOF_THREAD_PRIORITY::BOF_THREAD_PRIORITY_000, 0, 2000, 0);
  return Rts_E;
}
BOFERR DisClient::Stop()
{
  BOFERR Rts_E;

  Rts_E = DestroyBofProcessingThread(nullptr);
  return Rts_E;
}

BOFERR DisClient::V_OnProcessing()
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;
  uint32_t i_U32;
  std::map<BOF::BofGuid, DIS_SERVICE_DISCOVERED> DisDiscoveryServiceCollection;
  std::map<BOF::BofGuid, DIS_SERVICE_DISCOVERED> DisAddServiceCollection;
  std::map<BOF::BofGuid, DIS_SERVICE_DISCOVERED> DisDelServiceCollection;
  DIS_CLIENT_DBG_SERVICE DisClientDbgService_X;
  DIS_SERVICE_DISCOVERED DisServiceDiscovered_X;
  DIS_SERVICE_PARAM DisServiceParam_X;

  // Periodic thread in  LaunchBofProcessingThread do
  std::lock_guard Lock(mDisDiscoveryServiceCollectionMtx);
  {
    DisDiscoveryServiceCollection = mpuDisDiscovery->GetDisDiscoveryServiceCollection();
    for (const auto &rExistingItem : DisDiscoveryServiceCollection)
    {
      const auto &rItem = mDisDiscoveryServiceCollection.find(rExistingItem.first);
      if (rItem == mDisDiscoveryServiceCollection.end())
      {
        DisAddServiceCollection[rExistingItem.first] = rExistingItem.second;
      }
      else
      {
        DisDelServiceCollection[rExistingItem.first] = rExistingItem.second;
      }
    }
    for (const auto &rItem : DisDelServiceCollection)
    {
      mDisClientDbgServiceCollection.erase(rItem.first);  //destructor called ?
      //mDisClientDbgServiceCollection[rItem.first].puDisService.reset()
    }
    for (const auto &rItem : DisAddServiceCollection)
    {
      DisServiceParam_X.Reset();
      DisServiceParam_X.DisServerEndpoint_S = "ws://" + rItem.second.IpAddress_S + ":8080";
      DisServiceParam_X.PollTimeInMs_U32 = mDisClientParam_X.DisServerPollTimeInMs_U32;
      DisServiceParam_X.DisService_X = rItem.second;

      DisClientDbgService_X.IsVisisble_B = true;
      DisClientDbgService_X.puDisService= std::make_unique<DisService>(DisServiceParam_X);;
      DisClientDbgService_X.PageIndex_S32=DIS_CLIENT_INVALID_INDEX;
      DisClientDbgService_X.SubPageIndex_S32= DIS_CLIENT_INVALID_INDEX;
      mDisClientDbgServiceCollection.emplace(std::make_pair(rItem.first, std::move(DisClientDbgService_X)));
    }
    mDisDiscoveryServiceCollection = DisDiscoveryServiceCollection;
  }
  // while ((!IsThreadLoopMustExit()) && (Rts_E == BOF_ERR_NO_ERROR));
  return Rts_E;
}
/*
  Xt1_DisService
  |
  + 10.129.171.112
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
    + P6X
      |
      + Cpu
        |
        + Temp
        + Task
        + Snmp
*/