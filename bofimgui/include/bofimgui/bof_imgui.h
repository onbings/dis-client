/*
 * Copyright (c) 2023-2033, Onbings. All rights reserved.
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * This module defines the bof module interface to imgui and helloimgui lib
 *
 * History:
 *
 * V 1.00  Dec 24 2023  Bernard HARMEL: onbings@gmail.com : Initial release
 */
#pragma once
// You may use this file to debug, understand or extend Dear ImGui features but we don't provide any guarantee of forward compatibility.
// To implement maths operators for ImVec2 (disabled by default to not conflict with using IM_VEC2_CLASS_EXTRA with your own math types+operators), use:
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

#include "hello_imgui/hello_imgui.h"
/*
HELLOIMGUI SDL based project define the following symbols
HELLOIMGUI_USE_GLAD
IMGUI_IMPL_OPENGL_LOADER_GLAD
HELLOIMGUI_USE_SDL_OPENGL3
HELLOIMGUI_USE_SDL
HELLOIMGUI_HAS_OPENGL

if WIN32 def, we use Glfw: HELLOIMGUI_USE_GLFW
*/

#if defined(HELLOIMGUI_USE_SDL)
/*
Under windows, Hello ImGui will automatically provide a WinMain() function that will call main, and expects its signature to be int main(int, char**).
You may get a linker error if your main function signature is for example int main().
You can disable this via cmake by passing -DHELLOIMGUI_WIN32_AUTO_WINMAIN=OFF as a command line cmake option. In this case, write your own WinMain under windows.
Warning: if using SDL, you will need to #define SDL_MAIN_HANDLED before any inclusion of SDL.h (to refrain SDL from #defining #define main SDL_main)
*/
#if defined(_WIN32)
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>
#else
#endif
#include <bofstd/bofstd.h>
#include <functional>
#include <stdarg.h>

BEGIN_BOF_NAMESPACE()
using Logger = std::function<void(const char *)>;

struct BOF_IMGUI_PARAM
{
  std::string WindowTitle_S;  //Used also as user setting save and load op id
  BOF::BOF_SIZE<uint32_t> Size_X;
  std::string BackgroudHexaColor_S;  //if not set use V_CustomBackground callback
  bool ShowDemoWindow_B;
  bool ShowMenuBar_B;
  bool ShowStatusBar_B;
  Logger TheLogger;

  BOF_IMGUI_PARAM()
  {
    Reset();
  }
  void Reset()
  {
    WindowTitle_S = "";
    Size_X.Reset();
    BackgroudHexaColor_S = "";
    ShowDemoWindow_B = false;
    ShowMenuBar_B = false;
    ShowStatusBar_B = false;
    TheLogger = nullptr;
  }
};
struct BOF_IMGUI_FONT
{
  std::string Name_S;
  uint32_t Size_U32;  //To be multiplied by ImGui::GetIO().FontGlobalScale if needed
  BOF_IMGUI_FONT()
  {
    Reset();
  }
  void Reset()
  {
    Name_S = "";
    Size_U32 = 0;
  }
};
struct Bof_ImGui_ImTextCustomization
{
  struct Bof_ImGui_RangeItem
  {
    const char *pPosStart_c, *pPosStop_c;
    ImColor TextColor_X, HighlightColor_X, UnderlineColor_X, StrikethroughColor_X, MaskColor_X;
    uint32_t Flag_U32;

    enum FLAG
    {
      TEXTCOLOR = 1,
      HIGHLIGHT = 1 << 1,
      UNDERLINE = 1 << 2,
      STRIKETHROUGH = 1 << 3,
      MASK = 1 << 4,
      DISABLED = 1 << 5
    };

    // A zero color value will be ignored
    Bof_ImGui_RangeItem()
    {
      pPosStart_c = nullptr;
      pPosStop_c = nullptr;
      TextColor_X = 0;
      HighlightColor_X = 0;
      UnderlineColor_X = 0;
      StrikethroughColor_X = 0;
      Flag_U32 = 0;
    }
    Bof_ImGui_RangeItem(const char *pos_start, const char *pos_stop)
        : Bof_ImGui_RangeItem()
    {
      pPosStart_c = pos_start;
      pPosStop_c = pos_stop;
    }
  };

  ImVector<Bof_ImGui_RangeItem> RangeCollection;

  // Reset all customizations by empty RangeCollection
  Bof_ImGui_ImTextCustomization &Clear()
  {
    RangeCollection.clear();
    return *this;
  }

