/*!
  Copyright (c) 2008, EVS. All rights reserved.

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND,  EITHER EXPRESSED OR IMPLIED,  INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  History:
    V 1.00  August 24 2013  BHA : Original version
*/
#include <bofstd/boffs.h>

#include "bofwebrpc/bofgrpc.h"
#include <grpcpp/impl/codegen/interceptor.h>


BEGIN_WEBRPC_NAMESPACE()
/*
GRPC with post man:
Msg:
{
    "channel": 0,
    "flag": 3,
    "key": 854235486,
    "page": 800,
    "sub_page": 0,
    "user_input": "ad quis commodo"
}
ip 127.0.0.1:2510
3247618: PRE_SEND_INITIAL_METADATA
3247618: PRE_SEND_MESSAGE
3247619: PRE_SEND_STATUS
3247619: POST_SEND_MESSAGE
3247621: POST_RECV_CLOSE
*/

// Grpc 'ipv6:%5B::1%5D:51465' appears to be an IPv6 address in URL-encoded format.

// This is a simple interceptor that logs whenever it gets a request, which on
// the server side happens when initial metadata is received.

void GrpcInterceptor::Intercept(grpc::experimental::InterceptorBatchMethods *pMethod)
{
  if (pMethod->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_SEND_INITIAL_METADATA))
  {
    GRPC_DBG_LOG("%d: PRE_SEND_INITIAL_METADATA\n", BOF::Bof_GetMsTickCount());
  }
  if (pMethod->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_SEND_MESSAGE))
  {
    GRPC_DBG_LOG("%d: PRE_SEND_MESSAGE\n", BOF::Bof_GetMsTickCount());
  }
  if (pMethod->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::POST_SEND_MESSAGE))
  {
    GRPC_DBG_LOG("%d: POST_SEND_MESSAGE\n", BOF::Bof_GetMsTickCount());
  }
  if (pMethod->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_SEND_STATUS))
  {
    GRPC_DBG_LOG("%d: PRE_SEND_STATUS\n", BOF::Bof_GetMsTickCount());
  }
  if (pMethod->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_SEND_CLOSE)) // Client
  {
    GRPC_DBG_LOG("%d: PRE_SEND_CLOSE\n", BOF::Bof_GetMsTickCount());
  }
  if (pMethod->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_RECV_INITIAL_METADATA))
  {
    GRPC_DBG_LOG("%d: PRE_RECV_INITIAL_METADATA\n", BOF::Bof_GetMsTickCount());
  }
  if (pMethod->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_RECV_MESSAGE))
  {
    GRPC_DBG_LOG("%d: PRE_RECV_MESSAGE\n", BOF::Bof_GetMsTickCount());
  }
  if (pMethod->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_RECV_STATUS))
  {
    GRPC_DBG_LOG("%d: PRE_RECV_STATUS\n", BOF::Bof_GetMsTickCount());
  }
  if (pMethod->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::POST_RECV_INITIAL_METADATA))
  {
    GRPC_DBG_LOG("%d: POST_RECV_INITIAL_METADATA\n", BOF::Bof_GetMsTickCount());
    std::multimap<grpc::string_ref, grpc::string_ref> *pGetRecvInitialMetadata = pMethod->GetRecvInitialMetadata();
    for (const auto &pair : *pGetRecvInitialMetadata)
    {
      //GRPC_DBG_LOG("%d: POST_RECV_INITIAL_METADATA %s=%s\n", BOF::Bof_GetMsTickCount(), pair.first, pair.second);
    }
  }
  if (pMethod->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::POST_RECV_MESSAGE))
  {
    GRPC_DBG_LOG("%d: POST_RECV_MESSAGE\n", BOF::Bof_GetMsTickCount());
  }
  if (pMethod->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::POST_RECV_STATUS))
  {
    GRPC_DBG_LOG("%d: POST_RECV_STATUS\n", BOF::Bof_GetMsTickCount());
  }
  if (pMethod->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::POST_RECV_CLOSE))
  {
    GRPC_DBG_LOG("%d: POST_RECV_CLOSE\n", BOF::Bof_GetMsTickCount());
  }
  if (pMethod->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_SEND_CANCEL))
  {
    GRPC_DBG_LOG("%d: PRE_SEND_CANCEL\n", BOF::Bof_GetMsTickCount());
  }

  pMethod->Proceed();
}


grpc::experimental::Interceptor *GrpcInterceptorFactory::CreateServerInterceptor(grpc::experimental::ServerRpcInfo *pInfo)
{
  return new GrpcInterceptor();
}



END_WEBRPC_NAMESPACE()