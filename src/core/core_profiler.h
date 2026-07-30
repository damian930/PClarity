#ifndef CORE_PROFILER_H
#define CORE_PROFILER_H

#include "core/core_base.h"
#include "core/core_arena.h"

void profiler_init();
void profiler_release();
U64  profiler_time_in_ns();
void spall_buffer_begin_fmt(const char* fmt, ...);

#define ProfBeginGroup(name_cstr)      
#define ProfBeginGroupF(name_cstr, ...)
#define ProfBeginFunc()                
#define ProfEndGroup()                 

#define ProfGroup(name_cstr)       
#define ProfGroupF(name_cstr, ...)

#endif