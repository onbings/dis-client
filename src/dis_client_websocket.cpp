/*!
  Copyright (c) 2008, EVS. All rights reserved.

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND,  EITHER EXPRESSED OR IMPLIED,  INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  History:
  V 1.00  Sep 4 2023	  BHA : Initial release for Gige/ppc integration
*/
#include "dis_client_websocket.h"

DisClientWebSocket::DisClientWebSocket(const BOFWEBRPC::BOF_WEB_SOCKET_PARAM &_rWebSocketParam_X)
    : BOFWEBRPC::BofWebSocket(_rWebSocketParam_X)
{
  mWebSocketParam_X = _rWebSocketParam_X;
}

DisClientWebSocket::~DisClientWebSocket()
{
}

