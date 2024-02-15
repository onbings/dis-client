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

enum DIS_CLIENT_FONT
{
  DIS_CLIENT_FONT_CONSOLE = 0,
  DIS_CLIENT_FONT_SERVICE,
  DIS_CLIENT_FONT_EXTRA,
  DIS_CLIENT_FONT_SIMULATOR,
};
#define IMGUI_WINDOW_TITLEBAR_HEIGHT (mDisClientParam_X.FontSize_U32 + 8)
#define IS_DIS_SERVICE_VALID(DisClientDbgService) (((DisClientDbgService).ServiceIndex_S32 >= 0) && ((DisClientDbgService).ServiceIndex_S32 < (DisClientDbgService).puDisService->GetDbgDisService().DisDbgServiceItemCollection.size()))
#define IS_PAGE_SUBPAGE_LAYOUT_VALID(DisClientDbgService) ((IS_DIS_SERVICE_VALID(DisClientDbgService)) && ((DisClientDbgService).PageIndex_S32 >= 0) && ((DisClientDbgService).PageIndex_S32 < (DisClientDbgService).puDisService->GetDbgDisService().DisDbgServiceItemCollection[(DisClientDbgService).ServiceIndex_S32].DisDbgPageLayoutCollection.size()) && ((DisClientDbgService).SubPageIndex_S32 >= 0) && ((DisClientDbgService).SubPageIndex_S32 < (DisClientDbgService).puDisService->GetDbgDisService().DisDbgServiceItemCollection[(DisClientDbgService).ServiceIndex_S32].DisDbgPageLayoutCollection[(DisClientDbgService).PageIndex_S32].DisDbgSubPageCollection.size()))
// #define IS_PAGE_INFO_VALID(DisClientDbgService) (((DisClientDbgService).puDisService->GetDbgDisService().PageInfo_X.BackColor_S != "") && ((DisClientDbgService).puDisService->GetDbgDisService().PageInfo_X.BackPageInfoLineCollection.size()) &&((DisClientDbgService).puDisService->GetDbgDisService().PageInfo_X.ForePageInfoLineCollection.size()))
#define IS_PAGE_INFO_VALID(DisClientDbgService) ((DisClientDbgService).puDisService->GetDbgDisService().PageInfo_X.BackColor_S != "")
/*
     if ((PageIndex_S32 == 4) && (SubPageIndex_S32 == 5))
     {
       const DIS_DBG_SERVICE &rDisDbgSrv_X = rDisClientDbgService_X.second.puDisService->GetDbgDisService();
       const DIS_DBG_SERVICE_ITEM &rDisDbgSrvItem_X = rDisDbgSrv_X.DisDbgServiceItemCollection[rDisClientDbgService_X.second.ServiceIndex_S32];

       S_Log("DIS_SERVICE %d < %zd ?\n", rDisClientDbgService_X.second.ServiceIndex_S32, rDisDbgSrv_X.DisDbgServiceItemCollection.size());
       S_Log("PAGE_SUBPAGE Pg %d < %zd SubP %d < %zd\n", rDisClientDbgService_X.second.PageIndex_S32, rDisDbgSrvItem_X.DisDbgPageLayoutCollection.size(),
              rDisClientDbgService_X.second.SubPageIndex_S32, rDisDbgSrvItem_X.DisDbgPageLayoutCollection[rDisClientDbgService_X.second.PageIndex_S32].DisDbgSubPageCollection.size());
       S_Log("PAGE_INFO Bg '%s' NbLineBg %zd NbLineFg %zd\n", rDisDbgSrv_X.PageInfo_X.BackColor_S.c_str(), rDisDbgSrv_X.PageInfo_X.BackPageInfoLineCollection.size(), rDisDbgSrv_X.PageInfo_X.ForePageInfoLineCollection.size());
     }
*/
DisClient::DisClient(const DIS_CLIENT_PARAM &_rDisClientParam_X)
    : BOF::Bof_ImGui(_rDisClientParam_X.ImguiParam_X), BOF::BofThread()
{
  DIS_DISCOVERY_PARAM DisDiscoveryParam_X;
  BOF::BOF_DIR_GRAPH_PARAM BofDirGraphParam_X;
  DIS_DEVICE_SIMULATOR_ENTRY SimulatorEntry_X;

  mDisClientParam_X = _rDisClientParam_X;

  SimulatorEntry_X.DevicePresent_B = false;
  SimulatorEntry_X.DisDevice_X.Type_U32 = 1;
  SimulatorEntry_X.DisDevice_X.Sn_U32 = 0x00010203;
  SimulatorEntry_X.DisDevice_X.Name_S = "Xt1-DisDevice";
  SimulatorEntry_X.DisDevice_X.IpAddress_S = "10.129.171.112";
  mSimulatorEntryCollection.push_back(SimulatorEntry_X);

  SimulatorEntry_X.DevicePresent_B = false;
  SimulatorEntry_X.DisDevice_X.Type_U32 = 1;
  SimulatorEntry_X.DisDevice_X.Sn_U32 = 0x00010204;
  SimulatorEntry_X.DisDevice_X.Name_S = "Xt2-DisDevice";
  SimulatorEntry_X.DisDevice_X.IpAddress_S = "10.129.171.112";
  mSimulatorEntryCollection.push_back(SimulatorEntry_X);

  SimulatorEntry_X.DevicePresent_B = false;
  SimulatorEntry_X.DisDevice_X.Type_U32 = 1;
  SimulatorEntry_X.DisDevice_X.Sn_U32 = 0x00010205;
  SimulatorEntry_X.DisDevice_X.Name_S = "Xt3-DisDevice";
  SimulatorEntry_X.DisDevice_X.IpAddress_S = "10.129.171.112";
  mSimulatorEntryCollection.push_back(SimulatorEntry_X);

  DisDiscoveryParam_X.PollTimeInMs_U32 = 2000;
  mpuDisDiscovery = std::make_unique<DisDiscovery>(DisDiscoveryParam_X);
  mpuDisDiscovery->Run();

  BofDirGraphParam_X.MultiThreadAware_B = true;
  mpuDiscoveryGraph = std::make_unique<BOF::BofDirGraph<DIS_CLIENT_NODE>>(BofDirGraphParam_X);
  Run();
}

