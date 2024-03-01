/*
 * Copyright (c) 2023-2033, Onbings. All rights reserved.
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * This module implements the bof module interface to imgui and helloimgui lib
 *
 * History:
 *
 * V 1.00  Dec 24 2023  Bernard HARMEL: onbings@gmail.com : Initial release
 */
#include "bofimgui/bof_imgui.h"
#include "bofimgui/bof_imgui_internal.h"
#include <bofstd/bofstring.h>

#if defined(_WIN32)
#include <ShellScalingApi.h>
#endif

BEGIN_BOF_NAMESPACE()

void LogHelper(Logger &_rLogFunction, const char *_pFormat_c, ...)
{
  char pBuffer_c[0x1000];
  va_list Args;
  va_start(Args, _pFormat_c);
  vsnprintf(pBuffer_c, sizeof(pBuffer_c), _pFormat_c, Args);
  va_end(Args);
  _rLogFunction(pBuffer_c);
}

// DBG_LOG macro for conditionally logging messages
#define DBG_LOG(fmt, ...)                                   \
  do                                                        \
  {                                                         \
    if (mImguiParam_X.TheLogger)                            \
    {                                                       \
      LogHelper(mImguiParam_X.TheLogger, fmt, ##__VA_ARGS__); \
    }                                                       \
  } while (0)

Bof_ImGui::Bof_ImGui(const BOF_IMGUI_PARAM &_rImguiParam_X)
{
  mImguiParam_X = _rImguiParam_X;
  mLastError_E = BOF_ERR_NO_ERROR;
}
Bof_ImGui::~Bof_ImGui()
{
}
void Bof_ImGui::S_BuildHelpMarker(const char *_pHelp_c)
{
  ImGui::TextDisabled("(?)");
  if (ImGui::BeginItemTooltip())
  {
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(_pHelp_c);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}
// BOFERR Bof_ImGui::DisplayText(const char *_pTextStart_c, const char *_pTextEnd_c, bool _Wrapped_B, bool _Disabled_B, const Bof_ImGui_ImTextCustomization *_pTextCustomization_X)
BOFERR Bof_ImGui::DisplayText(const char *_pText_c, bool _Wrapped_B, bool _Disabled_B, const Bof_ImGui_ImTextCustomization *_pTextCustomization_X)
{
  BOFERR Rts_E = mLastError_E;
  if (Rts_E == BOF_ERR_NO_ERROR)
  {
    Rts_E = DisplayText(nullptr, _pText_c, _Wrapped_B, _Disabled_B, _pTextCustomization_X);
    if (Rts_E == BOF_ERR_NO_ERROR)
    {
      ImGui::NewLine();
    }
  }
  return Rts_E;
}

BOFERR Bof_ImGui::DisplayText(BOF::BOF_POINT_2D<int32_t> *_pCursorPos_X, const char *_pText_c, bool _Wrapped_B, bool _Disabled_B, const Bof_ImGui_ImTextCustomization *_pTextCustomization_X)
{
  BOFERR Rts_E = mLastError_E;
  bool NeedBackup_B;
  Bof_ImGui_ImTextCustomization TextCustomization_X;

  if (Rts_E == BOF_ERR_NO_ERROR)
  {
    ImGuiContext &rGuiContext = *GImGui;
    if (_pCursorPos_X)
    {
      ImGui::SetCursorPos(ImVec2((float)_pCursorPos_X->x, (float)_pCursorPos_X->y));
    }
    NeedBackup_B = (rGuiContext.CurrentWindow->DC.TextWrapPos < 0.0f); // Keep existing wrap position if one is already set
    if (NeedBackup_B && _Wrapped_B)
    {
      ImGui::PushTextWrapPos(0.0f);
    }
    if (_Disabled_B)
    {
      ImGui::PushStyleColor(ImGuiCol_Text, rGuiContext.Style.Colors[ImGuiCol_TextDisabled]);
    }
    ImGuiTextEx(_pText_c, nullptr, ImGuiTextFlags_NoWidthForLargeClippedText, _pTextCustomization_X);
    if (_Disabled_B)
    {
      ImGui::PopStyleColor();
    }
    if (NeedBackup_B && _Wrapped_B)
    {
      ImGui::PopTextWrapPos();
    }
  }
  return Rts_E;
}

std::string Bof_ImGui::S_GetKeyboardState()
{
  std::string Rts_S;

  uint32_t i_U32;
  static const char *S_pKeyNameCollection_c[] = {
      "Tab ", "Left ", "Right ", "Up ", "Down ", "PgUp ", "PgDown ", "Home ", "End ", "Ins ", "Del ",
      "Back ", "Space ", "Enter ", "Esc ", "LCtrl ", "LShift ", "LAlt ", "LSup ", "RCtrl ", "RShift ", "RAlt ", "RSup ", "Menu ",
      "0 ", "1 ", "2 ", "3 ", "4 ", "5 ", "6 ", "7 ", "8 ", "9 ",
      "A ", "B ", "C ", "D ", "E ", "F ", "G ", "H ", "I ", "J ",
      "K ", "L ", "M ", "N ", "O ", "P ", "Q ", "R ", "S ", "T ",
      "U ", "V ", "W ", "X ", "Y ", "Z ",
      "F1 ", "F2 ", "F3 ", "F4 ", "F5 ", "F6 ", "F7 ", "F8 ", "F9 ", "F10 ",
      "F11 ", "F12 ", "F13 ", "F14 ", "F15 ", "F16 ", "F17 ", "F18 ", "F19 ", "F20 ",
      "F21 ", "F22 ", "F23 ", "F24 ",
      "' ", ", ", "- ", ". ", "/ ", "; ", "= ", "[ ", "\\ ", "] ", "` ", "Caps ", "Lock", "Num ", "Prt ", "Hold ",
      "N0 ", "N1 ", "N2 ", "N3 ", "N4 ", "N5 ", "N6 ", "N7 ", "N8 ", "N9 ",
      "N. ", "N/ ", "N* ", "N- ", "N+ ", "NEnter ", "N= "};
  for (i_U32 = ImGuiKey_Tab; i_U32 <= ImGuiKey_KeypadEqual; i_U32++)
  {
    if (ImGui::IsKeyPressed((ImGuiKey)i_U32, true))
    {
      Rts_S += S_pKeyNameCollection_c[i_U32 - ImGuiKey_Tab];
      // printf("rts %s\n", Rts_S.c_str());
    }
  }
  // char pKeyCombi_c[0x100];
  // ImGui::GetKeyChordName(ImGuiKeyChord key_chord, pKeyCombi_c, sizeof(pKeyCombi_c));

  return Rts_S;
}
void MyGui()
{
  ImGui::Text("Hello, world");
  if (ImGui::Button("Exit"))
  {
    HelloImGui::GetRunnerParams()->appShallExit = true;
  }
}
BOFERR Bof_ImGui::MainLoop()
{
  BOFERR Rts_E = mLastError_E;
  HelloImGui::RunnerParams RunnerParam_X;
  uint8_t pColor_U8[4];
  // emscripten_set_main_loop_arg(emscripten_imgui_main_loop, NULL, 0, true);
  if (Rts_E == BOF_ERR_NO_ERROR)
  {
    mShowDemoWindow_B = mImguiParam_X.ShowDemoWindow_B;

    // HelloImGui::SimpleRunnerParams simpleRunnerParams;
    // simpleRunnerParams.guiFunction = MyGui;
    // simpleRunnerParams.windowSizeAuto = true;
    // simpleRunnerParams.windowTitle = "Example";
    // HelloImGui::Run(simpleRunnerParams);
    // return Rts_E;
    /* MobileCallbacks **is a struct that contains callbacks that are called by the application when running under "Android, iOS and WinRT".
    #ifdef HELLOIMGUI_MOBILEDEVICE
      if (params.callbacks.mobileCallbacks.OnResume)
        params.callbacks.mobileCallbacks.OnResume();
    #endif
    */
#if 1
    RunnerParam_X.callbacks.SetupImGuiConfig = BOF_BIND_0_ARG_TO_METHOD(this, Bof_ImGui::V_SetupImGuiConfig);
    RunnerParam_X.callbacks.SetupImGuiStyle = BOF_BIND_0_ARG_TO_METHOD(this, Bof_ImGui::V_SetupImGuiStyle);
    RunnerParam_X.callbacks.PostInit = BOF_BIND_0_ARG_TO_METHOD(this, Bof_ImGui::V_PostInit);
    RunnerParam_X.callbacks.LoadAdditionalFonts = BOF_BIND_0_ARG_TO_METHOD(this, Bof_ImGui::V_LoadAdditionalFonts);
    RunnerParam_X.callbacks.PreNewFrame = BOF_BIND_0_ARG_TO_METHOD(this, Bof_ImGui::V_PreNewFrame);
    // If callback not defined, call Impl_Frame_3D_ClearColor(); with RunnerParam_X.imGuiWindowParams.backgroundColor (BackgroudHexaColor_S)
    if (S_HexaColor(mImguiParam_X.BackgroudHexaColor_S, pColor_U8))
    {
      RunnerParam_X.imGuiWindowParams.backgroundColor = ImVec4((float)pColor_U8[0] / 255.0f, (float)pColor_U8[1] / 255.0f, (float)pColor_U8[2] / 255.0f, (float)pColor_U8[3] / 255.0f);
    }
    else
    {
      RunnerParam_X.callbacks.CustomBackground = BOF_BIND_0_ARG_TO_METHOD(this, Bof_ImGui::V_CustomBackground);
    }
    RunnerParam_X.callbacks.CustomBackground = nullptr;
    RunnerParam_X.callbacks.ShowMenus = BOF_BIND_0_ARG_TO_METHOD(this, Bof_ImGui::V_ShowMenus);
    RunnerParam_X.callbacks.ShowStatus = BOF_BIND_0_ARG_TO_METHOD(this, Bof_ImGui::V_ShowStatus);
    RunnerParam_X.callbacks.ShowGui = BOF_BIND_0_ARG_TO_METHOD(this, Bof_ImGui::V_ShowGui);
    RunnerParam_X.callbacks.BeforeImGuiRender = BOF_BIND_0_ARG_TO_METHOD(this, Bof_ImGui::V_BeforeImGuiRender);
    RunnerParam_X.callbacks.AfterSwap = BOF_BIND_0_ARG_TO_METHOD(this, Bof_ImGui::V_AfterSwap);
    RunnerParam_X.callbacks.AnyBackendEventCallback = BOF_BIND_1_ARG_TO_METHOD(this, Bof_ImGui::V_AnyBackendEventCallback);
    RunnerParam_X.callbacks.ShowAppMenuItems = BOF_BIND_0_ARG_TO_METHOD(this, Bof_ImGui::V_ShowAppMenuItems);
    RunnerParam_X.callbacks.BeforeExit = BOF_BIND_0_ARG_TO_METHOD(this, Bof_ImGui::V_BeforeExit);
    RunnerParam_X.callbacks.BeforeExit_PostCleanup = BOF_BIND_0_ARG_TO_METHOD(this, Bof_ImGui::V_BeforeExit_PostCleanup);
    RunnerParam_X.callbacks.RegisterTests = BOF_BIND_0_ARG_TO_METHOD(this, Bof_ImGui::V_RegisterTests);

    RunnerParam_X.appWindowParams.windowTitle = mImguiParam_X.WindowTitle_S;
    RunnerParam_X.appWindowParams.windowGeometry.size[0] = mImguiParam_X.Size_X.Width;
    RunnerParam_X.appWindowParams.windowGeometry.size[1] = mImguiParam_X.Size_X.Height;
    // If true, adapt the app window size to the presented widgets. This is done at startup
    ////RunnerParam_X.appWindowParams.windowGeometry.sizeAuto = true;
    // If true, the application window will try to resize based on its content on the next displayed frame
    ////RunnerParam_X.appWindowParams.windowGeometry.resizeAppWindowAtNextFrame = true;

    RunnerParam_X.appWindowParams.windowGeometry.fullScreenMode = HelloImGui::FullScreenMode::NoFullScreen;
    RunnerParam_X.appWindowParams.windowGeometry.positionMode = HelloImGui::WindowPositionMode::MonitorCenter; // OsDefault;
    // used if windowPositionMode==FromCoords, default=(40, 40)
    RunnerParam_X.appWindowParams.windowGeometry.position = HelloImGui::DefaultScreenPosition;
    // used if positionMode==MonitorCenter or if fullScreenMode!=NoFullScreen
    RunnerParam_X.appWindowParams.windowGeometry.monitorIdx = 0;
    RunnerParam_X.appWindowParams.windowGeometry.windowSizeState = HelloImGui::WindowSizeState::Standard;
    RunnerParam_X.appWindowParams.windowGeometry.windowSizeMeasureMode = HelloImGui::WindowSizeMeasureMode::RelativeTo96Ppi;
    // if true, then save & restore from last run
    RunnerParam_X.appWindowParams.restorePreviousGeometry = false;
    RunnerParam_X.appWindowParams.borderless = false;
    RunnerParam_X.appWindowParams.resizable = true;
    RunnerParam_X.appWindowParams.hidden = false;

    RunnerParam_X.appWindowParams.edgeInsets.top = 0;
    RunnerParam_X.appWindowParams.edgeInsets.left = 0;
    RunnerParam_X.appWindowParams.edgeInsets.bottom = 0;
    RunnerParam_X.appWindowParams.edgeInsets.right = 0;
    RunnerParam_X.appWindowParams.handleEdgeInsets = true;

    RunnerParam_X.imGuiWindowParams.defaultImGuiWindowType = HelloImGui::DefaultImGuiWindowType::ProvideFullScreenDockSpace;
    // Above at if (S_HexaColor(mImguiParam_X.BackgroudHexaColor_S, pColor_U8)):  RunnerParam_X.imGuiWindowParams.backgroundColor = ImVec4(1.f, 0.f, 0.f, 0.f);
    //  In order to fully customize the menu, set showMenuBar to true, and set showMenu_App and showMenu_View params to false.Then, implement the
    //  callback `RunnerParams.callbacks.ShowMenus` which can optionally call `HelloImGui::ShowViewMenu` and `HelloImGui::ShowAppMenu`.
    RunnerParam_X.imGuiWindowParams.showMenuBar = mImguiParam_X.ShowMenuBar_B;
    RunnerParam_X.imGuiWindowParams.showMenu_App = false;
    RunnerParam_X.imGuiWindowParams.showMenu_View = false;
    // Set this to false if you intend to provide your own quit callback with possible user confirmation(and implement it inside RunnerCallbacks.ShowAppMenuItems)
    RunnerParam_X.imGuiWindowParams.showMenu_App_Quit = true;
    RunnerParam_X.imGuiWindowParams.showStatusBar = mImguiParam_X.ShowStatusBar_B;
    RunnerParam_X.imGuiWindowParams.showStatus_Fps = true;
    RunnerParam_X.imGuiWindowParams.rememberStatusBarSettings = false;
    RunnerParam_X.imGuiWindowParams.configWindowsMoveFromTitleBarOnly = false;
    RunnerParam_X.imGuiWindowParams.enableViewports = true;
    RunnerParam_X.imGuiWindowParams.menuAppTitle = mImguiParam_X.WindowTitle_S;
    RunnerParam_X.imGuiWindowParams.tweakedTheme.Theme = ImGuiTheme::ImGuiTheme_DarculaDarker;

    // Common rounding for widgets. If < 0, this is ignored.
    RunnerParam_X.imGuiWindowParams.tweakedTheme.Tweaks.Rounding = -1.f;
    // If rounding is applied, scrollbar rounding needs to be adjusted to be visually pleasing in conjunction with other widgets roundings. Only applied if Rounding > 0.f)
    RunnerParam_X.imGuiWindowParams.tweakedTheme.Tweaks.RoundingScrollbarRatio = 4.f;
    // Change the alpha that will be applied to windows, popups, etc. If < 0, this is ignored.
    RunnerParam_X.imGuiWindowParams.tweakedTheme.Tweaks.AlphaMultiplier = -1.f;
    // HSV Color tweaks
    // Change the hue of all widgets (gray widgets will remain gray, since their saturation is zero). If < 0, this is ignored.
    RunnerParam_X.imGuiWindowParams.tweakedTheme.Tweaks.Hue = -1.f;
    // Multiply the saturation of all widgets (gray widgets will remain gray, since their saturation is zero). If < 0, this is ignored.
    RunnerParam_X.imGuiWindowParams.tweakedTheme.Tweaks.SaturationMultiplier = -1.f;
    // Multiply the value (luminance) of all front widgets. If < 0, this is ignored.
    RunnerParam_X.imGuiWindowParams.tweakedTheme.Tweaks.ValueMultiplierFront = -1.f;
    // Multiply the value (luminance) of all backgrounds. If < 0, this is ignored.
    RunnerParam_X.imGuiWindowParams.tweakedTheme.Tweaks.ValueMultiplierBg = -1.f;
    // Multiply the value (luminance) of text. If < 0, this is ignored.
    RunnerParam_X.imGuiWindowParams.tweakedTheme.Tweaks.ValueMultiplierText = -1.f;
    // Multiply the value (luminance) of FrameBg. If < 0, this is ignored.
    // (Background of checkbox, radio button, plot, slider, text input)
    RunnerParam_X.imGuiWindowParams.tweakedTheme.Tweaks.ValueMultiplierFrameBg = -1.f;

    RunnerParam_X.imGuiWindowParams.showMenu_View_Themes = true;
    RunnerParam_X.imGuiWindowParams.rememberTheme = true;

    // RunnerParam_X.dockingParams.dockingSplits;
    //  RunnerParam_X.dockingParams.dockableWindows;
    //  RunnerParam_X.dockingParams.layoutName = "Default";
    //  RunnerParam_X.dockingParams.layoutCondition = DockingLayoutCondition::FirstUseEver;
    //  RunnerParam_X.dockingParams.layoutReset = false;
    //  RunnerParam_X.dockingParams.mainDockSpaceNodeFlags = ImGuiDockNodeFlags_PassthruCentralNode;
    //  RunnerParam_X.dockingParams.dockableWindowOfName(const std::string &name);
    //  RunnerParam_X.dockingParams.focusDockableWindow(const std::string &windowName);
    //  RunnerParam_X.dockingParams.dockSpaceIdFromName(const std::string &dockSpaceName);
    RunnerParam_X.rememberSelectedAlternativeLayout = false;

    // These pointers will be filled when the application starts, and you can use them to customize your application behavior using the selected backend.
    // RunnerParam_X.backendPointers;
    RunnerParam_X.platformBackendType = HelloImGui::PlatformBackendType::FirstAvailable;
    RunnerParam_X.rendererBackendType = HelloImGui::RendererBackendType::FirstAvailable;
    RunnerParam_X.fpsIdling.enableIdling = true;
    RunnerParam_X.useImGuiTestEngine = false;

    RunnerParam_X.iniFolderType = HelloImGui::IniFolderType::CurrentFolder;
    RunnerParam_X.iniFilename = ""; // relative to iniFolderType
    RunnerParam_X.iniFilename_useAppWindowTitle = true;
    RunnerParam_X.appShallExit = false;
    RunnerParam_X.emscripten_fps = 0;
#endif
    HelloImGui::Run(RunnerParam_X);

    // HelloImGui::SimpleRunnerParams SimpleRunnerParam_X;
    // SimpleRunnerParam_X.guiFunction = BOF_BIND_0_ARG_TO_METHOD(this, Bof_ImGui::V_ShowGui);
    // HelloImGui::Run(SimpleRunnerParam_X);
  }
  return Rts_E;
}

BOFERR Bof_ImGui::SetCursorPos(int32_t _x_S32, int32_t _y_S32)
{
  BOFERR Rts_E = mLastError_E;

  if (Rts_E == BOF_ERR_NO_ERROR)
  {
    ImGui::SetCursorPos(ImVec2((float)_x_S32, (float)_y_S32));
  }
  return Rts_E;
}
BOFERR Bof_ImGui::SetNextWindowSize(uint32_t _Width_U32, uint32_t _Height_U32)
{
  BOFERR Rts_E = mLastError_E;

  if (Rts_E == BOF_ERR_NO_ERROR)
  {
    ImGui::SetNextWindowSize(ImVec2((float)_Width_U32, (float)_Height_U32), ImGuiCond_FirstUseEver);
  }
  return Rts_E;
}
std::vector<BOF_IMGUI_FONT> Bof_ImGui::GetFontList()
{
  std::vector<BOF_IMGUI_FONT> Rts;
  BOF_IMGUI_FONT Font_X;
  ImFontAtlas *pFontAtlas_X = ImGui::GetIO().Fonts;
  ImFont *pFont_X;
  uint32_t i_U32;

  // Get the font atlas and iterate through the available fonts
  for (i_U32 = 0; i_U32 < pFontAtlas_X->Fonts.Size; ++i_U32)
  {
    pFont_X = pFontAtlas_X->Fonts[i_U32];
    Font_X.Name_S = pFont_X->ConfigData ? pFont_X->ConfigData[0].Name : "Unknown";
    Font_X.Size_U32 = pFont_X->FontSize;
    Rts.push_back(Font_X);
    // Print information about each font
    DBG_LOG("Font %d: Name - %s, Size - %.2f\n", i_U32, pFont_X->ConfigData ? pFont_X->ConfigData[0].Name : "Unknown", pFont_X->FontSize);
  }
  return Rts;
}
ImFont *Bof_ImGui::GetFont(uint32_t _FontIndex_U32)
{
  ImFont *pRts = nullptr;

  if (mLastError_E == BOF_ERR_NO_ERROR)
  {
    pRts = ImGui::GetIO().Fonts->Fonts[_FontIndex_U32];
  }
  return pRts;
}
ImFont *Bof_ImGui::LoadFont(const char *_pFontFileTtf_c, uint32_t _FontSizeInPixel_U32)
{
  ImFont *pRts = nullptr;

  if ((mLastError_E == BOF_ERR_NO_ERROR) && (_pFontFileTtf_c) && (_FontSizeInPixel_U32))
  {
    pRts = ImGui::GetIO().Fonts->AddFontFromFileTTF(_pFontFileTtf_c, _FontSizeInPixel_U32, nullptr, nullptr);
  }
  return pRts;
}
BOF::BOF_SIZE<uint32_t> Bof_ImGui::GetTextSize(const char *_pText_c)
{
  BOF::BOF_SIZE<uint32_t> Rts_X;

  if (mLastError_E == BOF_ERR_NO_ERROR)
  {
    ImVec2 TextSize_X = ImGui::CalcTextSize(_pText_c);
    Rts_X.Width = (uint32_t)TextSize_X.x;
    Rts_X.Height = (uint32_t)TextSize_X.y;
  }
  return Rts_X;
}
bool Bof_ImGui::S_HexaColor(const std::string &_rHexaColor_S, uint8_t (&_rColor_U8)[4])
{
  bool Rts_B = false;
  uint32_t Rgba_U32;

  if (_rHexaColor_S[0] == '#')
  {
    if (_rHexaColor_S.size() == (1 + 6))
    {
      Rgba_U32 = (uint32_t)BOF::Bof_StrToBin(16, _rHexaColor_S.substr(1).c_str());
      _rColor_U8[0] = (uint8_t)(Rgba_U32 >> 16);
      _rColor_U8[1] = (uint8_t)(Rgba_U32 >> 8);
      _rColor_U8[2] = (uint8_t)(Rgba_U32);
      _rColor_U8[3] = 255;
      Rts_B = true;
    }
    else if (_rHexaColor_S.size() == (1 + 8))
    {
      Rgba_U32 = (uint32_t)BOF::Bof_StrToBin(16, _rHexaColor_S.substr(1).c_str());
      _rColor_U8[0] = (uint8_t)(Rgba_U32 >> 24);
      _rColor_U8[1] = (uint8_t)(Rgba_U32 >> 16);
      _rColor_U8[2] = (uint8_t)(Rgba_U32 >> 8);
      _rColor_U8[3] = (uint8_t)(Rgba_U32);
      Rts_B = true;
    }
  }
  return Rts_B;
}
void Bof_ImGui::HandleComputerKeyboard()
{
  uint32_t i_U32;
  ImWchar Ch;
  char Ch_c;
  std::string KeyState_S;

  KeyState_S = S_GetKeyboardState();
  if (!ImGui::IsAnyItemFocused() && !ImGui::IsAnyItemActive())
  {
    ImGui::SetKeyboardFocusHere();
  }
  ImGui::Dummy(ImVec2(0, 0)); // This is needed to make the input text field capture keyboard input

  auto &rIo = ImGui::GetIO();
  rIo.WantCaptureKeyboard = true;
  if (rIo.InputQueueCharacters.Size > 0)
  {
    for (i_U32 = 0; i_U32 < rIo.InputQueueCharacters.Size; i_U32++)
    {
      Ch = rIo.InputQueueCharacters[i_U32];
      Ch_c = static_cast<char>(Ch);
      V_OnKeyboardPress(Ch_c, KeyState_S);
    }
    // Consume characters
    rIo.InputQueueCharacters.resize(0);
  }
#if 0
  if (ImGui::IsKeyPressed(ImGuiKey_Backspace))
  {
    // calculatorState.OnComputerKey('\b');
  }
  if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Enter))
  {
    // calculatorState.OnComputerKey('\n');
  }
#endif
}
void Bof_ImGui::V_OnKeyboardPress(char _Ch_c, const std::string &_rKeyState_S)
{
  DBG_LOG("V_OnKeyboardPress ch %02X St %s\n", _Ch_c, _rKeyState_S.c_str());
}

