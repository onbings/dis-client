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

#include <bofstd/bofrawcircularbuffer.h>
#include <bofstd/bofthread.h>
#include <bofstd/bofgraph.h>
#include "bofimgui/bof_imgui.h"
#include "dis_service.h"
#include "dis_discovery.h"
// NODE
#include "bofimgui/imnodes.h"

constexpr int32_t DIS_CLIENT_INVALID_INDEX = -1;
struct DIS_CLIENT_PARAM
{
  uint32_t DiscoverPollTimeInMs_U32;              //Poll time to query discovered device
  uint32_t DisServerPollTimeInMs_U32;     //Poll time to ask debug info to Dis Server

  uint32_t FontSize_U32;
  uint32_t ConsoleWidth_U32;
  uint32_t ConsoleHeight_U32;
  BOF::BOF_IMGUI_PARAM ImguiParam_X;

  DIS_CLIENT_PARAM()
  {
    Reset();
  }
  void Reset()
  {
    DiscoverPollTimeInMs_U32 = 0;
    DisServerPollTimeInMs_U32 = 0;

    FontSize_U32 = 0;
    ConsoleWidth_U32 = 0;
    ConsoleHeight_U32 = 0;
    ImguiParam_X.Reset();
  }
};

struct DIS_CLIENT_DBG_SERVICE
{
  bool IsVisisble_B;
  std::unique_ptr<DisService> puDisService;
  int32_t ServiceIndex_S32;
  int32_t PageIndex_S32;
  int32_t SubPageIndex_S32;
  DIS_CLIENT_DBG_SERVICE()
  {
    Reset();
  }
  void Reset()
  {
    IsVisisble_B = false;
    puDisService = nullptr;
    ServiceIndex_S32 = DIS_CLIENT_INVALID_INDEX;
    PageIndex_S32 = DIS_CLIENT_INVALID_INDEX;
    SubPageIndex_S32 = DIS_CLIENT_INVALID_INDEX;
  }
};

struct DIS_CLIENT_NODE
{
  uint32_t NodeId_U32;
  float x_f;
  float y_f;

  //bool Input_B;
  float Value_f;

  DIS_CLIENT_NODE()
  {
    Reset();
  }
  void Reset()
  {
    NodeId_U32 = 0;
    x_f = 0.0f;
    y_f = 0.0f;
//    Input_B = false;
    Value_f = 0.0f;
  }
};

struct SIMULATOR_ENTRY
{
  bool DevicePresent_B;
  DIS_DEVICE DisDevice_X;

  SIMULATOR_ENTRY()
  {
    Reset();
  }
  void Reset()
  {
    DevicePresent_B = false;
    DisDevice_X.Reset();
  }
};
class DisClient : public BOF::Bof_ImGui, public BOF::BofThread
{
public:
  DisClient(const DIS_CLIENT_PARAM &_rDisClientParam_X);
  ~DisClient();
  static void S_Log(const char *_pFormat_c, ...);

private:
  BOFERR V_OnProcessing() override;
  BOFERR V_SaveSettings() override;
  BOFERR V_ReadSettings() override;
  void V_LoadAdditionalFonts() override;
  void V_PreNewFrame() override;
  BOFERR V_RefreshGui() override;
  void V_PostInit() override;
  void V_BeforeExit() override;

  BOFERR Run();
  BOFERR Stop();

  private:
  nlohmann::json ToJson() const;
  void FromJson(const nlohmann::json &_rJson);
  void DisplayDisService(int32_t _x_U32, int32_t _y_U32, DIS_CLIENT_DBG_SERVICE &_rDisClientDbgService_X, int32_t &_rDisServiceIndex_S32, int32_t &_rPageIndex_S32, int32_t &_rSubPageIndex_S32);
  void DisplayPageInfo(int32_t _x_U32, int32_t _y_U32, DIS_CLIENT_DBG_SERVICE &_rDisClientDbgService_X);
  void UpdateConsoleMenubar(DIS_CLIENT_DBG_SERVICE &_rDisClientDbgService_X);
  void DisplaySimulator(int32_t _x_U32, int32_t _y_U32);

private:
  BOFERR PrintAt(uint32_t _x_U32, uint32_t _y_U32, const std::string &_rHexaForeColor_S, const std::string &_rHexaBackColor_S, const std::string &_rText_S);

private:
  DIS_CLIENT_PARAM mDisClientParam_X;
  uint32_t mIdleTimer_U32 = 0;
  BOF::BOF_SIZE<uint32_t> mConsoleCharSize_X;

  std::vector<SIMULATOR_ENTRY> mSimulatorEntryCollection;

  std::unique_ptr<DisDiscovery> mpuDisDiscovery = nullptr;
  std::mutex mDisDeviceCollectionMtx;
  std::map<std::string, DIS_DEVICE> mDisDeviceCollection;

  std::mutex mDisClientDbgServiceCollectionMtx;
  std::map<std::string, DIS_CLIENT_DBG_SERVICE> mDisClientDbgServiceCollection;

  std::unique_ptr<BOF::BofDirGraph<DIS_CLIENT_NODE>> mpuNodeGraph = nullptr;
  std::vector<std::pair<uint32_t, uint32_t>> mLinkCollection;
  ImNodesMiniMapLocation mMinimapLocation = ImNodesMiniMapLocation_BottomRight;
};
