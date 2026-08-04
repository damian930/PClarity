#ifndef PCLARITY_UI_DLL_CODE_H
#define PCLARITY_UI_DLL_CODE_H

#include "profiler/profiler.h"

#include "pclarity/pclarity_commons.h"

struct PCL_UI_Dll_context {
  OS_State*  os_state;
  Thread_context* thread_context;
  Prof_State* prof_state;
  D3D_State* r_state;
  FP_State*  font_provider_state;
  D_State*   draw_state;
  UI_State*  ui_state;
  // TODO: You cant profile the ui yet, since you dont have a way to set the priler across the dll boundary, fix that 
};

#define PCL_BUILD_UI_FUNC_DEF(name) void (name)(FP_Font font, PCL_State* pcl, PCL_UI_Dll_context dll_context, PCL_Debug_data_for_ui debug_data)
typedef PCL_BUILD_UI_FUNC_DEF(PCL_Build_ui_func);
#define PCL_BUILD_UI__FUNC_FOR_EXPORT__NAME pcl_build_ui__func_for_export

// - Code to export outside the dll 
extern "C" __declspec( dllexport )
PCL_BUILD_UI_FUNC_DEF(PCL_BUILD_UI__FUNC_FOR_EXPORT__NAME);

// - UI
V4F32 pcl_color_from_name(PCL_Color_name color_name, PCL_State* pcl);
F32 pcl_ui_slider(F32 value, RangeF32 range_for_value, Str8 id);
void pcl_scroll_bar(UI_Size size_x, UI_Size size_y, Axis2 scroll_axis, Str8 scroll_bar_id, F32 outer_vp_size, F32 outer_content_size, F32 outer_vp_offset, F32* out_new_scroll, B32* is_new_offset);
UI_Actions pcl_ui_table_header(Str8 id, UI_Size size_in_x, UI_Size size_in_y, PCL_Table_header header, U64 generation_for_currently_selected_header);

#endif