DisClient::~DisClient()
{
  mpuDisDiscovery->Stop();
  Stop();
  ImNodes::DestroyContext();
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
    Rts["DiscoverPollTimeInMs"] = mDisClientParam_X.DiscoverPollTimeInMs_U32;
    Rts["DisServerPollTimeInMs"] = mDisClientParam_X.DisServerPollTimeInMs_U32;
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
    mDisClientParam_X.DiscoverPollTimeInMs_U32 = _rJson.value("DiscoverPollTimeInMs", 2000);
    mDisClientParam_X.DisServerPollTimeInMs_U32 = _rJson.value("DisServerPollTimeInMs", 1000);
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

void DisClient::V_PostInit()
{
  DIS_CLIENT_NODE *pDisClientNode_X, DisClientNode_X;

  Bof_ImGui::V_PostInit();
  ImNodes::CreateContext();

  DisClientNode_X.Reset();
  DisClientNode_X.Name_S = BOF::Bof_GetHostName();
  DisClientNode_X.x_f = 10;
  DisClientNode_X.y_f = 10;
  mGraphRootNodeId_U32 = mpuDiscoveryGraph->InsertNode(DisClientNode_X);
  pDisClientNode_X = mpuDiscoveryGraph->Node(mGraphRootNodeId_U32);
  BOF_ASSERT(pDisClientNode_X);
  pDisClientNode_X->NodeId_U32 = mGraphRootNodeId_U32;
  ImNodes::SetNodeEditorSpacePos(pDisClientNode_X->NodeId_U32, ImVec2(pDisClientNode_X->x_f, pDisClientNode_X->y_f));
  // ImNodes::SetNodeScreenSpacePos(pDisClientNode_X[i_U32].NodeId_U32, Pos);

  ImNodes::GetIO().LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
  ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);
}

void DisClient::V_BeforeExit()
{
  ImNodes::DestroyContext();
}

void DisClient::V_LoadAdditionalFonts()
{
  BOF_ASSERT(LoadFont("./assets/fonts/cour.ttf", mDisClientParam_X.FontSize_U32) != nullptr); // DIS_CLIENT_FONT_CONSOLE
  BOF_ASSERT(LoadFont("./assets/fonts/DroidSans.ttf", 18) != nullptr);                        // DIS_CLIENT_FONT_SERVICE
  BOF_ASSERT(LoadFont("./assets/fonts/fontawesome-webfont.ttf", 18) != nullptr);              // DIS_CLIENT_FONT_EXTRA
  BOF_ASSERT(LoadFont("./assets/fonts/scientific-calculator-lcd.ttf", 18) != nullptr);        // DIS_CLIENT_FONT_SIMULATOR
}

void DisClient::DisplaySimulator(int32_t _x_U32, int32_t _y_U32)
{
  DIS_DEVICE DisDevice_X;

  ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[DIS_CLIENT_FONT_SIMULATOR]);
  bool IsVisisble_B = true;
  ImGui::Begin("Dis Discovery Simulator", &IsVisisble_B, ImGuiWindowFlags_None);
  // Set the window size to fit its contents
  ImGui::SetWindowSize(ImVec2(0, 0)); // Width and height set to 0 to auto-size
  ImGui::SetWindowPos(ImVec2(_x_U32, _y_U32), ImGuiCond_FirstUseEver);

  for (DIS_DEVICE_SIMULATOR_ENTRY &rIt : mSimulatorEntryCollection)
  {
    ImGui::Checkbox(rIt.DisDevice_X.Name_S.c_str(), &rIt.DevicePresent_B);
    // if (ImGui::IsItemActivated()) //initial uncheck, click gives 0 1 0  1
    if (ImGui::IsItemDeactivated()) // initial uncheck, click gives1 0 1 0
    // if (ImGui::IsItemEdited())    //initial uncheck, click gives 1 0 1 0
    {
      // S_Log("state %d\n", rIt.DevicePresent_B);
      if (rIt.DevicePresent_B)
      {
        mpuDisDiscovery->AddDevice(rIt.DisDevice_X);
      }
      else
      {
        mpuDisDiscovery->RemoveDevice(rIt.DisDevice_X);
      }
    }
  }

  ImGui::End();
  ImGui::PopFont();
}

