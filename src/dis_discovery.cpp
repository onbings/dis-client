/*
 * Copyright (c) 2023-2033, Onbings. All rights reserved.
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * This module implement a dis discovery process
 *
 * History:
 *
 * V 1.00  Jan 25 2024  Bernard HARMEL: b.harmel@evs.com : Initial release
 */
#include "dis_discovery.h"
#include <bofstd/bofsocketos.h>

DisDiscovery::DisDiscovery(const DIS_DISCOVERY_PARAM &_rDisDiscoveryParam_X)
    : BofThread()
{
  mDisDiscoveryParam_X = _rDisDiscoveryParam_X;
}

DisDiscovery::~DisDiscovery()
{
  Stop();
}

// No return a COPY and not a ref const std::map<std::string, DIS_DEVICE> &DisDiscovery::GetDisDeviceCollection()
std::map<std::string, DIS_DEVICE> DisDiscovery::GetDisDeviceCollection()
{
  std::lock_guard Lock(mDisDeviceCollectionMtx);
  return mDisDeviceCollection;
}

BOFERR DisDiscovery::AddDevice(DIS_DEVICE &_rDisDevice_X)
{
  BOFERR Rts_E = BOF_ERR_EINVAL;
  BOF::BOF_SOCKET_ADDRESS Ip_X(_rDisDevice_X.IpAddress_S);
  char pDeviceUniqueKey_c[256];

  if ((_rDisDevice_X.Type_U32) && (_rDisDevice_X.Sn_U32) && (_rDisDevice_X.Name_S != "") && (Ip_X.Valid_B))
  {
    DIS_DEVICE_BUILD_UNIQUE_NAME(pDeviceUniqueKey_c, _rDisDevice_X.Type_U32, _rDisDevice_X.Sn_U32, _rDisDevice_X.Name_S.c_str());
    std::lock_guard Lock(mDisDeviceCollectionMtx);
    // Error cannot happens but 'paranoid'
    // Forwe accept to have two times the same device ip address but with a different DIS_DEVICE_BUILD_UNIQUE_NAME and for the moment Name_S must also be unique...
    Rts_E = BOF_ERR_NO_ERROR;
    for (auto &rIt : mDisDeviceCollection)
    {
      if ((rIt.first == pDeviceUniqueKey_c) || (rIt.second.Name_S == _rDisDevice_X.Name_S))
      {
        Rts_E = BOF_ERR_DUPLICATE;
        break;
      }
    }
    if (Rts_E == BOF_ERR_NO_ERROR)
    {
      _rDisDevice_X.MetaData_X.DeviceUniqueKey_S = pDeviceUniqueKey_c;  //Put the key in data to avoid re-computation when we need it
      mDisDeviceCollection[pDeviceUniqueKey_c] = _rDisDevice_X;
    }
  }
  return Rts_E;
}
BOFERR DisDiscovery::RemoveDevice(const DIS_DEVICE &_rDisDevice_X)
{
  BOFERR Rts_E = BOF_ERR_INTERNAL;

  std::lock_guard Lock(mDisDeviceCollectionMtx);
  auto Iter = mDisDeviceCollection.find(_rDisDevice_X.MetaData_X.DeviceUniqueKey_S);
  if (Iter != mDisDeviceCollection.end())
  {
    mDisDeviceCollection.erase(Iter);
    Rts_E = BOF_ERR_NO_ERROR;
  }
  return Rts_E;
}
BOFERR DisDiscovery::Run()
{
  BOFERR Rts_E;

  Rts_E = LaunchBofProcessingThread("DisDiscovery", false, mDisDiscoveryParam_X.PollTimeInMs_U32, BOF::BOF_THREAD_SCHEDULER_POLICY::BOF_THREAD_SCHEDULER_POLICY_OTHER,
                                    BOF::BOF_THREAD_PRIORITY::BOF_THREAD_PRIORITY_000, 0, 2000, 0);
  return Rts_E;
}
BOFERR DisDiscovery::Stop()
{
  BOFERR Rts_E;

  Rts_E = DestroyBofProcessingThread(nullptr);
  return Rts_E;
}

BOFERR DisDiscovery::V_OnProcessing()
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;
  // Periodic thread in  LaunchBofProcessingThread do
  std::lock_guard Lock(mDisDeviceCollectionMtx);
  {
    // Do discover
  }
  // while ((!IsThreadLoopMustExit()) && (Rts_E == BOF_ERR_NO_ERROR));
  return Rts_E;
}