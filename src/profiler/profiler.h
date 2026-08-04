#ifndef PROFILER_H
#define PROFILER_H

#include "core/core_include.h"
#include "os/win32.h" // DD: This is for time

struct Prof_State {
  Arena* state_arena;
  U64 addr_to_specific_backend_struct;
};

// TODO: Mark what is per backend and what is stable on top of it

// - State variables
extern global Prof_State* __prof_g_state;

// - State accessors
Prof_State* prof_get_state();
void prof_set_state(Prof_State*);

// - State #[Per backend]
void prof_init();
void prof_release();

// - Profiling #[Per backend]
void prof_begin_prof_scope_fmt(const char* fmt, ...);
void prof_end_prof_scope();

// - Scoped Macros 
#define ProfBeginGroup(name_cstr)       prof_begin_prof_scope_fmt(name_cstr)
#define ProfBeginGroupF(name_cstr, ...) prof_begin_prof_scope_fmt(name_cstr, ##__VA_ARGS__)
#define ProfBeginFunc()                 ProfBeginGroup(__FUNCTION__)
#define ProfEndGroup()                  prof_end_prof_scope()

#define ProfGroup(name_cstr)       DeferLoop(ProfBeginGroup(name_cstr), ProfEndGroup())
#define ProfGroupF(name_cstr, ...) DeferLoop(ProfBeginGroupF(name_cstr, ##__VA_ARGS__), ProfEndGroup())

// - Including header file for specific back end
#if CORE_PROFILER__SPALL
  #include "profiler/spall/profiler_spall.h"
#else
  #include "profiler/stub/profiler_stub.h"
#endif

#endif