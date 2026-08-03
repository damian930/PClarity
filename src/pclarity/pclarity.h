#ifndef PCLARITY_H
#define PCLARITY_H

#include "core/core_include.h"
#include "core/core_include.cpp"

#include "os/win32.h"
#include "os/win32.cpp"

#include "render/render.h"
#include "render/render.cpp"

#include "draw/draw.h"
#include "draw/draw.cpp"

#include "font_provider/font_provider.h"
#include "font_provider/font_provider.cpp"

#include "ui/ui_core.h"
#include "ui/ui_core.cpp"

#include "ui/widgets/ui_widgets.h"
#include "ui/widgets/ui_widgets.cpp"

#include "pclarity/pclarity_commons.h"

PCL_State pcl_init();
void pcl_frame_update(PCL_State* PCL);

void pcl_add_header_to_the_end_of_table(PCL_State* pcl, PCL_Table_header_kind header_kind, F32 flex_value);
void pcl_add_header_after_index_of_table(PCL_State* pcl, PCL_Table_header_kind header_kind, F32 flex_value, U64 add_after_index);

#endif









