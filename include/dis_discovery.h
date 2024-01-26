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
#include <bofstd/bofguid.h>

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

struct DIS_SERVICE_DISCOVERED
{
  std::string Name_S;
  std::string IpAddress_S;
  DIS_SERVICE_DISCOVERED()
  {
    Reset();
  }
  void Reset()
  {
    Name_S = "";
    IpAddress_S = "";
  }
};

class DisDiscovery:public BOF::BofThread
{
public:
  DisDiscovery(const DIS_DISCOVERY_PARAM &_rDisDiscoveryParam_X);
  ~DisDiscovery();

  BOFERR Run();
  BOFERR Stop();
  const std::map<BOF::BofGuid, DIS_SERVICE_DISCOVERED> GetDisDiscoveryServiceCollection();

private:
  BOFERR V_OnProcessing() override;

private:
  DIS_DISCOVERY_PARAM mDisDiscoveryParam_X;

  std::mutex mDisDiscoveryServiceCollectionMtx;
  std::map<BOF::BofGuid,DIS_SERVICE_DISCOVERED> mDisDiscoveryServiceCollection;
};