void DisClient::DisplayDiscoveryGraph(int32_t _x_U32, int32_t _y_U32)
{
  DIS_DEVICE DisDevice_X;
  DIS_CLIENT_NODE *pDisClientNode_X;
  std::vector<uint32_t> const *pNodeHeighbourCollection;

  char pPin_c[256];
  uint32_t PinId_U32, FirstOutId_U32, InIndex_U32, LinkId_U32;
  bool IsVisisble_B = true;

  PinId_U32 = 1;
  FirstOutId_U32 = 0;
  InIndex_U32 = 0;
  LinkId_U32 = 1;

  ImGui::Begin("Dis Discovery Graph", &IsVisisble_B, ImGuiWindowFlags_None);
  // Set the window size to fit its contents
  // ImGui::SetWindowSize(ImVec2(0, 0)); // Width and height set to 0 to auto-size
  ImGui::SetWindowSize(ImVec2(700, 350), ImGuiCond_FirstUseEver);
  ImGui::SetWindowPos(ImVec2(_x_U32, _y_U32), ImGuiCond_FirstUseEver);

  ImNodes::BeginNodeEditor();
  ImNodes::PushColorStyle(ImNodesCol_GridBackground, IM_COL32(0, 255, 0, 255));
  ImNodes::PushStyleVar(ImNodesStyleVar_GridSpacing, 100.0f); // ImVec2(100, 50));
  //    ImNodes::StyleColorsLight();
  // ImNodesStyle &rNodeStyle = ImNodes::GetStyle();
  // rNodeStyle.GridSpacing = 100;
  // rNodeStyle.Colors[ImNodesCol_NodeBackground] = ImColor(255, 0, 0, 255);

  auto &rNodeCollection = mpuDiscoveryGraph->NodeMap();
  mDiscoveryLinkCollection.clear();

  pDisClientNode_X = mpuDiscoveryGraph->Node(mGraphRootNodeId_U32);
  if (pDisClientNode_X)
  {
    ImNodes::BeginNode(mGraphRootNodeId_U32);
    FirstOutId_U32 = PinId_U32;
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(pDisClientNode_X->Name_S.c_str());
    ImNodes::EndNodeTitleBar();
    pNodeHeighbourCollection = mpuDiscoveryGraph->Neighbour(mGraphRootNodeId_U32);
    if (pNodeHeighbourCollection)
    {
      for (uint32_t NeighborId_U32 : *pNodeHeighbourCollection)
      {
        ImNodes::BeginOutputAttribute(PinId_U32, ImNodesPinShape_Triangle);
        ImGui::Indent(40);
        sprintf(pPin_c, "Dis_%d", PinId_U32);
        ImGui::Text(pPin_c);
        ImNodes::EndOutputAttribute();
        // DisClient::S_Log("Node %d pin %s %d\n", rNode_X.NodeId_U32, pTitle_c, PinId_U32);
        PinId_U32++;
      }
    }
    ImNodes::EndNode();
  }
  for (DIS_CLIENT_NODE &rNode_X : rNodeCollection)
  {
    if (rNode_X.NodeId_U32 != mGraphRootNodeId_U32)
    {
      ImNodes::BeginNode(rNode_X.NodeId_U32);
      // ImNodes::BeginNodeTitleBar();
      // sprintf(pTitle_c, "Node_%d", rNode_X.NodeId_U32);
      // ImGui::TextUnformatted(pTitle_c);
      // ImNodes::EndNodeTitleBar();
      ImNodes::BeginInputAttribute(PinId_U32, ImNodesPinShape_Circle);
      ImGui::Indent(40);
      sprintf(pPin_c, "%s", rNode_X.Name_S.c_str());
      ImGui::Text(pPin_c);
      // ImGui::DragFloat("Val", &rNode_X.Value_f, 0.01f, -10.0f, 10.0f,"%.2f");
      ImNodes::EndInputAttribute();
      // DisClient::S_Log("Node %d pin %s %d\n", rNode_X.NodeId_U32, pTitle_c, PinId_U32);

      mDiscoveryLinkCollection.push_back(std::make_pair(FirstOutId_U32 + InIndex_U32, PinId_U32));
      PinId_U32++;
      InIndex_U32++;

      ImNodes::EndNode();
    }
  }
  ImNodes::PopColorStyle();
  ImNodes::PopStyleVar();
  for (auto &rLink : mDiscoveryLinkCollection)
  {
    // DisClient::S_Log("Link %d %d->%d\n", LinkId_U32, rLink.first, rLink.second);
    ImNodes::Link(LinkId_U32++, rLink.first, rLink.second);
  }
  ImNodes::MiniMap(0.25f, ImNodesMiniMapLocation_BottomLeft);
  ImNodes::EndNodeEditor();

  ImGui::End();
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
  uint32_t DisServiceIndex_U32, PageIndex_U32, SubPageIndex_U32;
  int32_t DisServiceIndexClicked_S32, PageLayoutPageIndexClicked_S32, SubPageLayoutPageIndexClicked_S32;
  bool IsNodeOpen_B;
  const DIS_DBG_SERVICE &rDisDbgService_X = _rDisClientDbgService_X.puDisService->GetDbgDisService();

  DisServiceIndexClicked_S32 = DIS_CLIENT_INVALID_INDEX;
  PageLayoutPageIndexClicked_S32 = DIS_CLIENT_INVALID_INDEX;
  SubPageLayoutPageIndexClicked_S32 = DIS_CLIENT_INVALID_INDEX;

  ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[DIS_CLIENT_FONT_SERVICE]);
  ImGui::Begin(_rDisClientDbgService_X.puDisService->GetDbgDisService().DisDevice_X.MetaData_X.DeviceUniqueKey_S.c_str(), &_rDisClientDbgService_X.IsVisisble_B, ImGuiWindowFlags_None); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
  // Set the window size to fit its contents
  ImGui::SetWindowSize(ImVec2(0, 0)); // Width and height set to 0 to auto-size

  ImGui::SetWindowPos(ImVec2(_x_U32, _y_U32), ImGuiCond_FirstUseEver);
  // IsNodeOpen_B = ImGui::TreeNodeEx("##rDisDbgService", NodeFlag, "%s-%s", rDisDbgService_X.Name_S.c_str(), rDisDbgService_X.IpAddress_S.c_str());
  // if (IsNodeOpen_B)
  //{
  S_BuildHelpMarker("This is the list of the different dis service avalaible on this device.\n"
                    "Click to select, CTRL+Click to toggle, click on arrows or double-click to open.");
  // for (DIS_DBG_SERVICE_ITEM &rDisDbgServiceItem : _rDisClientDbgService_X.puDisService->GetDbgDisService().DisDbgServiceItemCollection)
  DisServiceIndex_U32 = 0;
  for (const DIS_DBG_SERVICE_ITEM &rDisDbgServiceItem : rDisDbgService_X.DisDbgServiceItemCollection)
  {
    if (_rDisServiceIndex_S32 == DisServiceIndex_U32)
    {
      ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));
    }
    IsNodeOpen_B = ImGui::TreeNodeEx("##rDisDbgServiceItem", NodeFlag, "%s", rDisDbgServiceItem.Name_S.c_str());
    if (ImGui::IsItemHovered())
    {
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
    if (_rDisServiceIndex_S32 == DisServiceIndex_U32)
    {
      ImGui::PopStyleColor();
    }
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
      DisServiceIndexClicked_S32 = DisServiceIndex_U32;
    }
    if (IsNodeOpen_B)
    {
      // Disable the default "open on single-click behavior" + set Selected flag according to our selection.
      // To alter selection we use IsItemClicked() && !IsItemToggledOpen(), so clicking on an arrow doesn't alter selection.
      ItemFlag = LeafFlag;
      IsNodeOpen_B = ImGui::TreeNodeEx("##rDisDbgServiceItemData", ItemFlag, "%s", rDisDbgServiceItem.IpAddress_S.c_str());
      if (IsNodeOpen_B)
      {
        PageIndex_U32 = 0;
        for (const DIS_DBG_PAGE_LAYOUT &rDisDbgPageLayout : rDisDbgServiceItem.DisDbgPageLayoutCollection)
        {
          IsNodeOpen_B = ImGui::TreeNodeEx("##rDisDbgPageLayout", NodeFlag, "%s", rDisDbgPageLayout.Label_S.c_str());
          if (IsNodeOpen_B)
          {
            SubPageIndex_U32 = 0;
            for (const DIS_DBG_SUB_PAGE_LAYOUT &rDisDbgSubPageLayout : rDisDbgPageLayout.DisDbgSubPageCollection)
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
              IsNodeOpen_B = ImGui::TreeNodeEx("##rDisDbgPageLayoutData", ItemFlag, "%s", rDisDbgSubPageLayout.Label_S.c_str());
              if (IsNodeOpen_B)
              {
                if ((_rPageIndex_S32 == PageIndex_U32) && (_rSubPageIndex_S32 == SubPageIndex_U32))
                {
                  ImGui::PopStyleColor();
                }
                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
                {
                  PageLayoutPageIndexClicked_S32 = PageIndex_U32;
                  SubPageLayoutPageIndexClicked_S32 = SubPageIndex_U32;
                }
                // Leaf ImGui::TreePop();
              }
              SubPageIndex_U32++;
            } // for (DIS_DBG_SUB_PAGE_LAYOUT &rDisDbgSubPageLayout : rDisDbgPageLayout.DisDbgSubPageCollection)
            ImGui::TreePop();
          } // if (IsNodeOpen_B)
          PageIndex_U32++;
        } // for (DIS_DBG_PAGE_LAYOUT &rDisDbgPageLayout : rDisDbgServiceItem.DisDbgPageLayoutCollection)
        // Leaf ImGui::TreePop();
      } // if (IsNodeOpen_B)
      ImGui::TreePop();
    } // if (IsNodeOpen_B)
    DisServiceIndex_U32++;
  } // for (const DIS_DBG_SERVICE_ITEM &rDisDbgServiceItem : rDisDbgService_X.DisDbgServiceItemCollection)
  //  ImGui::TreePop();
  //} // if (IsNodeOpen_B)

  if (DisServiceIndexClicked_S32 != DIS_CLIENT_INVALID_INDEX)
  {
    // Update selection state (process outside of tree loop to avoid visual inconsistencies during the clicking frame)
    if ((ImGui::GetIO().KeyCtrl) && (_rDisServiceIndex_S32 == DisServiceIndexClicked_S32))
    {
      _rDisServiceIndex_S32 = DIS_CLIENT_INVALID_INDEX;
    }
    else // Depending on selection behavior you want, may want to preserve selection when clicking on item that is part of the selection
    {
      _rDisServiceIndex_S32 = DisServiceIndexClicked_S32;
      // selection_mask = (1 6<< mPageLayoutLeafIdSelected_U32); // Click to single-select
    }
  } // if (DisServiceIndexClicked_S32 != DIS_CLIENT_INVALID_INDEX)

  if ((PageLayoutPageIndexClicked_S32 != DIS_CLIENT_INVALID_INDEX) && (SubPageLayoutPageIndexClicked_S32 != DIS_CLIENT_INVALID_INDEX))
  {
    if ((ImGui::GetIO().KeyCtrl) && (_rPageIndex_S32 == PageLayoutPageIndexClicked_S32) && (_rSubPageIndex_S32 == SubPageLayoutPageIndexClicked_S32))
    {
      _rPageIndex_S32 = DIS_CLIENT_INVALID_INDEX;
      _rSubPageIndex_S32 = DIS_CLIENT_INVALID_INDEX;
    }
    else
    {
      _rPageIndex_S32 = PageLayoutPageIndexClicked_S32;
      _rSubPageIndex_S32 = SubPageLayoutPageIndexClicked_S32;
    }
  } // if ((PageLayoutPageIndexClicked_S32 != DIS_CLIENT_INVALID_INDEX) && (SubPageLayoutPageIndexClicked_S32 != DIS_CLIENT_INVALID_INDEX))
  ImGui::End();
  ImGui::PopFont();
}

