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
  DIS_SERVICE_DISCOVERED DisServiceDiscovered_X;

  DisServiceDiscovered_X.IpAddress_S = "10.129.171.112";
  DisServiceDiscovered_X.Name_S = "Xt1_DisService";
  mDisDiscoveryServiceCollection[BOF::BofGuid()] =  DisServiceDiscovered_X;
/* TODO inc et dec mDisDiscoveryServiceCollection content to test
  DisServiceDiscovered_X.IpAddress_S = "10.129.171.112";
  DisServiceDiscovered_X.Name_S = "Xt2_DisService";
  mDisDiscoveryServiceCollection[BOF::BofGuid()] =  DisServiceDiscovered_X;

  DisServiceDiscovered_X.IpAddress_S = "10.129.171.112";
  DisServiceDiscovered_X.Name_S = "Xt3_DisService";
  mDisDiscoveryServiceCollection[BOF::BofGuid()] =  DisServiceDiscovered_X;
  */
#endif
}

DisDiscovery::~DisDiscovery()
{
  Stop();
}

const std::map<BOF::BofGuid, DIS_SERVICE_DISCOVERED> DisDiscovery::GetDisDiscoveryServiceCollection()
{
  std::lock_guard Lock(mDisDiscoveryServiceCollectionMtx);
  return mDisDiscoveryServiceCollection;
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
  std::lock_guard Lock(mDisDiscoveryServiceCollectionMtx);
  {
    //Do discover
  }
  //while ((!IsThreadLoopMustExit()) && (Rts_E == BOF_ERR_NO_ERROR));
  return Rts_E;
}