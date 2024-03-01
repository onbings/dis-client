/*
 * Copyright (c) 2023-2033, Onbings. All rights reserved.
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * This module implements a dis client app using emscripten and imgui
 *
 * History:
 *
 * V 1.00  Dec 24 2023  Bernard HARMEL: onbings@gmail.com : Initial release
 *
 * See data flow in comment at the end of this dis_client.cpp file
 *
 */
#include "dis_client.h"
#include <bofstd/bofsystem.h>
#include <bofstd/bofguid.h>
// opt/evs/evs-gbio/bin/dis_service --DisServer=ws://10.129.171.112:8080
#if defined(__EMSCRIPTEN__)
#include <filesystem>
BOFERR EmscriptenCallback(void *_pArg)
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;
  int i, Len_i;
  FILE *pIo_X;
  char pTxt_c[0x100], pFn_c[] = "/offline/any_file.bha";

  const std::filesystem::directory_iterator end;
  for (std::filesystem::directory_iterator it("/"); it != end; ++it)
  {
    const std::filesystem::path &entry = it->path();
    printf("root: %s\n", entry.c_str());
  }

  pIo_X = fopen(pFn_c, "rb");
  if (pIo_X)
  {
    for (i = 0; i < 10; i++)
    {
      if (fgets(pTxt_c, sizeof(pTxt_c), pIo_X) == nullptr)
      {
        break;
      }
      else
      {
        printf("[%d] '%s'\n", i, pTxt_c);
      }
    }
    fclose(pIo_X);
  }
  else
  {
    pIo_X = fopen(pFn_c, "wb");
    if (pIo_X)
    {
      for (i = 0; i < 10; i++)
      {
        Len_i = sprintf(pTxt_c, "This is line %d\n", i);
        if (fwrite(pTxt_c, Len_i, 1, pIo_X) != 1)
        {
          break;
        }
      }
      fclose(pIo_X);
    }
  }
  for (std::filesystem::directory_iterator it("/offline"); it != end; ++it)
  {
    const std::filesystem::path &entry = it->path();
    printf("offline: %s\n", entry.c_str());
  }
  Rts_E = BOF_ERR_FINISHED;
  return Rts_E;
}
#endif

// #define TEST_WEBSOCKET
#if defined(TEST_WEBSOCKET)
#define DIS_CLIENT_MAIN_LOOP_IDLE_TIME 100                          // 100ms We no state machine action during this time (keep imgui main loop cpu friendly)
#define DIS_CLIENT_WS_TIMEOUT (DIS_CLIENT_MAIN_LOOP_IDLE_TIME * 10) // 1000ms Timeout fo ws connect, read or write op
#define DIS_SERVICE_STATE_TIMEOUT (DIS_CLIENT_WS_TIMEOUT * 3)       // 3000ms Maximum time in a given state before returning to DIS_SERVICE_STATE_OPEN_WS
#define DIS_CLIENT_RX_BUFFER_SIZE 0x40000
#define DIS_CLIENT_NB_MAX_BUFFER_ENTRY 32
#define DIS_CLIENT_END_OF_COMMAND(pDisService)     \
  {                                                \
    (pDisService)->LastCmdSentTimeoutInMs_U32 = 0; \
    (pDisService)->WaitForReplyId_U32 = 0;         \
  }
