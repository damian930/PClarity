#ifndef PROFILER_STUB_CPP
#define PROFILER_STUB_CPP

#include "profiler/stub/profiler_stub.h"

///////////////////////////////////////////////////////////
// - State #[Per backend] 
//
void prof_init() { }
void prof_release() { }

///////////////////////////////////////////////////////////
// - Profiling #[Per backend] 
//
void prof_begin_prof_scope_fmt(const char* fmt, ...) { }
void prof_end_prof_scope() { }


#endif
