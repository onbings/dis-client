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
#include "bofimgui/bof_imgui_internal.h"

/*
The following lib is extracted from https://github.com/ForrestFeng/imgui/tree/Text_Customization
I got this from https://github.com/ocornut/imgui/issues/902 see ForrestFeng commented on May 4, 2022
the idea is to have a basic text renderer to gives a colored version of text.
*/

BEGIN_BOF_NAMESPACE()

//-----------------------------------------------------------------------------
// [SECTION] ImTextCustomStyle  definition
//-----------------------------------------------------------------------------
// Support text customization like text color, text background, croosline and underline
// ImGuiContext *GL_ImGuiContext = nullptr;

static inline const char *S_CalcWordWrapNextLineStartA(const char *text, const char *text_end)
{
  while (text < text_end && ImCharIsBlankA(*text))
  {
    text++;
  }
  if (*text == '\n')
  {
    text++;
  }
  return text;
}
// Note: as with every ImDrawList drawing function, this expects that the font atlas texture is bound.
void ImFontRenderText(const ImFont *pFont, ImDrawList *draw_list, float size, ImVec2 pos, ImU32 col, const ImVec4 &clip_rect, const char *text_begin, const char *text_end, float wrap_width, bool cpu_fine_clip, const Bof_ImGui_ImTextCustomization *customization)
{
  if (!text_end)
  {
    text_end = text_begin + strlen(text_begin); // ImGui:: functions generally already provides a valid text_end, so this is merely to handle direct calls.
  }

  // Align to be pixel perfect
  float x = IM_FLOOR(pos.x);
  float y = IM_FLOOR(pos.y);
  if (y > clip_rect.w)
  {
    return;
  }

  const float start_x = x;
  const float scale = size / pFont->FontSize;
  const float line_height = pFont->FontSize * scale;
  const bool word_wrap_enabled = (wrap_width > 0.0f);

  // Fast-forward to first visible line
  const char *s = text_begin;
  if (y + line_height < clip_rect.y)
  {
    while (y + line_height < clip_rect.y && s < text_end)
    {
      const char *line_end = (const char *)memchr(s, '\n', text_end - s);
      if (word_wrap_enabled)
      {
        // FIXME-OPT: This is not optimal as do first do a search for \n before calling CalcWordWrapPositionA().
        // If the specs for CalcWordWrapPositionA() were reworked to optionally return on \n we could combine both.
        // However it is still better than nothing performing the fast-forward!
        s = pFont->CalcWordWrapPositionA(scale, s, line_end, wrap_width);
        s = S_CalcWordWrapNextLineStartA(s, text_end);
      }
      else
      {
        s = line_end ? line_end + 1 : text_end;
      }
      y += line_height;
    }
  }

  // For large text, scan for the last visible line in order to avoid over-reserving in the call to PrimReserve()
  // Note that very large horizontal line will still be affected by the issue (e.g. a one megabyte string buffer without a newline will likely crash atm)
  if (text_end - s > 10000 && !word_wrap_enabled)
  {
    const char *s_end = s;
    float y_end = y;
    while (y_end < clip_rect.w && s_end < text_end)
    {
      s_end = (const char *)memchr(s_end, '\n', text_end - s_end);
      s_end = s_end ? s_end + 1 : text_end;
      y_end += line_height;
    }
    text_end = s_end;
  }
  if (s == text_end)
  {
    return;
  }

  // draw_list_text
  ImDrawList *draw_list_text = nullptr;

  // Create a dedicated draw list for text. The reson is that the highlight texts must draw before the text.
  // The highlight texts is generated during the glyph drawing process.
  if (customization)
  {
    draw_list_text = new ImDrawList(ImGui::GetDrawListSharedData());
    draw_list_text->AddDrawCmd();
  }
  else
  {
    draw_list_text = draw_list;
  }

  // Reserve vertices for remaining worse case (over-reserving is useful and easily amortized)
  const int vtx_count_max = (int)(text_end - s) * 4;
  const int idx_count_max = (int)(text_end - s) * 6;
  const int idx_expected_size = draw_list_text->IdxBuffer.Size + idx_count_max;

  // Reserve for text
  draw_list_text->PrimReserve(idx_count_max, vtx_count_max);

  ImDrawVert *vtx_write = draw_list_text->_VtxWritePtr;
  ImDrawIdx *idx_write = draw_list_text->_IdxWritePtr;
  unsigned int vtx_current_idx = draw_list_text->_VtxCurrentIdx;

  const ImU32 col_untinted = col | ~IM_COL32_A_MASK;
  const char *word_wrap_eol = nullptr;

  ImU32 text_col = col;

  // Internal struct to eclose all text customization processing
  struct _CustomizationParser
  {
    // customization for the text
    // readonly during the liftime of this struct
    const Bof_ImGui_ImTextCustomization *custom_style;
    // the text passed to RenderText
    // readonly during the liftime of this struct
    const char *text_begin;
    const char *text_end;

    // cache of the text col, as a default color of underline or strikethrough,
    // readonly during the liftime of this struct
    ImU32 text_col;

    // text size,
    // readonly during the liftime of this struct
    float size;

    // font, readonly during the liftime of this struct
    const ImFont *font;

    // the text lien height, readonly during the liftime of this struct
    float line_height;
    float strikethrough_offset_y;

    // style for the current glyph and last glyph
    Bof_ImGui_ImTextCustomization::Bof_ImGui_TextStyle style, last_style;

    // top left x, y value for the current glyph
    float x;
    float y;

    // Data to hold segment position and color
    // One segment can cover more than one glyphs
    struct SegmentPosCol
    {
      ImVec2 BeginPos;
      ImVec2 EndPos;
      ImU32 Color;
    };

    struct Segments
    {
      // indicates if current glyph starts a new line caused by wrod warp or one or more \n
      // 0: still in the same line.
      // >0: starts a new line
      int new_line = 0;

      // top left x, y value before new line (caused by word warp or line break \n)
      float x_wrap_eol = 0.0f;
      float y_wrap_eol = 0.0f;

      // the segments generated for underline, highlight, strikethrough etc.
      ImVector<SegmentPosCol> data;

      // CTOR
      Segments()
          : new_line(0), x_wrap_eol(0.0f), y_wrap_eol(0.0f)
      {
      }

      void OnNewLine(const char *char_pos, float x_pos, float y_pos)
      {
        // prevent unused error
        //(void *)char_pos;

        // Remember the x, y position of the last glyph before we start a new line
        if (++new_line == 1)
        {
          x_wrap_eol = x_pos;
          y_wrap_eol = y_pos;
        }
      }
    };
    Segments underline_segments;
    Segments strikethrough_segments;
    Segments highlight_segments;
    Segments mask_segments;

    // CTOR
    _CustomizationParser(ImU32 text_col, float size, const char *text_begin, const char *text_end, const ImFont *font, const Bof_ImGui_ImTextCustomization *custom_style)
        : text_col(text_col), size(size), text_begin(text_begin), text_end(text_end), font(font), custom_style(custom_style)
    {
      line_height = size;
      // Reference MS Word Offic SW, the strke y position is about 59% line_height down.
      strikethrough_offset_y = (line_height * 59.f / 100.f);
    }

    // Called for every glyph with is about to draw. It returns the color for the glyph
    ImU32 OnNewGlyph(const char *glyph_pos, const char *glyph_end, unsigned int glyph_code, float x_pos, float y_pos)
    {
      // this->x and this-> are readonly only change it here.
      this->x = x_pos;
      this->y = y_pos;

      ImU32 col = text_col;

      // check custom style
      last_style = style;
      style = custom_style->GetStyleByPosition(glyph_pos);

      // Update text color
      if (style.Disabled_B)
      {
        col = text_col;
      }
      else if (style.Text_B)
      {
        col = style.TextColor_X;
      }

      // call internal method to process for each segments
      _InternalOnNewGlyph(highlight_segments, glyph_end, glyph_code, style.HighlightColor_X, last_style.HighlightColor_X, style.Highlight_B, last_style.Highlight_B, 0.0f, line_height);
      _InternalOnNewGlyph(underline_segments, glyph_end, glyph_code, style.UnderlineColor_X, last_style.UnderlineColor_X, style.Underline_B, last_style.Underline_B, line_height, line_height);
      _InternalOnNewGlyph(strikethrough_segments, glyph_end, glyph_code, style.StrikethroughColor_X, last_style.StrikethroughColor_X, style.Strikethrough_B, last_style.Strikethrough_B, strikethrough_offset_y, strikethrough_offset_y);
      _InternalOnNewGlyph(mask_segments, glyph_end, glyph_code, style.MaskColor_X, last_style.MaskColor_X, style.Mask_B, last_style.Mask_B, 0.0f, line_height);
      return col;
    }

    // Handles the new line which caused by word warp or \n. Multiple \n will result the new_line > 1
    void OnNewLine(const char *char_pos, float x_pos, float y_pos)
    {
      // Remember the x, y position of the last glyph before we start a new line
      highlight_segments.OnNewLine(char_pos, x_pos, y_pos);
      underline_segments.OnNewLine(char_pos, x_pos, y_pos);
      strikethrough_segments.OnNewLine(char_pos, x_pos, y_pos);
      mask_segments.OnNewLine(char_pos, x_pos, y_pos);

      // Reaches to the end of the end of the text. this is the last chance to close all open position
      if (char_pos == text_end)
      {
        _InternalOnNewGlyph(highlight_segments, char_pos, 0, style.HighlightColor_X, last_style.HighlightColor_X, style.Highlight_B, last_style.Highlight_B, 0.0f, line_height);
        _InternalOnNewGlyph(underline_segments, char_pos, 0, style.UnderlineColor_X, last_style.UnderlineColor_X, style.Underline_B, last_style.Underline_B, line_height, line_height);
        _InternalOnNewGlyph(strikethrough_segments, char_pos, 0, style.StrikethroughColor_X, last_style.StrikethroughColor_X, style.Strikethrough_B, last_style.Strikethrough_B, strikethrough_offset_y, strikethrough_offset_y);
        _InternalOnNewGlyph(mask_segments, char_pos, 0, style.MaskColor_X, last_style.MaskColor_X, style.Mask_B, last_style.Mask_B, 0.0f, line_height);
      }
    }

    // skip to process the successive glyph
    // We will need to discard any incompleted positions(open positions)
    void OnSkipSuccessiveGlyph()
    {
      while (underline_segments.data.Size > 0 && underline_segments.data.back().EndPos.x == FLT_MAX)
      {
        underline_segments.data.pop_back();
      }
      while (strikethrough_segments.data.Size > 0 && strikethrough_segments.data.back().EndPos.x == FLT_MAX)
      {
        strikethrough_segments.data.pop_back();
      }
      while (highlight_segments.data.Size > 0 && highlight_segments.data.back().EndPos.x == FLT_MAX)
      {
        highlight_segments.data.pop_back();
      }
      while (mask_segments.data.Size > 0 && mask_segments.data.back().EndPos.x == FLT_MAX)
      {
        mask_segments.data.pop_back();
      }
    }

    // Internal method to be reused for highlight, underline, strikethrough and text mask
    void _InternalOnNewGlyph(Segments &segments, const char *glyph_end, unsigned int glyph_code, ImU32 color, ImU32 last_color,
                             bool current_on, bool last_on, float begin_pt_offset_y, float end_pt_offset_y)
    {
      // a segment can cover one or more glyphs
      if (!last_on && current_on) // new segment begin
      {
        if (glyph_end == text_end)
        {
          const ImFontGlyph *glyph = font->FindGlyph((ImWchar)glyph_code);
          const float cw = glyph == nullptr ? 0.0f : glyph->AdvanceX * (size / font->FontSize);

          //BHA bug: dump if only one char
          // original segments.data.back().EndPos = ImVec2(x + cw, y + end_pt_offset_y);
          // now:
          if (segments.data.size())
          {
            segments.data.back().EndPos = ImVec2(x + cw, y + end_pt_offset_y);
          }
          else
          {
            segments.data.push_back({ImVec2(x + cw, y + end_pt_offset_y)});
          }
        }
        else
        {
          if (color == 0)
          {
            color = text_col;
          }
          segments.data.push_back({ImVec2(x, y + begin_pt_offset_y), ImVec2(FLT_MAX, FLT_MAX), color});
          // segment begins from a new line
          if (segments.new_line)
          {
            segments.new_line = 0;
          }
        }
      }
      else if (last_on && current_on) // segment goes on to cover this glyph
      {
        // complete the segment if we reaches to the end of the text
        if (glyph_end == text_end)
        {
          const ImFontGlyph *glyph = font->FindGlyph((ImWchar)glyph_code);
          const float cw = glyph == nullptr ? 0.0f : glyph->AdvanceX * (size / font->FontSize);
          segments.data.back().EndPos = ImVec2(x + cw, y + end_pt_offset_y);
        }
        // handle new line caused by word wrap or \n
        else if (segments.new_line > 0)
        {
          // need end the segment with point we saved before new line starting
          segments.data.back().EndPos = ImVec2(segments.x_wrap_eol, segments.y_wrap_eol + end_pt_offset_y);
          // start a new segment with current point
          if (color == 0)
          {
            color = text_col;
          }
          segments.data.push_back({ImVec2(x, y + begin_pt_offset_y), ImVec2(FLT_MAX, FLT_MAX), color});
          segments.new_line = 0;
        }
        // color changes. this often happens when
        // 1. Late defined range with different color immediately follows an early defined range (or vice versa)
        // 2. Late defined range with different color partially overlaps an early defined range (or or vice versa)
        // 3. Late defined range with different color is inside an early defined range (or vice versa)
        // The all the net result looks like the early define drange paint first, the late defined range paint secondly.
        else if (color != last_color)
        {
          // need end the segment
          segments.data.back().EndPos = ImVec2(x, y + end_pt_offset_y);
          // start a new segment with current point
          if (color == 0)
          {
            color = text_col;
          }
          segments.data.push_back({ImVec2(x, y + begin_pt_offset_y), ImVec2(FLT_MAX, FLT_MAX), color});
        }
      }
      else if (last_on && !current_on && segments.data.Size > 0) // the current segment end
      {
        // special case, the segment ends with new line(word wrap or one or more \n) follwed
        if (segments.new_line > 0)
        {
          segments.data.back().EndPos = ImVec2(segments.x_wrap_eol, segments.y_wrap_eol + end_pt_offset_y);
          segments.new_line = 0;
        }
        // normal case, the segment ended withou new line followed
        else
        {
          segments.data.back().EndPos = ImVec2(x, y + end_pt_offset_y);
        }
      }
    }
  };

  // Create text customizaiton parser, it is only called if customization is applied.
  _CustomizationParser parser(text_col, size, text_begin, text_end, pFont, customization);

  while (s < text_end)
  {
    if (word_wrap_enabled)
    {
      // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
      if (!word_wrap_eol)
      {
        word_wrap_eol = pFont->CalcWordWrapPositionA(scale, s, text_end, wrap_width - (x - start_x));
      }

      if (s >= word_wrap_eol)
      {
        // save old position
        float old_x = x;
        float old_y = y;

        // update x, y
        x = start_x;
        y += line_height;

        word_wrap_eol = nullptr;
        s = S_CalcWordWrapNextLineStartA(s, text_end); // Wrapping skips upcoming blanks
        // process the new line event
        if (customization)
        {
          parser.OnNewLine(s, old_x, old_y);
        }
        continue;
      }
    }

    // Decode and advance source
    unsigned int c = (unsigned int)*s;
    const char *glyph_pos = s; // backup it for customization style check
    if (c < 0x80)
    {
      s += 1;
    }
    else
    {
      s += ImTextCharFromUtf8(&c, s, text_end);
      if (c == 0) // Malformed UTF-8?
      {
        if (customization)
        {
          parser.OnSkipSuccessiveGlyph();
        }
        break;
      }
    }

    // set default value
    col = text_col;

    if (c < 32)
    {
      if (c == '\n')
      {
        // save old position
        float old_x = x;
        float old_y = y;

        // update position
        x = start_x;
        y += line_height;

        // process the new line event
        if (customization)
        {
          parser.OnNewLine(s, old_x, old_y);
        }

        if (y > clip_rect.w)
        {
          if (customization)
          {
            parser.OnSkipSuccessiveGlyph();
          }
          break; // break out of main loop
        }
        continue;
      }
      if (c == '\r')
      {
        continue;
      }
    }

    if (customization)
    {
      col = parser.OnNewGlyph(glyph_pos, s, c, x, y);
    }

    const ImFontGlyph *glyph = pFont->FindGlyph((ImWchar)c);
    if (glyph == nullptr)
    {
      continue;
    }

    float char_width = glyph->AdvanceX * scale;
    if (glyph->Visible)
    {
      // We don't do a second finer clipping test on the Y axis as we've already skipped anything before clip_rect.y and exit once we pass clip_rect.w
      float x1 = x + glyph->X0 * scale;
      float x2 = x + glyph->X1 * scale;
      float y1 = y + glyph->Y0 * scale;
      float y2 = y + glyph->Y1 * scale;
      if (x1 <= clip_rect.z && x2 >= clip_rect.x)
      {
        // Render a character
        float u1 = glyph->U0;
        float v1 = glyph->V0;
        float u2 = glyph->U1;
        float v2 = glyph->V1;

        // CPU side clipping used to fit text in their frame when the frame is too small. Only does clipping for axis aligned quads.
        if (cpu_fine_clip)
        {
          if (x1 < clip_rect.x)
          {
            u1 = u1 + (1.0f - (x2 - clip_rect.x) / (x2 - x1)) * (u2 - u1);
            x1 = clip_rect.x;
          }
          if (y1 < clip_rect.y)
          {
            v1 = v1 + (1.0f - (y2 - clip_rect.y) / (y2 - y1)) * (v2 - v1);
            y1 = clip_rect.y;
          }
          if (x2 > clip_rect.z)
          {
            u2 = u1 + ((clip_rect.z - x1) / (x2 - x1)) * (u2 - u1);
            x2 = clip_rect.z;
          }
          if (y2 > clip_rect.w)
          {
            v2 = v1 + ((clip_rect.w - y1) / (y2 - y1)) * (v2 - v1);
            y2 = clip_rect.w;
          }
          if (y1 >= y2)
          {
            x += char_width;
            continue;
          }
        }

        // Support for untinted glyphs
        ImU32 glyph_col = glyph->Colored ? col_untinted : col;

        // We are NOT calling PrimRectUV() here because non-inlined causes too much overhead in a debug builds. Inlined here:
        {
          idx_write[0] = (ImDrawIdx)(vtx_current_idx);
          idx_write[1] = (ImDrawIdx)(vtx_current_idx + 1);
          idx_write[2] = (ImDrawIdx)(vtx_current_idx + 2);
          idx_write[3] = (ImDrawIdx)(vtx_current_idx);
          idx_write[4] = (ImDrawIdx)(vtx_current_idx + 2);
          idx_write[5] = (ImDrawIdx)(vtx_current_idx + 3);
          vtx_write[0].pos.x = x1;
          vtx_write[0].pos.y = y1;
          vtx_write[0].col = glyph_col;
          vtx_write[0].uv.x = u1;
          vtx_write[0].uv.y = v1;
          vtx_write[1].pos.x = x2;
          vtx_write[1].pos.y = y1;
          vtx_write[1].col = glyph_col;
          vtx_write[1].uv.x = u2;
          vtx_write[1].uv.y = v1;
          vtx_write[2].pos.x = x2;
          vtx_write[2].pos.y = y2;
          vtx_write[2].col = glyph_col;
          vtx_write[2].uv.x = u2;
          vtx_write[2].uv.y = v2;
          vtx_write[3].pos.x = x1;
          vtx_write[3].pos.y = y2;
          vtx_write[3].col = glyph_col;
          vtx_write[3].uv.x = u1;
          vtx_write[3].uv.y = v2;
          vtx_write += 4;
          vtx_current_idx += 4;
          idx_write += 6;
        }
      }
    }
    x += char_width;
  }

  // Give back unused vertices (clipped ones, blanks) ~ this is essentially a PrimUnreserve() action.
  draw_list_text->VtxBuffer.Size = (int)(vtx_write - draw_list_text->VtxBuffer.Data); // Same as calling shrink()
  draw_list_text->IdxBuffer.Size = (int)(idx_write - draw_list_text->IdxBuffer.Data);
  draw_list_text->CmdBuffer[draw_list_text->CmdBuffer.Size - 1].ElemCount -= (idx_expected_size - draw_list_text->IdxBuffer.Size);
  draw_list_text->_VtxWritePtr = vtx_write;
  draw_list_text->_IdxWritePtr = idx_write;
  draw_list_text->_VtxCurrentIdx = vtx_current_idx;

  if (customization)
  {
    // Draw the highlight firt because it's the backgroud of the text
    for (int i = 0; i < parser.highlight_segments.data.Size; i++)
    {
      IM_ASSERT_USER_ERROR(parser.highlight_segments.data.back().EndPos.x != FLT_MAX && parser.highlight_segments.data.back().EndPos.y != FLT_MAX, "EndPos is expected no valide value.");
      ImVec2 begin = parser.highlight_segments.data[i].BeginPos;
      ImVec2 end = parser.highlight_segments.data[i].EndPos;
      ImColor color = parser.highlight_segments.data[i].Color;
      draw_list->AddRectFilled(begin, end, color);
    }

    // Append draw_list_text to the main draw_lists
    {
      unsigned int old_inx_buffer_sz = draw_list->IdxBuffer.Size;
      unsigned int old_vtx_buffer_sz = draw_list->VtxBuffer.Size;
      draw_list->PrimReserve(draw_list_text->IdxBuffer.Size, draw_list_text->VtxBuffer.Size);
      // draw_list_text->VtxBuffer buffer can be memcopied, it contains no relative data
      memcpy(draw_list->_VtxWritePtr, draw_list_text->VtxBuffer.Data, draw_list_text->VtxBuffer.Size * sizeof(ImDrawVert));
      // draw_list_text->IdxBuffer buffer cann't because the index value in the IdxBuffer points to draw_list_text->VtxBuffer. It need aligned to draw_list->VtxBuffer.
      for (int i = 0; i < draw_list_text->IdxBuffer.Size; i++)
      {
        draw_list->IdxBuffer[old_inx_buffer_sz + i] = (ImDrawIdx)(old_vtx_buffer_sz + draw_list_text->IdxBuffer[i]);
      }
      draw_list->_VtxWritePtr += draw_list_text->VtxBuffer.Size;
      draw_list->_IdxWritePtr += draw_list_text->IdxBuffer.Size;
      draw_list->_VtxCurrentIdx += vtx_current_idx;

      // Free draw_list_text. It is created for customized text only
      delete draw_list_text;
    }

    // Draw the underlines
    for (int i = 0; i < parser.underline_segments.data.Size; i++)
    {
      IM_ASSERT_USER_ERROR(parser.underline_segments.data.back().EndPos.x != FLT_MAX && parser.underline_segments.data.back().EndPos.y != FLT_MAX, "EndPos is expected no valide value.");
      ImVec2 begin = parser.underline_segments.data[i].BeginPos;
      ImVec2 end = parser.underline_segments.data[i].EndPos;
      ImColor color = parser.underline_segments.data[i].Color;
      // The underlines is about 6.5% line height
      float thickness = (float)ceill(ImMax<float>(1.0f, line_height * 6.5f / 100.0f));
      draw_list->AddLine(begin, end, color, thickness);
    }

    // Draw the strikethrough lines
    for (int i = 0; i < parser.strikethrough_segments.data.Size; i++)
    {
      IM_ASSERT_USER_ERROR(parser.strikethrough_segments.data.back().EndPos.x != FLT_MAX && parser.strikethrough_segments.data.back().EndPos.y != FLT_MAX, "EndPos is expected no valide value.");
      // if (parser.strikethrough_segments.data.back().EndPos.x == FLT_MAX) printf("***********EndPos == FLT_MAX\n");
      ImVec2 begin = parser.strikethrough_segments.data[i].BeginPos;
      ImVec2 end = parser.strikethrough_segments.data[i].EndPos;
      ImColor color = parser.strikethrough_segments.data[i].Color;
      // The strkethrough is about 4.5% line height
      float thickness = (float)ceill(ImMax<float>(1.0f, line_height * 4.5f / 100.0f));
      draw_list->AddLine(begin, end, color, thickness);
    }

    // Draw the mask last because is used to mask the text
    for (int i = 0; i < parser.mask_segments.data.Size; i++)
    {
      IM_ASSERT_USER_ERROR(parser.mask_segments.data.back().EndPos.x != FLT_MAX && parser.mask_segments.data.back().EndPos.y != FLT_MAX, "EndPos is expected no valide value.");
      ImVec2 begin = parser.mask_segments.data[i].BeginPos;
      ImVec2 end = parser.mask_segments.data[i].EndPos;
      ImColor color = parser.mask_segments.data[i].Color;
      draw_list->AddRectFilled(begin, end, color);
    }
  }
}

