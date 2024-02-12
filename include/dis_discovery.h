/*
 * Copyright (c) 2023-2033, Onbings. All rights reserved.
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * This module defines a dis discovery process
 *
 * History:
 *
 * V 1.00  Jan 25 2024  Bernard HARMEL: b.harmel@evs.com : Initial release
 */
#pragma once
#include <string>
#include <map>
#include <bofstd/bofthread.h>

#define DIS_DEVICE_BUILD_UNIQUE_NAME(pDeviceUniqueKey, Type, Sn, pName)     \
  {                                                                         \
    if (pDeviceUniqueKey)                                                   \
    {                                                                       \
      if ((Type) && (Sn) && (pName) && (pName[0]) && (strlen(pName) <= 64)) \
        sprintf(pDeviceUniqueKey, "%08X_%08X_%s", Type, Sn, pName);         \
      else                                                                  \
        strcpy(pDeviceUniqueKey, "?BadPrm?");                               \
    }                                                                       \
  }
struct DIS_DISCOVERY_PARAM
{
  uint32_t PollTimeInMs_U32;

  DIS_DISCOVERY_PARAM()
  {
    Reset();
  }
  void Reset()
  {
    PollTimeInMs_U32 = 0;
  }
};

// DIS_DEVICE unique name is build as %08X_%08X_%s Type_U32, Sn_U32, Name_S.
// This unique name is used as primary key in std::map (cf DIS_DEVICE_BUILD_UNIQUE_NAME macro)
struct DIS_DEVICE
{
  uint32_t Type_U32;
  uint32_t Sn_U32;
  std::string Name_S;
  std::string IpAddress_S;
  DIS_DEVICE()
  {
    Reset();
  }
  void Reset()
  {
    Type_U32 = 0;
    Sn_U32 = 0;
    Name_S = "";
    IpAddress_S = "";
  }
};

class DisDiscovery : public BOF::BofThread
{
public:
  DisDiscovery(const DIS_DISCOVERY_PARAM &_rDisDiscoveryParam_X);
  ~DisDiscovery();

  BOFERR Run();
  BOFERR Stop();
  std::map<std::string, DIS_DEVICE> GetDisDeviceCollection(); // Return a COPY of the discovered devices

  BOFERR Simul_AddDevice(const DIS_DEVICE &_rDevice_X);
  BOFERR Simul_RemoveDevice(const DIS_DEVICE &_rDevice_X);

private:
  BOFERR V_OnProcessing() override;

private:
  DIS_DISCOVERY_PARAM mDisDiscoveryParam_X;

  std::mutex mDisDeviceCollectionMtx;
  std::map<std::string, DIS_DEVICE> mDisDeviceCollection;
};
