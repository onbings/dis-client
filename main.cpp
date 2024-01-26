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
//opt/evs/evs-gbio/bin/dis_service --DisServer=ws://10.129.171.112:8080
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
  StdParam_X.AssertCallback = nullptr;
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

    DisClientParam_X.PollTimeInMs_U32 = 1000;
    DisClientParam_X.FontSize_U32 = 14;
    DisClientParam_X.ConsoleWidth_U32 = 80;
    DisClientParam_X.ConsoleHeight_U32 = 25;
    DisClientParam_X.ImguiParam_X.WindowTitle_S = "Dis-Client";
    DisClientParam_X.ImguiParam_X.Size_X = BOF::BOF_SIZE<uint32_t>(1600, 900);  // 800, 600);
    DisClientParam_X.ImguiParam_X.TheLogger = DisClient::S_Log;
    DisClientParam_X.ImguiParam_X.ShowDemoWindow_B = true;
    std::unique_ptr<DisClient> puDisClient = std::make_unique<DisClient>(DisClientParam_X);
    Rts_i = (int)puDisClient->MainLoop();
    BOF::Bof_Shutdown();
  }
  return Rts_i;
}