void ImDrawListAddText(const ImFont *font, float font_size, const ImVec2 &pos, ImU32 col, const char *text_begin, const char *text_end, float wrap_width, const ImVec4 *cpu_fine_clip_rect, const Bof_ImGui_ImTextCustomization *customization)
{
  ImGuiContext &g = *GImGui;
  if ((col & IM_COL32_A_MASK) == 0)
  {
    return;
  }

  if (text_end == nullptr)
  {
    text_end = text_begin + strlen(text_begin);
  }
  if (text_begin == text_end)
  {
    return;
  }

  ImDrawList *pThis = ImGui::GetWindowDrawList();
  ImDrawListSharedData *_Data = &g.DrawListSharedData;
  ImDrawCmdHeader *_CmdHeader = &pThis->_CmdHeader;
  // Pull default font/size from the shared ImDrawListSharedData instance
  if (font == nullptr)
  {
    font = _Data->Font;
  }
  if (font_size == 0.0f)
  {
    font_size = _Data->FontSize;
  }

  assert(font->ContainerAtlas->TexID == _CmdHeader->TextureId); // Use high-level ImGui::PushFont() or low-level ImDrawList::PushTextureId() to change font.

  ImVec4 clip_rect = _CmdHeader->ClipRect;
  if (cpu_fine_clip_rect)
  {
    clip_rect.x = ImMax(clip_rect.x, cpu_fine_clip_rect->x);
    clip_rect.y = ImMax(clip_rect.y, cpu_fine_clip_rect->y);
    clip_rect.z = ImMin(clip_rect.z, cpu_fine_clip_rect->z);
    clip_rect.w = ImMin(clip_rect.w, cpu_fine_clip_rect->w);
  }
  // font->RenderText(pThis, font_size, pos, col, clip_rect, text_begin, text_end, wrap_width, cpu_fine_clip_rect != nullptr, customization);
  ImFontRenderText(font, pThis, font_size, pos, col, clip_rect, text_begin, text_end, wrap_width, cpu_fine_clip_rect != nullptr, customization);
}

