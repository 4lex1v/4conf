
static void custom_render_buffer (Application_Links *app, View_ID view_id, Face_ID face_id, Buffer_ID buffer, Text_Layout_ID text_layout_id, Rect_f32 rect) {
  ProfileScope(app, "render buffer");
    
  View_ID active_view = get_active_view(app, Access_Always);
  b32 is_active_view = (active_view == view_id);
  Rect_f32 prev_clip = draw_set_clip(app, rect);
    
  // NOTE(allen): Token colorizing
  Token_Array token_array = get_token_array_from_buffer(app, buffer);
  if (token_array.tokens != 0){
    draw_cpp_token_colors(app, text_layout_id, &token_array);
        
    // NOTE(allen): Scan for TODOs and NOTEs
    if (global_config.use_comment_keyword) {
      Comment_Highlight_Pair pairs[] = {
        {string_u8_litexpr("NOTE"), finalize_color(defcolor_comment_pop, 0)},
        {string_u8_litexpr("TODO"), finalize_color(defcolor_comment_pop, 1)},
      };
            
      draw_comment_highlights(app, buffer, text_layout_id, &token_array, pairs, ArrayCount(pairs));
    }
  }
  else {
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    paint_text_color_fcolor(app, text_layout_id, visible_range, fcolor_id(defcolor_text_default));
  }
    
  i64 cursor_pos = view_correct_cursor(app, view_id);
  view_correct_mark(app, view_id);
    
  // NOTE(allen): Scope highlight
  if (global_config.use_scope_highlight){
    Color_Array colors = finalize_color_array(defcolor_back_cycle);
    draw_scope_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
  }
    
  if (global_config.use_error_highlight || global_config.use_jump_highlight){
    // NOTE(allen): Error highlight
    String_Const_u8 name = string_u8_litexpr("*compilation*");
    Buffer_ID compilation_buffer = get_buffer_by_name(app, name, Access_Always);
    if (global_config.use_error_highlight){
      draw_jump_highlights(app, buffer, text_layout_id, compilation_buffer,
                           fcolor_id(defcolor_highlight_junk));
    }
        
    // NOTE(allen): Search highlight
    if (global_config.use_jump_highlight){
      Buffer_ID jump_buffer = get_locked_jump_buffer(app);
      if (jump_buffer != compilation_buffer){
        draw_jump_highlights(app, buffer, text_layout_id, jump_buffer,
                             fcolor_id(defcolor_highlight_white));
      }
    }
  }
    
  // NOTE(allen): Color parens
  if (global_config.use_paren_helper){
    Color_Array colors = finalize_color_array(defcolor_text_cycle);
    draw_paren_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
  }
    
  // NOTE(allen): Line highlight
  if (global_config.highlight_line_at_cursor && is_active_view){
    i64 line_number = get_line_number_from_pos(app, buffer, cursor_pos);
    draw_line_highlight(app, text_layout_id, line_number,
                        fcolor_id(defcolor_highlight_cursor_line));
  }
    
  // NOTE(allen): Cursor shape
  Face_Metrics metrics = get_face_metrics(app, face_id);
  f32 cursor_roundness = 1.0f;//(metrics.normal_advance*0.5f)*0.9f;
  f32 mark_thickness = 2.f;
    
  // NOTE(allen): Cursor
  switch (fcoder_mode){
    case FCoderMode_Original:
    {
      draw_original_4coder_style_cursor_mark_highlight(app, view_id, is_active_view, buffer, text_layout_id, cursor_roundness, mark_thickness);
    }break;
    case FCoderMode_NotepadLike:
    {
      draw_notepad_style_cursor_highlight(app, view_id, buffer, text_layout_id, cursor_roundness);
    }break;
  }
    
  // NOTE(allen): Fade ranges
  paint_fade_ranges(app, text_layout_id, buffer, view_id);
    
  // NOTE(allen): put the actual text on the actual screen
  draw_text_layout_default(app, text_layout_id);
    
  draw_set_clip(app, prev_clip);
}

/**
 * Unlike the default function this would draw a bigger margin around the buffer
 */
