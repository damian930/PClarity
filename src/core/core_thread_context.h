#ifndef CORE_THREAD_CONTEXT_H
#define CORE_THREAD_CONTEXT_H

#include "core/core_base.h"
#include "core/core_arena.h"

// todo: There is no default id or something, might be consfusing

struct Thread_context {
  B32 is_initialised;
  Arena* scrach_arenas_buffer[3];
};

// todo: What does static do here excatly, if we compile as separate obj files
volatile static U64 __next_thread_id = 0; 
per_thread static Thread_context __per_thread_context = {};

tu_specific Thread_context* get_thread_context();
tu_specific void set_thread_context(Thread_context* other);

tu_specific void allocate_thread_context();
tu_specific void release_thread_context();

tu_specific U64 get_thread_id();

tu_specific Scratch get_scratch(Arena** conflic_arenas, U64 n_conflict_arenas);
tu_specific void end_scratch(Scratch* scratch);
tu_specific void reset_scratch(Scratch* scratch);
#define ScratchLoop(name, conflic_arenas, n_conflict_arenas) DeferInitReleaseLoop(Scratch name = get_scratch(conflic_arenas, n_conflict_arenas), end_scratch(&name))

#endif