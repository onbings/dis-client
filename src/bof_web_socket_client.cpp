/*
 * Copyright (c) 2023-2033, Onbings. All rights reserved.
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * This module implementsa web socket client
 *
 * History:
 *
 * V 1.00  Jan 4 2023  Bernard HARMEL: onbings@gmail.com : Initial release
 */
#include "bof_web_socket_client.h"
#include <nlohmann/json.hpp>

BEGIN_BOF_NAMESPACE()

uint32_t Bof_GetMsTickCount()
{
  uint64_t NbMs_U64 = std::chrono::steady_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
  return static_cast<uint32_t>(NbMs_U64);
}
void Bof_MsSleep(uint32_t _Ms_U32)
{
    if (_Ms_U32 == 0)
    {
      std::this_thread::yield();
    }
    else
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(_Ms_U32));
  }
}
uint32_t Bof_ElapsedMsTime(uint32_t _StartInMs_U32)
{
  uint32_t Rts_U32;

  Rts_U32 = Bof_GetMsTickCount() - _StartInMs_U32; // Unsigned arithmetic makes it works in any case even if Now_U64<_Start_U64
  return Rts_U32;
}
uint32_t WebSocketClient::S_mSeqId_U32 = 1;

WebSocketClient::WebSocketClient(bool _Verbose_B, const std::function<void()> &_rOnOpen, const std::function<void(const std::string &)> &_rOnRx,
                                 const std::function<void(const std::string &)> &_rOnClose, const std::function<void(const std::string &)> &_rOnError)
                : mState_E(WebSocketClientState::WS_CLIENT_STATE_INIT), mOnOpen(_rOnOpen),  mOnRx(_rOnRx),  mOnClose(_rOnClose),  mOnError(_rOnError)
{
}

WebSocketClientState WebSocketClient::GetClientState()
{
  return mState_E;
}

BOFERR WebSocketClient::SendMessage(const std::string &_rMessage_S)
{
  BOFERR Rts_E = BOF_ERR_INIT;

  if (mState_E == WebSocketClientState::WS_CLIENT_STATE_OPEN && !mWaitForCmdReply_B)
  {
    // Replace this with the actual WebSocket sending logic
    printf("Sending message: %s\n", _rMessage_S.c_str());
    Rts_E=BOF_ERR_NO_ERROR;
  }
  return Rts_E;
}