#define BOF_LOG_TO_DBG(pFormat, ...)              \
  {                                               \
    std::string Dbg;                              \
    Dbg = BOF::Bof_Sprintf(pFormat, ##__VA_ARGS__); \
    OutputDebugString(Dbg.c_str());               \
  }

class DisTest
{
private:
  std::unique_ptr<DisClientWebSocket> mpuDisClientWebSocket = nullptr;
  std::string mUniqueName_S;
  WS_CALLBBACK_PARAM mWsCbParam_X;
  DIS_DBG_SERVICE mDisDbgService_X;

public:
  DisTest()
  {
    BOFWEBRPC::BOF_WEB_SOCKET_PARAM WebSocketParam_X;
    BOFERR Sts_E;
    mUniqueName_S = BOF::BofGuid().ToString(true);
    mWsCbParam_X.pDisService = (DisService *)this;
    mWsCbParam_X.pDisService_X = &mDisDbgService_X;
    WebSocketParam_X.pUser = &mWsCbParam_X;
    WebSocketParam_X.NbMaxOperationPending_U32 = 4;
    WebSocketParam_X.RxBufferSize_U32 = DIS_CLIENT_RX_BUFFER_SIZE;
    WebSocketParam_X.NbMaxBufferEntry_U32 = DIS_CLIENT_NB_MAX_BUFFER_ENTRY;
    WebSocketParam_X.OnOperation = nullptr;
    WebSocketParam_X.OnOpen = BOF_BIND_1_ARG_TO_METHOD(this, DisTest::OnWebSocketOpenEvent);
    WebSocketParam_X.OnClose = BOF_BIND_1_ARG_TO_METHOD(this, DisTest::OnWebSocketCloseEvent);
    WebSocketParam_X.OnError = BOF_BIND_2_ARG_TO_METHOD(this, DisTest::OnWebSocketErrorEvent);
    WebSocketParam_X.OnMessage = BOF_BIND_4_ARG_TO_METHOD(this, DisTest::OnWebSocketMessageEvent);
    WebSocketParam_X.NbMaxClient_U32 = 0; // 0: client
    // WebSocketParam_X.ServerIp_X = BOF::BOF_SOCKET_ADDRESS(mDisClientParam_X.DisServerEndpoint_S);
    WebSocketParam_X.WebSocketThreadParam_X.Name_S = mUniqueName_S; // mainly usefull for mpuDisClientWebSocket->Connect
    WebSocketParam_X.WebSocketThreadParam_X.ThreadSchedulerPolicy_E = BOF::BOF_THREAD_SCHEDULER_POLICY::BOF_THREAD_SCHEDULER_POLICY_OTHER;
    WebSocketParam_X.WebSocketThreadParam_X.ThreadPriority_E = BOF::BOF_THREAD_PRIORITY::BOF_THREAD_PRIORITY_000;
    mpuDisClientWebSocket = std::make_unique<DisClientWebSocket>(WebSocketParam_X);
    mpuDisClientWebSocket->Run();
  }
  bool IsConnected()
  {
    return mDisDbgService_X.IsWebSocketConnected_B;
  }
  BOFERR OpenWebSocket(const std::string &_rIpAddress_S)
  {
    BOFERR Rts_E = BOF_ERR_INIT;
    if (mpuDisClientWebSocket)
    {
      Rts_E = mpuDisClientWebSocket->ResetWebSocketOperation();
      Rts_E = mpuDisClientWebSocket->Connect(DIS_CLIENT_WS_TIMEOUT, _rIpAddress_S, mUniqueName_S); // mUniqueName_S must be different for each connection
    }
    return Rts_E;
  }
  BOFERR CloseWebSocket()
  {
    BOFERR Rts_E = BOF_ERR_INIT;
    if (mpuDisClientWebSocket)
    {
      Rts_E = mpuDisClientWebSocket->Disconnect(DIS_CLIENT_WS_TIMEOUT);
    }
    return Rts_E;
  }

  BOFERR OnWebSocketOpenEvent(void *_pWsCbParam)
  {
    BOFERR Rts_E = BOF_ERR_INTERNAL;
    WS_CALLBBACK_PARAM *pWsCbParam_X = (WS_CALLBBACK_PARAM *)_pWsCbParam;

    if ((pWsCbParam_X) && (pWsCbParam_X->pDisService) && (pWsCbParam_X->pDisService_X))
    {
      pWsCbParam_X->pDisService_X->IsWebSocketConnected_B = true;
      Rts_E = BOF_ERR_NO_ERROR;
    }
    return Rts_E;
  }
  BOFERR OnWebSocketErrorEvent(void *_pWsCbParam, BOFERR _Sts_E)
  {
    BOFERR Rts_E = BOF_ERR_INTERNAL;
    WS_CALLBBACK_PARAM *pWsCbParam_X = (WS_CALLBBACK_PARAM *)_pWsCbParam;

    if ((pWsCbParam_X) && (pWsCbParam_X->pDisService) && (pWsCbParam_X->pDisService_X))
    {
      Rts_E = CloseWebSocket();
      pWsCbParam_X->pDisService_X->Ws = reinterpret_cast<uintptr_t>(pWsCbParam_X->pDisService_X);
      Rts_E = BOF_ERR_NO_ERROR;
    }
    return Rts_E;
  }
  BOFERR OnWebSocketCloseEvent(void *_pWsCbParam)
  {
    BOFERR Rts_E = BOF_ERR_INTERNAL;
    WS_CALLBBACK_PARAM *pWsCbParam_X = (WS_CALLBBACK_PARAM *)_pWsCbParam;

    if ((pWsCbParam_X) && (pWsCbParam_X->pDisService) && (pWsCbParam_X->pDisService_X))
    {
      pWsCbParam_X->pDisService_X->IsWebSocketConnected_B = false;
      pWsCbParam_X->pDisService_X->Ws = -1;
      DIS_CLIENT_END_OF_COMMAND(pWsCbParam_X->pDisService_X);
      Rts_E = BOF_ERR_NO_ERROR;
    }
    return Rts_E;
  }
  BOFERR OnWebSocketMessageEvent(void *_pWsCbParam, uint32_t _Nb_U32, uint8_t *_pData_U8, BOF::BOF_BUFFER &_rReply_X)
  {
    BOFERR Rts_E = BOF_ERR_INTERNAL;
    WS_CALLBBACK_PARAM *pWsCbParam_X = (WS_CALLBBACK_PARAM *)_pWsCbParam;
    BOF::BOF_RAW_BUFFER *pRawBuffer_X;
    bool FinalFragment_B;

    if ((pWsCbParam_X) && (pWsCbParam_X->pDisService) && (pWsCbParam_X->pDisService_X) && (_pData_U8))
    {
      pWsCbParam_X->pDisService_X->puRxBufferCollection->SetAppendMode(0, true, nullptr); // By default we append
      FinalFragment_B = true;
      Rts_E = pWsCbParam_X->pDisService_X->puRxBufferCollection->PushBuffer(0, _Nb_U32, _pData_U8, &pRawBuffer_X);
      pWsCbParam_X->pDisService_X->puRxBufferCollection->SetAppendMode(0, !FinalFragment_B, &pRawBuffer_X); // If it was the final one, we stop append->Result is pushed and committed
      // DisClient::S_Log("err %d data %d:%p Rts:indx %d slot %x 1: %x:%p 2: %x:%p\n", Rts_E, _Nb_U32, _pData_U8, pRawBuffer_X->IndexInBuffer_U32, pRawBuffer_X->SlotEmpySpace_U32, pRawBuffer_X->Size1_U32, pRawBuffer_X->pData1_U8, pRawBuffer_X->Size2_U32, pRawBuffer_X->pData2_U8);
      if (Rts_E != BOF_ERR_NO_ERROR)
      {
        CloseWebSocket();
      }
    }
    return Rts_E;
  }
};
#endif
BOFERR AppBofAssertCallback(const std::string &_rFile_S, uint32_t _Line_U32, const std::string &_rMasg_S)
{
  printf("Assert in %s line %d Msg %s\n", _rFile_S.c_str(), _Line_U32, _rMasg_S.c_str());
  return BOF_ERR_NO_ERROR;
}

int main(int _Argc_i, char *_pArgv[])
{
  int Rts_i;
  BOF::BOFSTDPARAM StdParam_X;
  DIS_CLIENT_PARAM DisClientParam_X;
  std::string Cwd_S;
  /*
  https://floooh.github.io/2023/11/11/emscripten-ide.html
  Breakpoints in WebAssembly code are resolved asynchronously, so breakpoints hit early on in a program’s lifecycle may be missed. There are plans to fix this in the future. If you’re debugging in a browser, you can refresh the page for your breakpoint to be hit. If you’re in Node.js, you can add an artificial delay, or set another breakpoint, after your WebAssembly module is loaded but before your desired breakpoint is hit.
  Hopefully this problem will be fixed soon-ish, since it’s currently the most annoying.
  One workaround is to first set a breakpoint in the Javascript launch file at a point where the WASM blob has been loaded.
  Load the file localhost:8080/binaries/bin/dis-client.js into the editor, search the function callMain, and set a breakpoint there.

  To debug bof fct, put brk on the call in c++, step in javascript, in getWasmTableEntry the func object contains the path to the source file of the bof fct:
  C:\pro\vcpkg\buildtrees\bofstd\src\08de758d7f-37ed795cab.clean\lib\src\bofsystem.cpp

 function invoke_vi(index,a1) {
  var sp = stackSave();
  try {
    getWasmTableEntry(index)(a1); ------------------------------->var getWasmTableEntry = (funcPtr) => {
  } catch(e) {                                                      var func = wasmTableMirror[funcPtr];
    stackRestore(sp);
    if (!(e instanceof EmscriptenEH)) throw e;
    _setThrew(1, 0);
  }
}
  */
  BOF::Bof_MsSleep(1000); // Breakpoints in WebAssembly code are resolved asynchronously See above comment
  StdParam_X.AssertInRelease_B = true;
  StdParam_X.AssertCallback = AppBofAssertCallback;
#if 0
#if defined(__EMSCRIPTEN__)
#if defined(IMGUI_IMPL_API)
// HelloImGui defines and uses its own personal mainloop (emscripten_set_main_loop_arg)
#else
  StdParam_X.EmscriptenCallback = EmscriptenCallback;
#endif
  StdParam_X.EmscriptenCallbackFps_U32 = 0;
  StdParam_X.pEmscriptenCallbackArg = (void *)0x12345678;
  StdParam_X.pPersistentRootDir_c = "/offline";
  StdParam_X.ExitOnBofShutdown_B = true;
#endif
#endif
  if (Bof_Initialize(StdParam_X) == BOF_ERR_NO_ERROR)
  {
    // BOF::Bof_GetCurrentDirectory(Cwd_S);
    printf("\nPwd %s\nRunning BofStd V %s on %s under %s\n", Cwd_S.c_str(), StdParam_X.Version_S.c_str(), StdParam_X.ComputerName_S.c_str(), StdParam_X.OsName_S.c_str());

#if defined(TEST_WEBSOCKET)
    DisTest DisTest1, DisTest2;
    BOFERR Sts_E;
    uint32_t i_U32;

    for (i_U32 = 0; i_U32 < 10; i_U32++)
    {
      BOF_LOG_TO_DBG("=%d====>Initial State %s/%s\n", i_U32, DisTest1.IsConnected() ? "Connected" : "Disconnected", DisTest2.IsConnected() ? "Connected" : "Disconnected");
      if ((DisTest1.IsConnected()) || (DisTest2.IsConnected()))
      {
        BOF_LOG_TO_DBG("jj");
      }
      BOF_LOG_TO_DBG("=%d====>Open first=========================\n", i_U32);
      Sts_E = DisTest1.OpenWebSocket("ws://10.129.171.112:8080");
      if (Sts_E == BOF_ERR_NO_ERROR)
      {
        BOF_LOG_TO_DBG("=%d====>Open second=========================\n", i_U32);
        Sts_E = DisTest2.OpenWebSocket("ws://10.129.171.112:8080");
      }
      BOF_LOG_TO_DBG("=%d====>OpenWebSocket Sts %s State %s/%s\n", i_U32, BOF::Bof_ErrorCode(Sts_E), DisTest1.IsConnected() ? "Connected" : "Disconnected", DisTest2.IsConnected() ? "Connected" : "Disconnected");
      if ((!DisTest1.IsConnected()) || (!DisTest2.IsConnected()))
      {
        BOF_LOG_TO_DBG("jj");
      }
      Sts_E = DisTest1.CloseWebSocket();
      if (Sts_E == BOF_ERR_NO_ERROR)
      {
        Sts_E = DisTest2.CloseWebSocket();
      }
      BOF_LOG_TO_DBG("=%d====>CloseWebSocket Sts %s State %s\n", i_U32, BOF::Bof_ErrorCode(Sts_E), DisTest1.IsConnected() ? "Connected" : "Disconnected", DisTest2.IsConnected() ? "Connected" : "Disconnected");
      if ((DisTest1.IsConnected()) || (DisTest2.IsConnected()))
      {
        BOF_LOG_TO_DBG("jj");
      }
    }
#endif

    DisClientParam_X.DiscoverPollTimeInMs_U32 = 2000;
    DisClientParam_X.DisServerPollTimeInMs_U32 = 1000;
    DisClientParam_X.FontSize_U32 = 24;
    DisClientParam_X.ConsoleWidth_U32 = 80;
    DisClientParam_X.ConsoleHeight_U32 = 25;
    DisClientParam_X.ImguiParam_X.WindowTitle_S = "Dis-Client";
    DisClientParam_X.ImguiParam_X.Size_X = BOF::BOF_SIZE<uint32_t>(1600, 900); // 800, 600);
    // DisClientParam_X.ImguiParam_X.BackgroudHexaColor_S = "#00FF0050";
    DisClientParam_X.ImguiParam_X.TheLogger = DisClient::S_Log;
    // DisClientParam_X.ImguiParam_X.ShowDemoWindow_B = true;
    DisClientParam_X.ImguiParam_X.ShowMenuBar_B = false;
    DisClientParam_X.ImguiParam_X.ShowStatusBar_B = false;
    std::unique_ptr<DisClient> puDisClient = std::make_unique<DisClient>(DisClientParam_X);
    Rts_i = (int)puDisClient->MainLoop();
    BOF::Bof_Shutdown();
  }
  return Rts_i;
}