void DisClient::UpdateConsoleMenubar(DIS_CLIENT_DBG_SERVICE &_rDisClientDbgService_X)
{
  std::string MenuBar_S, Text_S;
  DIS_DBG_PAGE_LAYOUT DisDbgPage_X;

  MenuBar_S = std::string(mDisClientParam_X.ConsoleWidth_U32, ' ');
  if (IS_PAGE_SUBPAGE_LAYOUT_VALID(_rDisClientDbgService_X))
  {
    DisDbgPage_X = _rDisClientDbgService_X.puDisService->GetDbgDisService().DisDbgServiceItemCollection[_rDisClientDbgService_X.ServiceIndex_S32].DisDbgPageLayoutCollection[_rDisClientDbgService_X.PageIndex_S32];
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

void DisClient::DisplayPageInfo(int32_t _x_U32, int32_t _y_U32, DIS_CLIENT_DBG_SERVICE &_rDisClientDbgService_X)
{
  uint32_t i_U32; // x_U32, y_U32;
  float Width_f, Height_f;
  uint8_t pColor_U8[4];
  char pTitle_c[128];
  const std::vector<DIS_SERVICE_PAGE_INFO_LINE> *pPageInfoLineCollection;

  ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[DIS_CLIENT_FONT_CONSOLE]);
  if ((mConsoleCharSize_X.Width == 0) || (mConsoleCharSize_X.Height == 0))
  {
    mConsoleCharSize_X = GetTextSize("A");
  }
  if ((mConsoleCharSize_X.Width) && (mConsoleCharSize_X.Height))
  {
    Width_f = mDisClientParam_X.ConsoleWidth_U32 * mConsoleCharSize_X.Width;
    Height_f = (mDisClientParam_X.ConsoleHeight_U32 * mConsoleCharSize_X.Height) + IMGUI_WINDOW_TITLEBAR_HEIGHT;
    // ImGui::SetNextWindowSize(ImVec2(Width_f, Height_f));
    // ImGui::SetNextWindowSizeConstraints(ImVec2(Width_f, Height_f), ImVec2(Width_f, Height_f));

    S_HexaColor(_rDisClientDbgService_X.puDisService->GetDbgDisService().PageInfo_X.BackColor_S, pColor_U8);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(pColor_U8[0] / 255.0f, pColor_U8[1] / 255.0f, pColor_U8[2] / 255.0f, pColor_U8[3] / 255.0f)); // Clear all

    // DisClient::S_Log("back is %s\n", _rDisClientDbgService_X.puDisService->GetDbgDisService().PageInfo_X.BackColor_S.c_str());
    sprintf(pTitle_c, "Console on %s", _rDisClientDbgService_X.puDisService->GetDbgDisService().DisDevice_X.MetaData_X.DeviceUniqueKey_S.c_str());
    ImGui::Begin(pTitle_c, nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar);
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
    } // if (IS_PAGE_SUBPAGE_LAYOUT_VALID(_rDisClientDbgService_X))
    ImGui::PopStyleColor();
  } // if ((mConsoleCharSize_X.Width) && (mConsoleCharSize_X.Height))
  ImGui::PopFont();
}

