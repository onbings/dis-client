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

DisDiscovery::DisDiscovery(const DIS_DISCOVERY_PARAM &_rDisDiscoveryParam_X)
             :BofThread()
{
  mDisDiscoveryParam_X = _rDisDiscoveryParam_X;
#if defined(NDEBUG) // We are in Release compil
#else
  DIS_DEVICE DisDevice_X;

  DisDevice_X.IpAddress_S = "10.129.171.112";
  DisDevice_X.Name_S = "Xt1_DisDevice";
  mDisDeviceCollection[BOF::BofGuid()] =  DisDevice_X;
/* TODO inc et dec mDisDeviceCollection content to test
  DisDevice_X.IpAddress_S = "10.129.171.112";
  DisDevice_X.Name_S = "Xt2_DisDevice";
  mDisDeviceCollection[BOF::BofGuid()] =  DisDevice_X;

  DisDevice_X.IpAddress_S = "10.129.171.112";
  DisDevice_X.Name_S = "Xt3_DisDevice";
  mDisDeviceCollection[BOF::BofGuid()] =  DisDevice_X;
  */
#endif
}

DisDiscovery::~DisDiscovery()
{
  Stop();
}

//No return a COPY and not a ref const std::map<BOF::BofGuid, DIS_DEVICE> &DisDiscovery::GetDisDeviceCollection()
std::map<BOF::BofGuid, DIS_DEVICE> DisDiscovery::GetDisDeviceCollection()
{
  std::lock_guard Lock(mDisDeviceCollectionMtx);
  return mDisDeviceCollection;
}

BOFERR DisDiscovery::Run()
{
  BOFERR Rts_E;

  Rts_E = LaunchBofProcessingThread("DisDiscovery", false, mDisDiscoveryParam_X.PollTimeInMs_U32, BOF::BOF_THREAD_SCHEDULER_POLICY::BOF_THREAD_SCHEDULER_POLICY_OTHER,
                                    BOF::BOF_THREAD_PRIORITY::BOF_THREAD_PRIORITY_000,0, 2000, 0);
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
//Periodic thread in  LaunchBofProcessingThread do
  std::lock_guard Lock(mDisDeviceCollectionMtx);
  {
    //Do discover
  }
  //while ((!IsThreadLoopMustExit()) && (Rts_E == BOF_ERR_NO_ERROR));
  return Rts_E;
}