void Bof_ImGui::V_SetupImGuiConfig()
{
  DBG_LOG("V_SetupImGuiConfig\n", 0);
#if defined(_WIN32)
  SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
#endif
  HelloImGui::ImGuiDefaultSettings::SetupDefaultImGuiConfig(); // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; ...
}
void Bof_ImGui::V_SetupImGuiStyle()
{
  DBG_LOG("V_SetupImGuiStyle\n", 0);
  HelloImGui::ImGuiDefaultSettings::SetupDefaultImGuiStyle(); // ImGui::StyleColorsDark(); ...
}
void Bof_ImGui::V_PostInit()
{
  // This should be done before Impl_SetupPlatformRendererBindings()
  // because, in the case of glfw ImGui_ImplGlfw_InstallCallbacks
  // will chain the user callbacks with ImGui callbacks; and PostInit()
  // is a good place for the user to install callbacks
  DBG_LOG("V_PostInit Dear ImGui version: %s\n", ImGui::GetVersion());
  V_ReadSettings();
}
void Bof_ImGui::V_LoadAdditionalFonts()
{
  DBG_LOG("V_LoadAdditionalFonts\n", 0);
  HelloImGui::ImGuiDefaultSettings::LoadDefaultFont_WithFontAwesomeIcons();
}
void Bof_ImGui::V_PreNewFrame()
{
  //  DBG_LOG("V_PreNewFrame\n", 0); // Before ImGui::NewFrame();
}
void Bof_ImGui::V_CustomBackground()
{
  DBG_LOG("V_CustomBackground\n", 0);
  // If callback not defined, call Impl_Frame_3D_ClearColor(); with RunnerParam_X.imGuiWindowParams.backgroundColor (BackgroudHexaColor_S)
}
void Bof_ImGui::V_ShowMenus()
{
  DBG_LOG("V_ShowMenus\n", 0);
}
void Bof_ImGui::V_ShowStatus()
{
  DBG_LOG("V_ShowStatus\n", 0);
}
void Bof_ImGui::V_ShowGui()
{
  DBG_LOG("V_ShowGui\n", 0);
  // HandleComputerKeyboard();

  // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
  if (mImguiParam_X.ShowDemoWindow_B)
  {
    ShowDemoSelectorWindow();
    if (mShowDemoWindow_B)
    {
      ImGui::ShowDemoWindow(&mShowDemoWindow_B);
    }
    ShowDemoAnotherWindow();
    ShowDemoSpecialTextWindow();
  }
  V_RefreshGui();
}
void Bof_ImGui::V_BeforeImGuiRender()
{
  DBG_LOG("V_BeforeImGuiRender\n", 0);
}
void Bof_ImGui::V_AfterSwap()
{
  DBG_LOG("V_AfterSwap\n", 0);
}
bool Bof_ImGui::V_AnyBackendEventCallback(void *_pEvent)
{
  bool Rts_B = false; // By default let's the system handle events
#if defined(HELLOIMGUI_USE_SDL)
#if 0
  SDL_Event *pEvent_X = (SDL_Event *)_pEvent;
  // DBG_LOG("V_AnyBackendEventCallback %p\n", pEvent_X);
  if (pEvent_X)
  {
    switch (pEvent_X->type)
    {
      case SDL_QUIT:
        DBG_LOG("SDL_QUIT event\n", 0);
        break;
      case SDL_APP_TERMINATING:
        DBG_LOG("SDL_APP_TERMINATING event\n", 0);
        break;
      case SDL_APP_LOWMEMORY:
        DBG_LOG("SDL_APP_LOWMEMORY event\n", 0);
        break;
      case SDL_APP_WILLENTERBACKGROUND:
        DBG_LOG("SDL_APP_WILLENTERBACKGROUND event\n", 0);
        break;
      case SDL_APP_DIDENTERBACKGROUND:
        DBG_LOG("SDL_APP_DIDENTERBACKGROUND event\n", 0);
        break;
      case SDL_APP_WILLENTERFOREGROUND:
        DBG_LOG("SDL_APP_WILLENTERFOREGROUND event\n", 0);
        break;
      case SDL_APP_DIDENTERFOREGROUND:
        DBG_LOG("SDL_APP_DIDENTERFOREGROUND event\n", 0);
        break;
      case SDL_LOCALECHANGED:
        DBG_LOG("SDL_LOCALECHANGED event\n", 0);
        break;
      case SDL_DISPLAYEVENT:
        DBG_LOG("SDL_DISPLAYEVENT event\n", 0);
        break;
      case SDL_WINDOWEVENT:
        switch (pEvent_X->window.event)
        {
          // Add cases for SDL_WINDOWEVENT events
          case SDL_WINDOWEVENT_SHOWN:
            DBG_LOG("SDL_WINDOWEVENT_SHOWN event\n", 0);
            break;
          case SDL_WINDOWEVENT_HIDDEN:
            DBG_LOG("SDL_WINDOWEVENT_HIDDEN event\n", 0);
            break;
          case SDL_WINDOWEVENT_EXPOSED:
            DBG_LOG("SDL_WINDOWEVENT_EXPOSED event\n", 0);
            break;
          // Add more cases for other SDL_WINDOWEVENT events
          default:
            DBG_LOG("Unhandled SDL_WindowEvent type: %d\n", pEvent_X->window.event);
            break;
        }
        break;
      case SDL_SYSWMEVENT:
        DBG_LOG("SDL_SYSWMEVENT event\n", 0);
        break;
      case SDL_KEYDOWN:
        DBG_LOG("SDL_KEYDOWN event - Keycode: %d\n", pEvent_X->key.keysym.sym);
        break;
      // Add cases for other SDL keyboard events
      case SDL_KEYUP:
        DBG_LOG("SDL_KEYUP event - Keycode: %d\n", pEvent_X->key.keysym.sym);
        break;
      // Add cases for other SDL keyboard events
      case SDL_TEXTEDITING:
        DBG_LOG("SDL_TEXTEDITING event\n", 0);
        break;
      // Add cases for other SDL text editing events
      case SDL_TEXTINPUT:
        DBG_LOG("SDL_TEXTINPUT event\n", 0);
        break;
      // Add cases for other SDL text input events
      case SDL_KEYMAPCHANGED:
        DBG_LOG("SDL_KEYMAPCHANGED event\n", 0);
        break;
      // Add cases for other SDL keymap changed events
      case SDL_TEXTEDITING_EXT:
        DBG_LOG("SDL_TEXTEDITING_EXT event\n", 0);
        break;
      // Add cases for other SDL extended text editing events
      case SDL_MOUSEMOTION:
        DBG_LOG("SDL_MOUSEMOTION event\n", 0);
        break;
      // Add cases for other SDL mouse events
      case SDL_MOUSEBUTTONDOWN:
        DBG_LOG("SDL_MOUSEBUTTONDOWN event\n", 0);
        break;
      // Add cases for other SDL mouse button down events
      case SDL_MOUSEBUTTONUP:
        DBG_LOG("SDL_MOUSEBUTTONUP event\n", 0);
        break;
      // Add cases for other SDL mouse button up events
      case SDL_MOUSEWHEEL:
        DBG_LOG("SDL_MOUSEWHEEL event\n", 0);
        break;
      // Add cases for other SDL mouse wheel events
      case SDL_JOYAXISMOTION:
        DBG_LOG("SDL_JOYAXISMOTION event\n", 0);
        break;
      // Add cases for other SDL joystick events
      case SDL_JOYBALLMOTION:
        DBG_LOG("SDL_JOYBALLMOTION event\n", 0);
        break;
      // Add cases for other SDL joystick ball motion events
      case SDL_JOYHATMOTION:
        DBG_LOG("SDL_JOYHATMOTION event\n", 0);
        break;
      // Add cases for other SDL joystick hat motion events
      case SDL_JOYBUTTONDOWN:
        DBG_LOG("SDL_JOYBUTTONDOWN event\n", 0);
        break;
      // Add cases for other SDL joystick button down events
      case SDL_JOYBUTTONUP:
        DBG_LOG("SDL_JOYBUTTONUP event\n", 0);
        break;
      // Add cases for other SDL joystick button up events
      case SDL_JOYDEVICEADDED:
        DBG_LOG("SDL_JOYDEVICEADDED event\n", 0);
        break;
      // Add cases for other SDL joystick device added events
      case SDL_JOYDEVICEREMOVED:
        DBG_LOG("SDL_JOYDEVICEREMOVED event\n", 0);
        break;
      // Add cases for other SDL joystick device removed events
      case SDL_JOYBATTERYUPDATED:
        DBG_LOG("SDL_JOYBATTERYUPDATED event\n", 0);
        break;
      // Add cases for other SDL joystick battery updated events
      case SDL_CONTROLLERAXISMOTION:
        DBG_LOG("SDL_CONTROLLERAXISMOTION event\n", 0);
        break;
      // Add cases for other SDL game controller events
      case SDL_CONTROLLERBUTTONDOWN:
        DBG_LOG("SDL_CONTROLLERBUTTONDOWN event\n", 0);
        break;
      // Add cases for other SDL game controller button down events
      case SDL_CONTROLLERBUTTONUP:
        DBG_LOG("SDL_CONTROLLERBUTTONUP event\n", 0);
        break;
      // Add cases for other SDL game controller button up events
      case SDL_CONTROLLERDEVICEADDED:
        DBG_LOG("SDL_CONTROLLERDEVICEADDED event\n", 0);
        break;
      // Add cases for other SDL game controller device added events
      case SDL_CONTROLLERDEVICEREMOVED:
        DBG_LOG("SDL_CONTROLLERDEVICEREMOVED event\n", 0);
        break;
      // Add cases for other SDL game controller device removed events
      case SDL_CONTROLLERDEVICEREMAPPED:
        DBG_LOG("SDL_CONTROLLERDEVICEREMAPPED event\n", 0);
        break;
      // Add cases for other SDL game controller device remapped events
      case SDL_CONTROLLERTOUCHPADDOWN:
        DBG_LOG("SDL_CONTROLLERTOUCHPADDOWN event\n", 0);
        break;
      // Add cases for other SDL game controller touchpad down events
      case SDL_CONTROLLERTOUCHPADMOTION:
        DBG_LOG("SDL_CONTROLLERTOUCHPADMOTION event\n", 0);
        break;
      // Add cases for other SDL game controller touchpad motion events
      case SDL_CONTROLLERTOUCHPADUP:
        DBG_LOG("SDL_CONTROLLERTOUCHPADUP event\n", 0);
        break;
      // Add cases for other SDL game controller touchpad up events
      case SDL_CONTROLLERSENSORUPDATE:
        DBG_LOG("SDL_CONTROLLERSENSORUPDATE event\n", 0);
        break;
      // Add cases for other SDL game controller sensor update events
      case SDL_FINGERDOWN:
        DBG_LOG("SDL_FINGERDOWN event\n", 0);
        break;
      // Add cases for other SDL touch events
      case SDL_FINGERUP:
        DBG_LOG("SDL_FINGERUP event\n", 0);
        break;
      // Add cases for other SDL finger up events
      case SDL_FINGERMOTION:
        DBG_LOG("SDL_FINGERMOTION event\n", 0);
        break;
      // Add cases for other SDL finger motion events
      case SDL_DOLLARGESTURE:
        DBG_LOG("SDL_DOLLARGESTURE event\n", 0);
        break;
      // Add cases for other SDL dollar gesture events
      case SDL_DOLLARRECORD:
        DBG_LOG("SDL_DOLLARRECORD event\n", 0);
        break;
      // Add cases for other SDL dollar record events
      case SDL_MULTIGESTURE:
        DBG_LOG("SDL_MULTIGESTURE event\n", 0);
        break;
      // Add cases for other SDL multi-gesture events
      case SDL_CLIPBOARDUPDATE:
        DBG_LOG("SDL_CLIPBOARDUPDATE event\n", 0);
        break;
      // Add cases for other SDL clipboard update events
      case SDL_DROPFILE:
        DBG_LOG("SDL_DROPFILE event\n", 0);
        break;
      // Add cases for other SDL drop file events
      case SDL_DROPTEXT:
        DBG_LOG("SDL_DROPTEXT event\n", 0);
        break;
      // Add cases for other SDL drop text events
      case SDL_DROPBEGIN:
        DBG_LOG("SDL_DROPBEGIN event\n", 0);
        break;
      // Add cases for other SDL drop begin events
      case SDL_DROPCOMPLETE:
        DBG_LOG("SDL_DROPCOMPLETE event\n", 0);
        break;
      // Add cases for other SDL drop complete events
      case SDL_AUDIODEVICEADDED:
        DBG_LOG("SDL_AUDIODEVICEADDED event\n", 0);
        break;
      // Add cases for other SDL audio device added events
      case SDL_AUDIODEVICEREMOVED:
        DBG_LOG("SDL_AUDIODEVICEREMOVED event\n", 0);
        break;
      // Add cases for other SDL audio device removed events
      case SDL_SENSORUPDATE:
        DBG_LOG("SDL_SENSORUPDATE event\n", 0);
        break;
      // Add cases for other SDL sensor events
      case SDL_RENDER_TARGETS_RESET:
        DBG_LOG("SDL_RENDER_TARGETS_RESET event\n", 0);
        break;
      // Add cases for other SDL render targets reset events
      case SDL_RENDER_DEVICE_RESET:
        DBG_LOG("SDL_RENDER_DEVICE_RESET event\n", 0);
        break;
      // Add cases for other SDL render device reset events
      case SDL_POLLSENTINEL:
        DBG_LOG("SDL_POLLSENTINEL event\n", 0);
        break;
      // Add cases for other SDL poll sentinel events
      case SDL_USEREVENT:
        DBG_LOG("SDL_USEREVENT event\n", 0);
        break;
      // Add cases for other SDL user events
      case SDL_LASTEVENT:
        DBG_LOG("SDL_LASTEVENT event\n", 0);
        break;
      // Add cases for other SDL last events
      default:
        DBG_LOG("Unhandled SDL_Event type: %d\n", pEvent_X->type);
        break;
    }
  }
  else
  {
    DBG_LOG("AnyBackendEventCallback with nullptr\n", 0);
  }
#endif
#else
  void *pEvent_X = (void *)_pEvent;
  DBG_LOG("AnyBackendEventCallback %p\n", pEvent_X);
#endif
  return Rts_B;
}
void Bof_ImGui::V_ShowAppMenuItems()
{
  DBG_LOG("V_ShowAppMenuItems\n", 0);
}