void DisClient::V_PreNewFrame()
{
  DIS_SERVICE_STATE Sts_E;
  DIS_DBG_STATE_MACHINE StateMachine_X;

  for (auto &rDisClientDbgService_X : mDisClientDbgServiceCollection)
  {
    StateMachine_X.DisDevice_X = rDisClientDbgService_X.second.puDisService->GetDbgDisService().DisDevice_X;
    StateMachine_X.Channel_U32 = 0;
    StateMachine_X.CtrlFlag_U32 = 0;
    if (IS_DIS_SERVICE_VALID(rDisClientDbgService_X.second))
    {
      StateMachine_X.DisService_S = rDisClientDbgService_X.second.puDisService->GetDbgDisService().DisDbgServiceItemCollection[rDisClientDbgService_X.second.ServiceIndex_S32].Name_S;
    }
    else
    {
      StateMachine_X.DisService_S = "";
    }
    if (IS_PAGE_SUBPAGE_LAYOUT_VALID(rDisClientDbgService_X.second))
    {
      // StateMachine_X.Page_U32 = rDisClientDbgService_X.second.PageIndex_S32;
      // const DIS_DBG_SERVICE &a=rDisClientDbgService_X.second.puDisService->GetDbgDisService();
      StateMachine_X.Page_U32 = rDisClientDbgService_X.second.puDisService->GetDbgDisService().DisDbgServiceItemCollection[rDisClientDbgService_X.second.ServiceIndex_S32].DisDbgPageLayoutCollection[rDisClientDbgService_X.second.PageIndex_S32].Page_U32;
      StateMachine_X.SubPage_U32 = rDisClientDbgService_X.second.SubPageIndex_S32;
      StateMachine_X.CtrlFlag_U32 |= (DIS_CTRL_FLAG_BACK | DIS_CTRL_FLAG_FORE);
    }
    else
    {
      StateMachine_X.Page_U32 = DIS_CLIENT_INVALID_INDEX;
      StateMachine_X.SubPage_U32 = DIS_CLIENT_INVALID_INDEX;
    }
    StateMachine_X.FctKeyFlag_U32 = 0;
    StateMachine_X.UserInput_S = "";
    //    DisClient::S_Log("V_PreNewFrame: '%s' call StateMachine at %s\n", rDisClientDbgService_X.second.puDisService->GetDbgDisService().Name_S.c_str(), rDisClientDbgService_X.second.puDisService->GetDbgDisService().IpAddress_S.c_str());
    Sts_E = rDisClientDbgService_X.second.puDisService->StateMachine(StateMachine_X);
  }
}
// Function to check if the mouse is hovering over a circle
bool IsMouseHoveringCircle(const ImVec2 &center, float radius)
{
  ImVec2 mousePos = ImGui::GetMousePos();
  return (ImLengthSqr(mousePos - center) <= (radius * radius));
}
// Function to display small circles evenly spaced on the circumference of the big circle
void DisplayCircleConnector(ImDrawList *drawList, const ImVec2 &center, float bigRadius, int numSmallCircles, float smallRadius, ImU32 smallCircleColor)
{
  // Calculate angle increment for evenly spacing the small circles
  float angleIncrement = 2 * IM_PI / numSmallCircles;

  // Draw small circles along the circumference
  for (int i = 0; i < numSmallCircles; ++i)
  {
    // Calculate position of the small circle
    float angle = i * angleIncrement;
    ImVec2 smallCirclePos(center.x + bigRadius * std::cos(angle), center.y + bigRadius * std::sin(angle));

    // Draw the small circle
    drawList->AddCircleFilled(smallCirclePos, smallRadius, smallCircleColor);
  }
}
// Function to draw a blue circle with specified parameters
void DrawCircleWithText(ImDrawList *drawList, const ImVec2 &center, float radius, ImU32 fillColor, ImU32 borderColor, const char *text, const ImVec4 &textColor)
{
  // Check if the mouse is hovering over the circle
  bool isHovered = IsMouseHoveringCircle(center, radius);

  // Invert colors if hovered
  ImU32 finalFillColor = isHovered ? borderColor : fillColor;
  ImU32 finalBorderColor = isHovered ? fillColor : borderColor;

  // Draw filled circle
  drawList->AddCircleFilled(center, radius, finalFillColor);

  // Draw border of the circle
  drawList->AddCircle(center, radius, finalBorderColor, 0, 5.0f);

  // Calculate text size
  ImVec2 textSize = ImGui::CalcTextSize(text);

  // Calculate text position at the center
  ImVec2 textPosition(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f);

  // Draw the text
  drawList->AddText(textPosition, IM_COL32(int(textColor.x * 255), int(textColor.y * 255), int(textColor.z * 255), int(textColor.w * 255)), text);
}