// Internal ImGui functions to render text
// RenderText***() functions calls ImDrawList::AddText() calls ImBitmapFont::RenderText()
void ImGuiRenderText(ImVec2 pos, const char *text, const char *text_end, bool hide_text_after_hash, const Bof_ImGui_ImTextCustomization *customization)
{
  ImGuiContext &g = *GImGui;
  ImGuiWindow *window = g.CurrentWindow;

  // Hide anything after a '##' string
  const char *text_display_end;
  if (hide_text_after_hash)
  {
    text_display_end = ImGui::FindRenderedTextEnd(text, text_end);
  }
  else
  {
    if (!text_end)
    {
      text_end = text + strlen(text); // FIXME-OPT
    }
    text_display_end = text_end;
  }

  if (text != text_display_end)
  {
    // window->DrawList->AddText(g.Font, g.FontSize, pos, ImGui::GetColorU32(ImGuiCol_Text), text, text_display_end, 0.0f, nullptr, customization);
    ImDrawListAddText(g.Font, g.FontSize, pos, ImGui::GetColorU32(ImGuiCol_Text), text, text_display_end, 0.0f, nullptr, customization);
    if (g.LogEnabled)
    {
      ImGui::LogRenderedText(&pos, text, text_display_end);
    }
  }
}

void ImGuiRenderTextWrapped(ImVec2 pos, const char *text, const char *text_end, float wrap_width, const Bof_ImGui_ImTextCustomization *customization)
{
  ImGuiContext &g = *GImGui;
  ImGuiWindow *window = g.CurrentWindow;

  if (!text_end)
  {
    text_end = text + strlen(text); // FIXME-OPT
  }

  if (text != text_end)
  {
    //    window->DrawList->AddText(g.Font, g.FontSize, pos, ImGui::GetColorU32(ImGui::ImGuiCol_Text), text, text_end, wrap_width, nullptr, customization);
    ImDrawListAddText(g.Font, g.FontSize, pos, ImGui::GetColorU32(ImGuiCol_Text), text, text_end, wrap_width, nullptr, customization);
    if (g.LogEnabled)
    {
      ImGui::LogRenderedText(&pos, text, text_end);
    }
  }
}