  // Specify a range of the text first
  Bof_ImGui_ImTextCustomization &Range(const char *_pBegin_c, const char *_pEnd_c = nullptr)
  {
    // add valid range to list. ignore invalid range
    if (_pBegin_c != nullptr && _pEnd_c >= _pBegin_c)
    {
      RangeCollection.push_back(Bof_ImGui_RangeItem(_pBegin_c, _pEnd_c));
    }
    else if (_pBegin_c != nullptr && _pEnd_c == nullptr)
    {
      RangeCollection.push_back(Bof_ImGui_RangeItem(_pBegin_c, _pBegin_c + strlen(_pBegin_c)));
    }

    return *this;
  }
  // The last range specified will be colored
  Bof_ImGui_ImTextCustomization &TextColor(ImColor _Color_X)
  {
    Bof_ImGui_RangeItem &rItem_X = RangeCollection.back();
    rItem_X.Flag_U32 |= Bof_ImGui_RangeItem::FLAG::TEXTCOLOR;
    rItem_X.TextColor_X = _Color_X;
    return *this;
  }
  // The last range specified will be highlighted
  Bof_ImGui_ImTextCustomization &Highlight(ImColor _Color_X)
  {
    Bof_ImGui_RangeItem &rItem_X = RangeCollection.back();
    rItem_X.Flag_U32 |= Bof_ImGui_RangeItem::FLAG::HIGHLIGHT;
    rItem_X.HighlightColor_X = _Color_X;
    return *this;
  }
  // The last range specified will be underlined
  Bof_ImGui_ImTextCustomization &Underline(ImColor _Color_X = 0)
  {
    Bof_ImGui_RangeItem &rItem_X = RangeCollection.back();
    rItem_X.Flag_U32 |= Bof_ImGui_RangeItem::FLAG::UNDERLINE;
    rItem_X.UnderlineColor_X = _Color_X;
    return *this;
  }
  // The last range specified will be striked through
  Bof_ImGui_ImTextCustomization &Strkethrough(ImColor _Color_X = 0)
  {
    Bof_ImGui_RangeItem &rItem_X = RangeCollection.back();
    rItem_X.Flag_U32 |= Bof_ImGui_RangeItem::FLAG::STRIKETHROUGH;
    rItem_X.StrikethroughColor_X = _Color_X;
    return *this;
  }
  // The last range specified will be masked
  Bof_ImGui_ImTextCustomization &Mask(ImColor _Color_X = 0)
  {
    Bof_ImGui_RangeItem &rItem_X = RangeCollection.back();
    rItem_X.Flag_U32 |= Bof_ImGui_RangeItem::FLAG::MASK;
    rItem_X.MaskColor_X = _Color_X;
    return *this;
  }
  // The last range specified will be shown as disabled
  Bof_ImGui_ImTextCustomization &Disabled(bool _Disabled_B = true)
  {
    Bof_ImGui_RangeItem &rItem_X = RangeCollection.back();
    if (_Disabled_B)
    {
      rItem_X.Flag_U32 |= Bof_ImGui_RangeItem::FLAG::DISABLED;
    }
    return *this;
  }

  struct Bof_ImGui_TextStyle
  {
    bool Text_B;
    bool Disabled_B;
    bool Highlight_B;
    bool Underline_B;
    bool Strikethrough_B;
    // Mask covers the text so that it cannot be seen
    bool Mask_B;
    ImU32 TextColor_X;
    ImU32 HighlightColor_X;
    ImU32 UnderlineColor_X;
    ImU32 StrikethroughColor_X;
    ImU32 MaskColor_X;
    Bof_ImGui_TextStyle()
    {
      memset(this, 0, sizeof(Bof_ImGui_TextStyle));
    }
  };