BOFERR WebSocketClient::SendCommand(uint32_t _TimeoutInMs_U32, const std::string &_rCmd_S, std::string &_rReply_S)
{
  BOFERR Rts_E = BOF_ERR_INVALID_STATE;
  uint32_t Start_U32;
  std::string Command_S;
  nlohmann::json JsonData;

  if (mState_E == WebSocketClientState::WS_CLIENT_STATE_OPEN)
  {
    Start_U32 = BOF::Bof_GetMsTickCount();
    mReply_S.clear();
    mWaitForCmdReply_B = true;
    mReplyId_U32 = S_mSeqId_U32;
    BOF_INC_TICKET_NUMBER(S_mSeqId_U32);

    Command_S = _rCmd_S + "?seq=" + std::to_string(mReplyId_U32);
    // Replace this with the actual WebSocket sending logic
    printf("Send seq '%d' cmd '%s' \n", mReplyId_U32, Command_S.c_str());

    Rts_E = BOF_ERR_EAGAIN;
    while ((mState_E == WebSocketClientState::WS_CLIENT_STATE_OPEN) && (mWaitForCmdReply_B))
    {
      if (!mWaitForCmdReply_B)
      {
        Rts_E = BOF_ERR_NO_ERROR;
      }
      else
      {
        if (BOF::Bof_ElapsedMsTime(Start_U32) >= _TimeoutInMs_U32)
        {
          Rts_E = BOF_ERR_ETIMEDOUT;
          mWaitForCmdReply_B = false;
        }
        BOF::Bof_MsSleep(1);
      }
    }

    printf("Final check: state '%d' wait '%d' replay '%s'\n", static_cast<int>(mState_E), mWaitForCmdReply_B, mReply_S.c_str());

    if (Rts_E == BOF_ERR_NO_ERROR)
    {
      try
      {
        JsonData = nlohmann::json::parse(mReply_S);
        auto It = JsonData.find("protocolInfo");
        if (It != JsonData.end())
        {
          protocol_info = *It
          # Parse the URL
          parsed_http_request = urlparse(protocol_info)
          #Extract components
          #scheme = parsed_http_request.scheme
          #netloc = parsed_http_request.netloc
          #path = parsed_http_request.path
          query = parsed_http_request.query
          #Parse the query string into a dictionary
          query_param = parse_qs(query)
          # Get the value of the 'seq' parameter
          seq_value = int(query_param.get("seq", [None])[0])
          if self.reply_id == seq_value:
        return True, self.reply
          else:
        print(f"ProtocolInfo seq of '{protocol_info}' ({seq_value}) is not equal to seq of '{cmd}' ({self.reply_id})")

      }
      catch (nlohmann::json::type_error &re)
      {
        Rts_E = BOF_ERR_INVALID_ANSWER;
        printf("FromJson: exception caught '%s'\n", re.what());
      }
    }
    else
    {
      printf("State '%d' is not correct.\n",static_cast<int>(mState_E));
    }
  }

  mWaitForCmdReply_B = false;
  return {false, ""};
}

std::string WebSocketClient::ws_url()
{
  return mpUrl_S;
}

bool WebSocketClient::is_connected()
{
  return mConnected_B;
}

BOFERR WebSocketClient::connect(int timeout_in_ms, const std::string &url)
{
  BOFERR Rts_E;
  this->mpUrl_S = url;
  mClientThread = std::thread(&WebSocketClient::_do_connect, this, url);

  auto Start_U32 = BOF::Bof_GetMsTickCount();
  while (mState_E != WebSocketClientState::WS_CLIENT_STATE_OPEN)
  {
    Rts_E = BOF_ERR_NO_ERROR;
    if (BOF::Bof_ElapsedMsTime(Start_U32) >= 5000)
    {
      printf("Connection timeout.\n");
      Rts_E = BOF_ERR_ETIMEDOUT;
      break;
    }
    BOF::Bof_MsSleep(100);
  }
  return Rts_E;

}

BOFERR WebSocketClient::disconnect()
{
  BOFERR Rts_E=BOF_ERR_INTERNAL;
  // Replace this with the actual WebSocket closing logic
  printf("Disconnecting...\n");
  // ws.close();
  if (mClientThread.joinable())
  {
    mClientThread.join();
    Rts_E = BOF_ERR_NO_ERROR;
  }
  return Rts_E;
}

void WebSocketClient::on_open()
{
  printf("Connection opened\n");
  mState_E = WebSocketClientState::WS_CLIENT_STATE_OPEN;
  mConnected_B = true;
  if (mOnOpen)
  {
    mOnOpen();
  }
}

void WebSocketClient::on_message(const std::string &message)
{
  if (mWaitForCmdReply_B)
  {
    mReply_S = message;
    printf("Received reply: %s\n", message.c_str());
    mWaitForCmdReply_B = false;
  }
  else
  {
    printf("Received message: %s\n", message.c_str());
    if (mOnRx)
    {
      mOnRx(message);
    }
  }
}

void WebSocketClient::on_close(const std::string &close_msg)
{
  printf("Connection closed\n");
  mState_E = WebSocketClientState::WS_CLIENT_STATE_CLOSE;
  mReply_S.clear();
  mWaitForCmdReply_B = true;
  mConnected_B = false;
  if (mCbClose)
  {
    mCbClose(close_msg);
  }
}

void WebSocketClient::on_error(const std::string &error)
{
  printf("Error: %s\n", error.c_str());
  mState_E = WebSocketClientState::WS_CLIENT_STATE_ERROR;
  if (mOnError)
  {
    mOnError(error);
  }
}

void WebSocketClient::_do_connect(const std::string &url)
{
  printf("Thread starts\n");

  // Replace this with the actual WebSocket setup logic
  // websocketpp::client<websocketpp::config::asio> ws_client;
  // ws_client.set_message_handler(bind(&WebSocketClient::on_message, this, std::placeholders::_1, std::placeholders::_2));

  // Replace this with the actual WebSocket run logic
  // ws_client.run();
  printf("Thread leaves\n");
}

int main2()
{
  WebSocketClient wsClient(
    true,
    []() { printf("Callback: Connection opened.\n"); },
    [](const std::string &message) { printf("Callback: Received message: %s\n", message.c_str()); },
    [](const std::string &close_msg) { printf("Callback: Connection closed: %s\n", close_msg.c_str()); },
    [](const std::string &error) { printf("Callback: Error: %s\n", error.c_str()); });

  wsClient.connect(1000,"ws://127.0.0.1:8080");
  wsClient.SendMessage("Hello, WebSocket!");
  auto result = wsClient.send_command(5000, "your_command");
  if (std::get<0>(result))
  {
    printf("Command succeeded. Reply: %s\n", std::get<1>(result).c_str());
  }
  else
  {
    printf("Command failed or timed out.\n");
  }
  wsClient.disconnect();

  return 0;
}

END_BOF_NAMESPACE()