void ImGuiTextEx(const char *_pTextStart_c, const char *_pTextEnd_c, ImGuiTextFlags _Flags_S32, const Bof_ImGui_ImTextCustomization *_pTextCustomization_X)
{
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems)
  {
    return;
  }
  ImGuiContext &g = *GImGui;

  // Accept null ranges
  if (_pTextStart_c == _pTextEnd_c)
  {
    _pTextStart_c = _pTextEnd_c = "";
  }

  // Calculate length
  const char *text_begin = _pTextStart_c;
  if (_pTextEnd_c == nullptr)
  {
    _pTextEnd_c = _pTextStart_c + strlen(_pTextStart_c); // FIXME-OPT
  }

  const ImVec2 text_pos(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
  const float wrap_pos_x = window->DC.TextWrapPos;
  const bool wrap_enabled = (wrap_pos_x >= 0.0f);
  if (_pTextEnd_c - _pTextStart_c <= 2000 || wrap_enabled)
  {
    // Common case
    const float wrap_width = wrap_enabled ? ImGui::CalcWrapWidthForPos(window->DC.CursorPos, wrap_pos_x) : 0.0f;
    const ImVec2 text_size = ImGui::CalcTextSize(text_begin, _pTextEnd_c, false, wrap_width);

    ImRect bb(text_pos, text_pos + text_size);
    ImGui::ItemSize(text_size, 0.0f);
    if (!ImGui::ItemAdd(bb, 0))
    {
      return;
    }

    // Render (we don't hide text after ## in this end-user function)
    ImGuiRenderTextWrapped(bb.Min, text_begin, _pTextEnd_c, wrap_width, _pTextCustomization_X);
  }
  else
  {
    // Long text!
    // Perform manual coarse clipping to optimize for long multi-line text
    // - From this point we will only compute the width of lines that are visible. Optimization only available when word-wrapping is disabled.
    // - We also don't vertically center the text within the line full height, which is unlikely to matter because we are likely the biggest and only item on the line.
    // - We use memchr(), pay attention that well optimized versions of those str/mem functions are much faster than a casually written loop.
    const char *line = _pTextStart_c;
    const float line_height = ImGui::GetTextLineHeight();
    ImVec2 text_size(0, 0);

    // Lines to skip (can't skip when logging text)
    ImVec2 pos = text_pos;
    if (!g.LogEnabled)
    {
      int lines_skippable = (int)((window->ClipRect.Min.y - text_pos.y) / line_height);
      if (lines_skippable > 0)
      {
        int lines_skipped = 0;
        while (line < _pTextEnd_c && lines_skipped < lines_skippable)
        {
          const char *line_end = (const char *)memchr(line, '\n', _pTextEnd_c - line);
          if (!line_end)
          {
            line_end = _pTextEnd_c;
          }
          if ((_Flags_S32 & ImGuiTextFlags_NoWidthForLargeClippedText) == 0)
          {
            text_size.x = ImMax(text_size.x, ImGui::CalcTextSize(line, line_end).x);
          }
          line = line_end + 1;
          lines_skipped++;
        }
        pos.y += lines_skipped * line_height;
      }
    }

    // Lines to render
    if (line < _pTextEnd_c)
    {
      ImRect line_rect(pos, pos + ImVec2(FLT_MAX, line_height));
      while (line < _pTextEnd_c)
      {
        if (ImGui::IsClippedEx(line_rect, 0))
        {
          break;
        }

        const char *line_end = (const char *)memchr(line, '\n', _pTextEnd_c - line);
        if (!line_end)
        {
          line_end = _pTextEnd_c;
        }
        text_size.x = ImMax(text_size.x, ImGui::CalcTextSize(line, line_end).x);
        ImGuiRenderText(pos, line, line_end, false, _pTextCustomization_X);
        line = line_end + 1;
        line_rect.Min.y += line_height;
        line_rect.Max.y += line_height;
        pos.y += line_height;
      }

      // Count remaining lines
      int lines_skipped = 0;
      while (line < _pTextEnd_c)
      {
        const char *line_end = (const char *)memchr(line, '\n', _pTextEnd_c - line);
        if (!line_end)
        {
          line_end = _pTextEnd_c;
        }
        if ((_Flags_S32 & ImGuiTextFlags_NoWidthForLargeClippedText) == 0)
        {
          text_size.x = ImMax(text_size.x, ImGui::CalcTextSize(line, line_end).x);
        }
        line = line_end + 1;
        lines_skipped++;
      }
      pos.y += lines_skipped * line_height;
    }
    text_size.y = (pos - text_pos).y;

    ImRect bb(text_pos, text_pos + text_size);
    ImGui::ItemSize(text_size, 0.0f);
    ImGui::ItemAdd(bb, 0);
  }
}


END_BOF_NAMESPACE()