BOFERR DisClient::V_RefreshGui()
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;
  int32_t DisServiceIndex_S32, PageIndex_S32, SubPageIndex_S32;
  uint32_t i_U32, x_U32, y_U32;

  x_U32 = 0;
  y_U32 = 0;
  std::lock_guard Lock(mDisDeviceCollectionMtx);
  {
    for (auto &rDisClientDbgService_X : mDisClientDbgServiceCollection)
    {
      DisServiceIndex_S32 = rDisClientDbgService_X.second.ServiceIndex_S32;
      PageIndex_S32 = rDisClientDbgService_X.second.PageIndex_S32;
      SubPageIndex_S32 = rDisClientDbgService_X.second.SubPageIndex_S32;
      DisplayDisService(x_U32, y_U32, rDisClientDbgService_X.second, DisServiceIndex_S32, PageIndex_S32, SubPageIndex_S32);

      //      DisClient::S_Log("V_RefreshGui: '%s' call DisplayDisService for %s\n", rDisClientDbgService_X.second.puDisService->GetDbgDisService().Name_S.c_str(), rDisClientDbgService_X.second.puDisService->GetDbgDisService().IpAddress_S.c_str());

      if (DisServiceIndex_S32 != rDisClientDbgService_X.second.ServiceIndex_S32)
      {
        rDisClientDbgService_X.second.ServiceIndex_S32 = DisServiceIndex_S32;
        rDisClientDbgService_X.second.puDisService->ResetPageInfoTimer(); // To refresh asap
      }
      if (PageIndex_S32 != rDisClientDbgService_X.second.PageIndex_S32)
      {
        rDisClientDbgService_X.second.PageIndex_S32 = PageIndex_S32;
        rDisClientDbgService_X.second.puDisService->ResetPageInfoTimer(); // To refresh asap
      }
      if (SubPageIndex_S32 != rDisClientDbgService_X.second.SubPageIndex_S32)
      {
        rDisClientDbgService_X.second.SubPageIndex_S32 = SubPageIndex_S32;
        rDisClientDbgService_X.second.puDisService->ResetPageInfoTimer(); // To refresh asap
      }
      if (IS_PAGE_SUBPAGE_LAYOUT_VALID(rDisClientDbgService_X.second))
      {
        if (IS_PAGE_INFO_VALID(rDisClientDbgService_X.second))
        {
          DisplayPageInfo(x_U32 + 500, y_U32, rDisClientDbgService_X.second);
        }
      }
      x_U32 += 50;
      y_U32 += 50;
    }
  }

  DisplaySimulator(900, 500);

  DisplayDiscoveryGraph(1100, 100);

