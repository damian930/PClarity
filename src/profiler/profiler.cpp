#ifndef PROFILER_CPP
#define PROFILER_CPP

#include "core/core_include.cpp"

///////////////////////////////////////////////////////////
// - State accessors
//
global Prof_State* __prof_g_state = 0;

///////////////////////////////////////////////////////////
// - State accessors
//
Prof_State* prof_get_state()
{
	return __prof_g_state;
}

void prof_set_state(Prof_State* state)
{
	__prof_g_state = state;
}

///////////////////////////////////////////////////////////
// - Including cpp file for specific back end
//
#if CORE_PROFILER__SPALL
  #include "profiler/spall/profiler_spall.cpp"
#else
  #include "profiler/stub/profiler_stub.cpp"
#endif

#endif