static Rect_f32 custom_background_and_margin (Application_Links *app, View_ID view, b32 is_active_view) {
  FColor margin_color = get_panel_margin_color(is_active_view ? UIHighlight_Active : UIHighlight_None);
  ARGB_Color margin_argb = fcolor_resolve(margin_color);
  ARGB_Color back_argb = fcolor_resolve(fcolor_id(defcolor_back));

  Rect_f32 view_rect = view_get_screen_rect(app, view);
  Rect_f32 inner = rect_inner(view_rect, 5.0f);
  draw_rectangle(app, inner, 0.f, back_argb);
  draw_margin(app, view_rect, inner, margin_argb);

  return inner;
}

static void custom_render_caller (Application_Links *app, Frame_Info frame_info, View_ID view_id) {
  ProfileScope(app, "default render caller");
  View_ID active_view = get_active_view(app, Access_Always);
  b32 is_active_view = (active_view == view_id);
    
  Rect_f32 region = custom_background_and_margin(app, view_id, is_active_view);
  Rect_f32 prev_clip = draw_set_clip(app, region);
    
  Buffer_ID buffer = view_get_buffer(app, view_id, Access_Always);
  Face_ID face_id = get_face_id(app, buffer);
  Face_Metrics face_metrics = get_face_metrics(app, face_id);
  f32 line_height = face_metrics.line_height;
  f32 digit_advance = face_metrics.decimal_digit_advance;
    
  // NOTE(allen): file bar
  b64 showing_file_bar = false;
  if (view_get_setting(app, view_id, ViewSetting_ShowFileBar, &showing_file_bar) && showing_file_bar){
    Rect_f32_Pair pair = layout_file_bar_on_bot(region, line_height);
    draw_file_bar(app, view_id, buffer, face_id, pair.max);
    region = pair.min;
  }
    
  Buffer_Scroll scroll = view_get_buffer_scroll(app, view_id);
    
  Buffer_Point_Delta_Result delta = delta_apply(app, view_id,
                                                frame_info.animation_dt, scroll);
  if (!block_match_struct(&scroll.position, &delta.point)){
    block_copy_struct(&scroll.position, &delta.point);
    view_set_buffer_scroll(app, view_id, scroll, SetBufferScroll_NoCursorChange);
  }
  if (delta.still_animating){
    animate_in_n_milliseconds(app, 0);
  }
    
  // NOTE(allen): query bars
  {
    Query_Bar *space[32];
    Query_Bar_Ptr_Array query_bars = {};
    query_bars.ptrs = space;
    if (get_active_query_bars(app, view_id, ArrayCount(space), &query_bars)){
      for (i32 i = 0; i < query_bars.count; i += 1){
        Rect_f32_Pair pair = layout_query_bar_on_top(region, line_height, 1);
        draw_query_bar(app, query_bars.ptrs[i], face_id, pair.min);
        region = pair.max;
      }
    }
  }
    
  // NOTE(allen): FPS hud
  if (show_fps_hud){
    Rect_f32_Pair pair = layout_fps_hud_on_bottom(region, line_height);
    draw_fps_hud(app, frame_info, face_id, pair.max);
    region = pair.min;
    animate_in_n_milliseconds(app, 1000);
  }
    
  // NOTE(allen): layout line numbers
  Rect_f32 line_number_rect = {};
  if (global_config.show_line_number_margins){
    Rect_f32_Pair pair = layout_line_number_margin(app, buffer, region, digit_advance);
    line_number_rect = pair.min;
    region = pair.max;
  }
    
  // NOTE(allen): begin buffer render
  Buffer_Point buffer_point = scroll.position;
  Text_Layout_ID text_layout_id = text_layout_create(app, buffer, region, buffer_point);
    
  // NOTE(allen): draw line numbers
  if (global_config.show_line_number_margins){
    draw_line_number_margin(app, view_id, buffer, face_id, text_layout_id, line_number_rect);
  }
    
  // NOTE(allen): draw the buffer
  custom_render_buffer(app, view_id, face_id, buffer, text_layout_id, region);
    
  text_layout_free(app, text_layout_id);
  draw_set_clip(app, prev_clip);
}
