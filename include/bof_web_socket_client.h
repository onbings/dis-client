/*
 * Copyright (c) 2023-2033, Onbings. All rights reserved.
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * This module defines a web socket client
 *
 * History:
 *
 * V 1.00  Jan 4 2023  Bernard HARMEL: onbings@gmail.com : Initial release
 */
#pragma once

#include "bofstd.h"
#include <thread>
#include <functional>

BEGIN_BOF_NAMESPACE()

enum class WebSocketClientState
{
  WS_CLIENT_STATE_INIT,
  WS_CLIENT_STATE_OPEN,
  WS_CLIENT_STATE_ERROR,
  WS_CLIENT_STATE_CLOSE
};

class WebSocketClient
{
private:
  static uint32_t S_mSeqId_U32;

  std::string mpUrl_S;
  WebSocketClientState mState_E;
  bool mWaitForCmdReply_B=false;
  std::string mReply_S;
  uint32_t mReplyId_U32=0;
  std::thread mClientThread;
  bool mConnected_B=false;

  std::function<void()> mOnOpen;
  std::function<void(const std::string &)> mOnRx;
  std::function<void(const std::string &)> mOnClose;
  std::function<void(const std::string &)> mOnError;

  void DoConnect(const std::string &_rUrl_S);

public:
  WebSocketClient(bool _Verbose_B, const std::function<void()> &_rOnOpen, const std::function<void(const std::string &)> &_rOnRx,
                  const std::function<void(const std::string &)> &_rOnClose, const std::function<void(const std::string &)> &_rOnError);
  WebSocketClientState GetClientState();
  BOFERR SendMessage(const std::string &_rMessage_S);
  BOFERR SendCommand(uint32_t _TimeoutInMs_U32, const std::string &_rCmd_S, std::string &_rReply_S);
  std::string WsUrl();
  bool IsConnected();
  BOFERR Connect(int timeout_in_ms, const std::string &url);
  BOFERR Disconnect();
  void OnOpen();
  void OnMessage(const std::string &message);
  void OnClose(const std::string &close_msg);
  void OnError(const std::string &error);
};

END_BOF_NAMESPACE()