void Bof_ImGui::V_BeforeExit()
{
  DBG_LOG("V_BeforeExit\n", 0);
  V_SaveSettings();
}
void Bof_ImGui::V_BeforeExit_PostCleanup()
{
  DBG_LOG("V_BeforeExit_PostCleanup\n", 0);
}
void Bof_ImGui::V_RegisterTests()
{
  DBG_LOG("V_RegisterTests\n", 0);
}

// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
BOFERR Bof_ImGui::ShowDemoSelectorWindow()
{
  BOFERR Rts_E = mLastError_E;

  if (Rts_E == BOF_ERR_NO_ERROR)
  {
    ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

    ImGui::Text("This is some useful text.");           // Display some text (you can use a format strings too)
    ImGui::Checkbox("Demo Window", &mShowDemoWindow_B); // Edit bools storing our window open/close state
    ImGui::Checkbox("Another Window", &mShowDemoAnotherWindow_B);
    ImGui::Checkbox("Special Text Window", &mShowDemoSpecialTextWindow_B);

    ImGui::SliderFloat("float", &mClearColor_f, 0.0f, 1.0f);   // Edit 1 float using a slider from 0.0f to 1.0f
    ImGui::ColorEdit3("clear color", (float *)&mClearColor_X); // Edit 3 floats representing a color

    if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
    {
      mCounter_U32++;
    }
    ImGui::SameLine();
    ImGui::Text("counter = %d", mCounter_U32);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
  }
  return Rts_E;
}