  // Find the style for given position
  Bof_ImGui_TextStyle GetStyleByPosition(const char *char_pos) const
  {
    Bof_ImGui_TextStyle Rts_X;

    for (int idx = 0; idx < RangeCollection.Size; idx++)
    {
      const Bof_ImGui_RangeItem &rStyleItem = RangeCollection[idx];
      if (char_pos >= rStyleItem.pPosStart_c && char_pos < rStyleItem.pPosStop_c)
      {
        if (rStyleItem.Flag_U32 & Bof_ImGui_RangeItem::FLAG::TEXTCOLOR)
        {
          Rts_X.Text_B = true;
          Rts_X.TextColor_X = rStyleItem.TextColor_X;
        }
        if (rStyleItem.Flag_U32 & Bof_ImGui_RangeItem::FLAG::HIGHLIGHT)
        {
          Rts_X.Highlight_B = true;
          Rts_X.HighlightColor_X = rStyleItem.HighlightColor_X;
        }
        if (rStyleItem.Flag_U32 & Bof_ImGui_RangeItem::FLAG::STRIKETHROUGH)
        {
          Rts_X.Strikethrough_B = true;
          Rts_X.StrikethroughColor_X = rStyleItem.StrikethroughColor_X;
        }
        if (rStyleItem.Flag_U32 & Bof_ImGui_RangeItem::FLAG::UNDERLINE)
        {
          Rts_X.Underline_B = true;
          Rts_X.UnderlineColor_X = rStyleItem.UnderlineColor_X;
        }
        if (rStyleItem.Flag_U32 & Bof_ImGui_RangeItem::FLAG::DISABLED)
        {
          Rts_X.Disabled_B = true;
        }
        if (rStyleItem.Flag_U32 & Bof_ImGui_RangeItem::FLAG::MASK)
        {
          Rts_X.Mask_B = true;
          Rts_X.MaskColor_X = rStyleItem.MaskColor_X;
        }
      }
    }
    return Rts_X;
  }
};

class Bof_ImGui
{
public:
  Bof_ImGui(const BOF_IMGUI_PARAM &_rImguiParam_X);
  ~Bof_ImGui();
  BOFERR MainLoop();
  BOFERR SetNextWindowSize(uint32_t _Width_U32, uint32_t _Height_U32);
  BOFERR SetCursorPos(int32_t _x_S32, int32_t _y_S32);
  BOFERR DisplayText(const char *_pText_c, bool _Wrapped_B, bool _Disabled_B, const Bof_ImGui_ImTextCustomization *_pTextCustomization_X);
  BOFERR DisplayText(BOF::BOF_POINT_2D<int32_t> *_pCursorPos_X, const char *_pText_c, bool _Wrapped_B, bool _Disabled_B, const Bof_ImGui_ImTextCustomization *_pTextCustomization_X);
  std::vector<BOF_IMGUI_FONT> GetFontList();
  ImFont *GetFont(uint32_t _FontIndex_U32);
  ImFont *LoadFont(const char *_pFontFileTtf_c, uint32_t _FontSizeInPixel_U32);
  BOF::BOF_SIZE<uint32_t> GetTextSize(const char *_pText_c);
  static bool S_HexaColor(const std::string &_rHexaColor_S, uint8_t(&_rColor_U8)[4]); //#RRGGBB or #RRGGBBAA

protected:
  virtual BOFERR V_ReadSettings() = 0;
  virtual BOFERR V_SaveSettings() = 0;
  virtual BOFERR V_RefreshGui() = 0;
  static void S_BuildHelpMarker(const char *_pHelp_c);
  virtual void V_OnKeyboardPress(char _Ch_c, const std::string &_rKeyState_S);

  virtual void V_SetupImGuiConfig();
  virtual void V_SetupImGuiStyle();
  virtual void V_PostInit();
  virtual void V_LoadAdditionalFonts();
  virtual void V_PreNewFrame();
  virtual void V_CustomBackground();
  virtual void V_ShowMenus();
  virtual void V_ShowStatus();
  virtual void V_ShowGui();
  virtual void V_BeforeImGuiRender();
  virtual void V_AfterSwap();
  virtual bool V_AnyBackendEventCallback(void *_pEvent);
  virtual void V_ShowAppMenuItems();
  virtual void V_BeforeExit();
  virtual void V_BeforeExit_PostCleanup();
  virtual void V_RegisterTests();
  static std::string S_GetKeyboardState();

private:
  void HandleComputerKeyboard();
 

private:
  BOF_IMGUI_PARAM mImguiParam_X;
  BOFERR mLastError_E;

  bool mShowDemoWindow_B = false;
  bool mShowDemoAnotherWindow_B = false;
  bool mShowDemoSpecialTextWindow_B = false;
  ImVec4 mClearColor_X;
  float mClearColor_f = 0.0f;
  uint32_t mCounter_U32 = 0;
  
  BOFERR ShowDemoSelectorWindow();
  BOFERR ShowDemoAnotherWindow();
  BOFERR ShowDemoSpecialTextWindow();
};
END_BOF_NAMESPACE()