#if 0 // Circle
  //NODE
  // Example values
  ImVec2 circleCenter(400.0f, 400.0f);
  float circleRadius = 50.0f;
  ImU32 fillColor = IM_COL32(0, 0, 255, 255); 
  ImU32 borderColor = IM_COL32(0, 255, 0, 255);  
  const char *labelText = "Click me";
  ImVec4 labelTextColor(1.0f, 1.0f, 0.0f, 1.0f);  // Yellow color
  int numSmallCircles = 8;
  float smallCircleRadius = 10.0f;
  ImU32 smallCircleColor = IM_COL32(255, 0, 0, 255);  // Red color for small circles

  // Get ImGui draw list
  ImDrawList *drawList = ImGui::GetWindowDrawList();



  ImNodes::BeginNodeEditor();

  static int dbha = 1;
  if (dbha == 1) goto l;
  if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
  {
    if (ImNodes::IsEditorHovered())
    {
      if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
      {
l:
        dbha++;
        DIS_CLIENT_NODE pDisClientNode_X[1 + 8];
        uint32_t i_U32, pEdgeId_U32[1 + 8];
        ImVec2 Pos = ImGui::GetMousePos();
        for (i_U32 = 0; i_U32 < BOF_NB_ELEM_IN_ARRAY(pDisClientNode_X); i_U32++)
        {
          pDisClientNode_X[i_U32].Reset();
          pDisClientNode_X[i_U32].Value_f=i_U32;
          pDisClientNode_X[i_U32].NodeId_U32 = mpuDiscoveryGraph->InsertNode(pDisClientNode_X[i_U32]);
          //ImNodes::SetNodeScreenSpacePos(pDisClientNode_X[i_U32].NodeId_U32, Pos);
        }
        for (i_U32 = 1; i_U32 < BOF_NB_ELEM_IN_ARRAY(pDisClientNode_X); i_U32++)
        {
          pEdgeId_U32[i_U32] = mpuDiscoveryGraph->InsertEdge(pDisClientNode_X[0].NodeId_U32, pDisClientNode_X[i_U32].NodeId_U32);
        }

        //NodeId_U32 = mpuDiscoveryGraph->InsertNode(DisClientNode_X);
        //mNodeIdCollection.push_back(NodeId_U32);

        //mpuDiscoveryGraph->InsertEdge(NodeId_U32, NodeId_U32);
        //mpuDiscoveryGraph->InsertEdge(NodeId_U32, NodeId_U32);

      }
    }
  }


  // Draw the circle with text
  DrawCircleWithText(drawList, circleCenter, circleRadius, fillColor, borderColor, labelText, labelTextColor);
  DisplayCircleConnector(drawList, circleCenter, circleRadius, numSmallCircles, smallCircleRadius, smallCircleColor);

  // Example logic: Check if the mouse is over the circle and if the left mouse button is pressed
  if (IsMouseHoveringCircle(circleCenter, circleRadius) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
  {
  }
#endif

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

  Rts_E = LaunchBofProcessingThread("DisDiscovery", false, mDisClientParam_X.DiscoverPollTimeInMs_U32, BOF::BOF_THREAD_SCHEDULER_POLICY::BOF_THREAD_SCHEDULER_POLICY_OTHER,
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
  uint32_t i_U32, NodeId_U32, EdgeId_U32;
  std::map<std::string, DIS_DEVICE> DisDeviceCollection;
  std::map<std::string, DIS_DEVICE> DisAddDeviceCollection;
  std::map<std::string, DIS_DEVICE> DisDelDeviceCollection;
  DIS_CLIENT_DBG_SERVICE DisClientDbgService_X;
  DIS_SERVICE_PARAM DisServiceParam_X;

  DIS_CLIENT_NODE *pDisClientNode_X, DisClientNode_X;

  // Periodic thread started by LaunchBofProcessingThread 
  std::lock_guard Lock(mDisDeviceCollectionMtx);
  {
    DisDeviceCollection = mpuDisDiscovery->GetDisDeviceCollection();
    // Flag services which have disappeared
    for (const auto &rExistingItem : mDisDeviceCollection)
    {
      //S_Log("Chk %s %d\n", rExistingItem.first.c_str(), rExistingItem.second.MetaData_X.NodeId_U32);
      auto pItem = DisDeviceCollection.find(rExistingItem.first);
      if (pItem != DisDeviceCollection.end())
      {
//Device still present, copy Meta data added by DisClient::V_OnProcessing from mDisDeviceCollection in DisDeviceCollection
        pItem->second.MetaData_X.NodeId_U32 = rExistingItem.second.MetaData_X.NodeId_U32;
      }
      else
      {
        auto Iter = mDisClientDbgServiceCollection.find(rExistingItem.second.MetaData_X.DeviceUniqueKey_S);
        if (Iter != mDisClientDbgServiceCollection.end())
        {
          DisClientDbgService_X = std::move(Iter->second);
          DisDelDeviceCollection[rExistingItem.first] = rExistingItem.second;
        }
      }
    }
    // Check if new service have appeared
    for (const auto &rNewItem : DisDeviceCollection)
    {
      const auto &rItem = mDisDeviceCollection.find(rNewItem.first);
      if (rItem == mDisDeviceCollection.end())
      {
        DisAddDeviceCollection[rNewItem.first] = rNewItem.second;
      }
    }
    for (const auto &rItem : DisDelDeviceCollection)
    {
      pDisClientNode_X = mpuDiscoveryGraph->Node(rItem.second.MetaData_X.NodeId_U32);
      //S_Log("DEL %d %p\n", rItem.second.MetaData_X.NodeId_U32, pDisClientNode_X);
      if (pDisClientNode_X)
      {
        mpuDiscoveryGraph->EraseNode(pDisClientNode_X->NodeId_U32);
      }
      
      DisClientDbgService_X.puDisService->Stop();
      mDisClientDbgServiceCollection.erase(rItem.first); // destructor called ?
    }

//Is something has changed ?
    if ((DisAddDeviceCollection.size()) || (DisDelDeviceCollection.size()))
    {
      i_U32 = (DisDeviceCollection.size() > DisAddDeviceCollection.size()) ? DisDeviceCollection.size() - DisAddDeviceCollection.size() : 0;
      for (auto &rItem : DisAddDeviceCollection)
      {
        DisClientNode_X.Reset();
        DisClientNode_X.Name_S = rItem.second.MetaData_X.DeviceUniqueKey_S;
        DisClientNode_X.x_f = 200;
        DisClientNode_X.y_f = 40 + (i_U32 * 50);
        NodeId_U32 = mpuDiscoveryGraph->InsertNode(DisClientNode_X);
        BOF_ASSERT(NodeId_U32);
        auto IterDev = DisDeviceCollection.find(rItem.first);
        BOF_ASSERT(IterDev != DisDeviceCollection.end());
        IterDev->second.MetaData_X.NodeId_U32 = NodeId_U32;   //Will go in mDisDeviceCollection just after
        //S_Log("Add %d %p\n", NodeId_U32);

        pDisClientNode_X = mpuDiscoveryGraph->Node(NodeId_U32);
        BOF_ASSERT(pDisClientNode_X);
        pDisClientNode_X->NodeId_U32 = NodeId_U32;
        ImNodes::SetNodeEditorSpacePos(pDisClientNode_X->NodeId_U32, ImVec2(pDisClientNode_X->x_f, pDisClientNode_X->y_f));

        EdgeId_U32 = mpuDiscoveryGraph->InsertEdge(mGraphRootNodeId_U32, NodeId_U32);
        BOF_ASSERT(EdgeId_U32);

        DisServiceParam_X.Reset();
        DisServiceParam_X.DisServerEndpoint_S = "ws://" + rItem.second.IpAddress_S + ":8080";
        DisServiceParam_X.QueryServerPollTimeInMs_U32 = mDisClientParam_X.DisServerPollTimeInMs_U32;
        DisServiceParam_X.DisDevice_X = rItem.second;
        DisServiceParam_X.DisDevice_X.MetaData_X.NodeId_U32 = NodeId_U32;

        DisClientDbgService_X.IsVisisble_B = true;
        DisClientDbgService_X.PageIndex_S32 = DIS_CLIENT_INVALID_INDEX;
        DisClientDbgService_X.SubPageIndex_S32 = DIS_CLIENT_INVALID_INDEX;
        DisClientDbgService_X.puDisService = std::make_unique<DisService>(DisServiceParam_X);

        mDisClientDbgServiceCollection.emplace(std::make_pair(rItem.first, std::move(DisClientDbgService_X)));
        auto Iter = mDisClientDbgServiceCollection.find(rItem.first);
        BOF_ASSERT(Iter != mDisClientDbgServiceCollection.end());
        Iter->second.puDisService->Run();

        i_U32++;
      }
      mDisDeviceCollection = DisDeviceCollection;
    }
  } //std::lock_guard Lock(mDisDeviceCollectionMtx)
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