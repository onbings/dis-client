/*
 * Copyright (c) 2023-2033, Onbings. All rights reserved.
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * The following lib is extracted from https://github.com/ForrestFeng/imgui/tree/Text_Customization
 * I got this from https://github.com/ocornut/imgui/issues/902 see ForrestFeng commented on May 4, 2022
 * the idea is to have a basic text renderer to gives a colored version of text.
 *
 * History:
 *
 * V 1.00  Dec 24 2023  Bernard HARMEL: onbings@gmail.com : Initial release
 */
#pragma once
// You may use this file to debug, understand or extend Dear ImGui features but we don't provide any guarantee of forward compatibility.
// To implement maths operators for ImVec2 (disabled by default to not conflict with using IM_VEC2_CLASS_EXTRA with your own math types+operators), use:
#define IMGUI_DEFINE_MATH_OPERATORS
#include "hello_imgui/hello_imgui.h"
#include "imgui.h"
#include "imgui_internal.h"

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
#include <SDL.h>
//#include <SDL_events.h>
#else
#endif
#include "bof_imgui.h"

BEGIN_BOF_NAMESPACE()

void ImGuiTextEx(const char *_pTextStart_c, const char *_pTextEnd_c, ImGuiTextFlags _Flags_S32, const Bof_ImGui_ImTextCustomization *_pTextCustomization_X);

END_BOF_NAMESPACE()
