/*!
  Copyright (c) 2008, EVS. All rights reserved.

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND,  EITHER EXPRESSED OR IMPLIED,  INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  History:
  V 1.00  Sep 4 2023	  BHA : Initial release for Gige/ppc integration
*/
#pragma once
#include <webrpc/websocket.h>

class DisClientWebSocket : public WEBRPC::WebSocket
{
public:
  DisClientWebSocket(const WEBRPC::WEB_SOCKET_PARAM &_rWebSocketParam_X);
  ~DisClientWebSocket();

private:
  //BOFERR V_WebSocketCallback(WEB_SOCKET_LWS_CALLBACK_REASON _Reason_E, uint32_t _Nb_U32, uint8_t *_pData_U8, BOF::BOF_BUFFER &_rReply_X) override;
private:
  WEBRPC::WEB_SOCKET_PARAM mWebSocketParam_X;
};