// 3. Show another simple window.
BOFERR Bof_ImGui::ShowDemoAnotherWindow()
{
  BOFERR Rts_E = mLastError_E;

  if (Rts_E == BOF_ERR_NO_ERROR)
  {
    if (mShowDemoAnotherWindow_B)
    {
      ImGui::Begin("Another Window", &mShowDemoAnotherWindow_B); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
      ImGui::Text("Hello from another window!");
      if (ImGui::Button("Close Me"))
      {
        mShowDemoAnotherWindow_B = false;
      }
      ImGui::End();
    }
  }
  return Rts_E;
}

BOFERR Bof_ImGui::ShowDemoSpecialTextWindow()
{
  BOFERR Rts_E = mLastError_E;
  ImColor red(255, 0, 0, 255);
  ImColor green(0, 255, 0, 255);
  ImColor blue(0, 0, 255, 255);
  ImColor yellow(255, 255, 0, 255);
  ImColor brown(187, 126, 0, 255);
  ImColor cyan(0, 255, 255, 255);
  ImColor magenta(255, 0, 255, 125);
  const char *pText1_c = "The quick red fox jumps over the green box.";
  Bof_ImGui_ImTextCustomization tc;
  static ImGuiColorEditFlags col_flag = ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel;

  // Demo the colorful text. In practice, we can generate the index for a pattern or text with a grep like tool.
  const static char *pText2_c = "Hello, Dear ImGui Text Customization. It works with Text Highlight, Text Underline, Text Strikethrough, Text Wrap, Text Disabling and Text Mask. "
                                "And it shold work across one or more \r\n\r\nempty lines as well. You are free to apply one or more customization for any text inside the string.\r\n";

  struct _Config
  {
    bool wrapped = true;
    bool disabled = false;
    bool textcolor = true;
    bool highlight = true;
    bool underline = true;
    bool strikethrough = true;
    bool textmask = true;

    // text color
    int txt_1_pos_begin = 0;
    int txt_1_pos_end = 0;
    ImVec4 txt_col_1 = ImVec4(255.0f / 255.0f, 0.0f / 255.0f, 0.0f / 255.0f, 200.0f / 255.0f);
    int txt_2_pos_begin = 0;
    int txt_2_pos_end = 0;
    ImVec4 txt_col_2 = ImVec4(0.0f / 255.0f, 255.0f / 255.0f, 0.0f / 255.0f, 200.0f / 255.0f);

    // highlight color
    int hl_1_pos_begin = 0;
    int hl_1_pos_end = 0;
    ImVec4 hl_col_1 = ImVec4(255.0f / 255.0f, 0.0f / 255.0f, 0.0f / 255.0f, 200.0f / 255.0f);
    int hl_2_pos_begin = 0;
    int hl_2_pos_end = 0;
    ImVec4 hl_col_2 = ImVec4(0.0f / 255.0f, 255.0f / 255.0f, 0.0f / 255.0f, 200.0f / 255.0f);

    // underline color
    int ul_1_pos_begin = 0;
    int ul_1_pos_end = 0;
    ImVec4 ul_col_1 = ImVec4(255.0f / 255.0f, 0.0f / 255.0f, 0.0f / 255.0f, 200.0f / 255.0f);
    int ul_2_pos_begin = 0;
    int ul_2_pos_end = 0;
    ImVec4 ul_col_2 = ImVec4(0.0f / 255.0f, 255.0f / 255.0f, 0.0f / 255.0f, 200.0f / 255.0f);

    // strikethrough color
    int st_1_pos_begin = 0;
    int st_1_pos_end = 0;
    ImVec4 st_col_1 = ImVec4(255.0f / 255.0f, 0.0f / 255.0f, 0.0f / 255.0f, 200.0f / 255.0f);
    int st_2_pos_begin = 0;
    int st_2_pos_end = 0;
    ImVec4 st_col_2 = ImVec4(0.0f / 255.0f, 255.0f / 255.0f, 0.0f / 255.0f, 200.0f / 255.0f);

    // text mask color
    int msk_1_pos_begin = 0;
    int msk_1_pos_end = 0;
    ImVec4 msk_col_1 = ImVec4(255.0f / 255.0f, 0.0f / 255.0f, 0.0f / 255.0f, 200.0f / 255.0f);
    int msk_2_pos_begin = 0;
    int msk_2_pos_end = 0;
    ImVec4 msk_col_2 = ImVec4(0.0f / 255.0f, 255.0f / 255.0f, 0.0f / 255.0f, 200.0f / 255.0f);
  };
  static _Config c;
  int len = (int)strlen(pText2_c);
  Bof_ImGui_ImTextCustomization thestyle;

  if (Rts_E == BOF_ERR_NO_ERROR)
  {
    if (mShowDemoSpecialTextWindow_B)
    {
      ImGui::NewLine();
      ImGui::Text("Color the whole text");
      DisplayText(nullptr, pText1_c, true, false, &tc.Range(pText1_c).TextColor(green));

      // With Bof_ImGui_ImTextCustomization we can chain multiple styles for one range
      ImGui::Text("Or color and underline it at the same time");
      DisplayText(nullptr, pText1_c, true, false, &tc.Range(pText1_c).TextColor(yellow).Underline(red));

      ImGui::Text("Color the substring of the text");
      DisplayText(nullptr, pText1_c, true, false, &tc.Clear().Range(pText1_c + 14, pText1_c + 17).TextColor(red).Range(pText1_c + 39, pText1_c + 42).TextColor(green));

      ImGui::Text("Underline");
      DisplayText(nullptr, pText1_c, true, false, &tc.Clear().Range(pText1_c).Underline());

      ImGui::Text("Underline with color");
      DisplayText(nullptr, pText1_c, true, false, &tc.Clear().Range(pText1_c).Underline(blue));

      ImGui::Text("Strikethrough");
      DisplayText(nullptr, pText1_c, true, false, &tc.Clear().Range(pText1_c).Strkethrough());

      ImGui::Text("Strikethrough with color");
      DisplayText(nullptr, pText1_c, true, false, &tc.Clear().Range(pText1_c).Strkethrough(red));

      ImGui::Text("Hilight the text with brown");
      DisplayText(nullptr, pText1_c, true, false, &tc.Clear().Range(pText1_c).Highlight(brown));

      ImGui::Text("Mask the text so it is not readable");
      DisplayText(nullptr, pText1_c, true, false, &tc.Clear().Range(pText1_c + 10, pText1_c + 17).Mask());

      ImGui::Text("Mask the text with color");
      DisplayText(nullptr, pText1_c, true, false, &tc.Clear().Range(pText1_c + 10, pText1_c + 17).Mask(cyan));

      ImGui::Text("Mask the text with semitransparent color");
      DisplayText(nullptr, pText1_c, true, false, &tc.Clear().Range(pText1_c + 10, pText1_c + 28).Mask(magenta));
      ImGui::NewLine();

      ImGui::BeginGroup();
      ImGui::Checkbox("Wrap", &c.wrapped);
      ImGui::SameLine();
      ImGui::Checkbox("Disabled", &c.disabled);
      ImGui::SameLine();
      ImGui::Checkbox("Textcolor", &c.textcolor);
      ImGui::SameLine();
      ImGui::Checkbox("Highlight", &c.highlight);
      ImGui::SameLine();
      ImGui::Checkbox("Underline", &c.underline);
      ImGui::SameLine();
      ImGui::Checkbox("Strikethrough", &c.strikethrough);
      ImGui::SameLine();
      ImGui::Checkbox("Text Mask", &c.textmask);
      ImGui::EndGroup();

      ImGui::BeginGroup();
      ImGui::ColorEdit4("Text Col_1##1", (float *)&c.txt_col_1, col_flag);
      ImGui::SameLine();
      ImGui::DragIntRange2("Text Color 1", &c.txt_1_pos_begin, &c.txt_1_pos_end, 1, 0, len, "Range Begin: %d", "Range End: %d");
      ImGui::EndGroup();
      ImGui::BeginGroup();
      ImGui::ColorEdit4("Text Col_2##2", (float *)&c.txt_col_2, col_flag);
      ImGui::SameLine();
      ImGui::DragIntRange2("Text Color 2", &c.txt_2_pos_begin, &c.txt_2_pos_end, 1, 0, len, "Range Begin: %d", "Range End: %d");
      ImGui::EndGroup();

      ImGui::BeginGroup();
      ImGui::ColorEdit4("Highligh Col_1##1", (float *)&c.hl_col_1, col_flag);
      ImGui::SameLine();
      ImGui::DragIntRange2("Highlight 1", &c.hl_1_pos_begin, &c.hl_1_pos_end, 1, 0, len, "Range Begin: %d", "Range End: %d");
      ImGui::EndGroup();
      ImGui::BeginGroup();
      ImGui::ColorEdit4("Highlight Col_1##2", (float *)&c.hl_col_2, col_flag);
      ImGui::SameLine();
      ImGui::DragIntRange2("Highlight 2", &c.hl_2_pos_begin, &c.hl_2_pos_end, 1, 0, len, "Range Begin: %d", "Range End: %d");
      ImGui::EndGroup();

      ImGui::BeginGroup();
      ImGui::ColorEdit4("Underline Col_1##1", (float *)&c.ul_col_1, col_flag);
      ImGui::SameLine();
      ImGui::DragIntRange2("Underline 1", &c.ul_1_pos_begin, &c.ul_1_pos_end, 1, 0, len, "Range Begin: %d", "Range End: %d");
      ImGui::EndGroup();
      ImGui::BeginGroup();
      ImGui::ColorEdit4("Underline Col_2##1", (float *)&c.ul_col_2, col_flag);
      ImGui::SameLine();
      ImGui::DragIntRange2("Underline 2", &c.ul_2_pos_begin, &c.ul_2_pos_end, 1, 0, len, "Range Begin: %d", "Range End: %d");
      ImGui::EndGroup();

      ImGui::BeginGroup();
      ImGui::ColorEdit4("Strikethrough Col_1##1", (float *)&c.st_col_1, col_flag);
      ImGui::SameLine();
      ImGui::DragIntRange2("Strikethrough 1", &c.st_1_pos_begin, &c.st_1_pos_end, 1, 0, len, "Range Begin: %d", "Range End: %d");
      ImGui::EndGroup();
      ImGui::BeginGroup();
      ImGui::ColorEdit4("Strikethrough Col_2##1", (float *)&c.st_col_2, col_flag);
      ImGui::SameLine();
      ImGui::DragIntRange2("Strikethrough 2", &c.st_2_pos_begin, &c.st_2_pos_end, 1, 0, len, "Range Begin: %d", "Range End: %d");
      ImGui::EndGroup();

      ImGui::BeginGroup();
      ImGui::ColorEdit4("Text Mask Col_1##1", (float *)&c.msk_col_1, col_flag);
      ImGui::SameLine();
      ImGui::DragIntRange2("Mask 1", &c.msk_1_pos_begin, &c.msk_1_pos_end, 1, 0, len, "Range Begin: %d", "Range End: %d");
      ImGui::EndGroup();
      ImGui::BeginGroup();
      ImGui::ColorEdit4("Text Mask Col_2##1", (float *)&c.msk_col_2, col_flag);
      ImGui::SameLine();
      ImGui::DragIntRange2("Mask 2", &c.msk_2_pos_begin, &c.msk_2_pos_end, 1, 0, len, "Range Begin: %d", "Range End: %d");
      ImGui::EndGroup();

      // text color range
      if (c.textcolor)
      {
        thestyle.Range(pText2_c + c.txt_1_pos_begin, pText2_c + c.txt_1_pos_end).TextColor(c.txt_col_1).Disabled(c.disabled);
        thestyle.Range(pText2_c + c.txt_2_pos_begin, pText2_c + c.txt_2_pos_end).TextColor(c.txt_col_2).Disabled(c.disabled);
      }
      // highlight range
      if (c.highlight)
      {
        thestyle.Range(pText2_c + c.hl_1_pos_begin, pText2_c + c.hl_1_pos_end).Highlight(c.hl_col_1);
        thestyle.Range(pText2_c + c.hl_2_pos_begin, pText2_c + c.hl_2_pos_end).Highlight(c.hl_col_2);
      }
      // underline range
      if (c.underline)
      {
        thestyle.Range(pText2_c + c.ul_1_pos_begin, pText2_c + c.ul_1_pos_end).Underline(c.ul_col_1);
        thestyle.Range(pText2_c + c.ul_2_pos_begin, pText2_c + c.ul_2_pos_end).Underline(c.ul_col_2);
      }
      // strikethrough range
      if (c.strikethrough)
      {
        thestyle.Range(pText2_c + c.st_1_pos_begin, pText2_c + c.st_1_pos_end).Strkethrough(c.st_col_1);
        thestyle.Range(pText2_c + c.st_2_pos_begin, pText2_c + c.st_2_pos_end).Strkethrough(c.st_col_2);
      }
      // mask range
      if (c.textmask)
      {
        thestyle.Range(pText2_c + c.msk_1_pos_begin, pText2_c + c.msk_1_pos_end).Mask(c.msk_col_1);
        thestyle.Range(pText2_c + c.msk_2_pos_begin, pText2_c + c.msk_2_pos_end).Mask(c.msk_col_2);
      }

      DisplayText(nullptr, pText2_c, c.wrapped, c.disabled, &thestyle);
    }
  }
  return Rts_E;
}

END_BOF_NAMESPACE()