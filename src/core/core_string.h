#ifndef _CORE_STRING_H_
#define _CORE_STRING_H_

#include "string.h" // For memset and strlen
#include "wchar.h"  // For wstrlen

#include "core/core_base.h"
#include "core/core_arena.h"

/* todo:
  [ ] - improve the section for str8_list, right now only 1 can exist.
        api asserts if another one gets created.
  [ ] - search for other todos in the code ))
*/

struct Str8 {
  U8* data;
  U64 count;
};

struct Str8_node {
  Str8 str;
  Str8_node* next;
  Str8_node* prev;
};

struct Str8_list {
  Str8 str;
  Str8_node* first;
  Str8_node* last;
  U64 node_count;
  U64 codepoint_count;
  
  B8 is_temp_section_present;
  U64 nodes_in_temp_section;
  U64 codepoints_in_temp_section;
};  

// Note: This only works for ascii, since i dont yet support unicode
enum Str8_match : U32 {
  Str8_match__NONE            = 0,
  Str8_match__ignore_case     = (1 << 0),
  Str8_match__normalise_slash = (1 << 1),
};
typedef U32 Str8_match_flags;

struct Str16 {
  wchar_t* data;
  U64 count;
};

#define Str8FmtArg(str) (int)((str).count), ((str).data) // NOTE(S): Use this for variadic functions where the format specifier is "%.*s" meaning an int value (width) is provided before the char string. 

// - data buffer
typedef Str8 Data_buffer;
tu_specific Data_buffer data_buffer_make(Arena* arena, U64 count);

// - str8 makers
#define MakeSureClit(clit) "" clit "" 
#define Str8FromC(clit) Str8 { (U8*)MakeSureClit(clit), ArrayCount(clit) - 1 }
#define String8(clit) Str8FromC(clit) 
tu_specific Str8 str8_manual_alloc(Arena* arena, U8* str, U64 count);
tu_specific Str8 str8_manual_view(U8* buffer, U64 count);
//
tu_specific Str8 str8_manual_alloc_nt(Arena* arena, U8* str);
tu_specific Str8 str8_manual_view_nt(U8* buffer);
//
tu_specific Str8 str8_copy(Arena* arena, Str8 str);
//
tu_specific Str8 str8_from_list(Arena* arena, Str8_list* list); // todo: There is not point in the list beeing passed by a pointer here. It shoud either be by value. Or a const pointer
tu_specific Str8 str8_from_list_ex(Arena* arena, const Str8_list list, Str8 str_to_put_before, Str8 str_to_put_between, Str8 str_to_put_after); 
tu_specific Str8 str8_fmt(Arena* arena, const char* fmt, ...);
tu_specific Str8 str8_valist(Arena* arena, const char* fmt, va_list valist);
tu_specific char* cstr_from_str8(Arena* arena, Str8 str);
tu_specific U64 str8_fmt_count(const char* fmt, ...);
tu_specific U64 __str8_fmt_count_valist(const char* fmt, va_list valist);

// - str16 makes
tu_specific Str16 str16_manual_alloc(Arena* arena, wchar_t* wstr);
tu_specific Str16 str16_manual_view(wchar_t* wstr);
//
tu_specific Str16 str16_manual_alloc_nt(Arena* arena, wchar_t* wstr);
tu_specific Str16 str16_manual_view_nt(wchar_t* wstr);

// - string editing
tu_specific Str8 str8_substring(Str8 str, U64 start_index, U64 end_index);
tu_specific Str8 str8_substring_range(Str8 str, RangeU64 range);
tu_specific B32 str8_match(Str8 str, Str8 other, Str8_match_flags flags);
tu_specific B32 str8_is_substring(Str8 str, Str8 test_sub, Str8_match_flags flags);
tu_specific B32 str8_is_front(Str8 str, Str8 other, Str8_match_flags flags);
tu_specific B32 str8_is_back(Str8 str, Str8 other, Str8_match_flags flags);
tu_specific Str8 str8_chop_front(Str8 str, U64 end_index);
tu_specific Str8 str8_chop_back(Str8 str, U64 n_chars_to_chop);
tu_specific Str8 str8_chop_front_if_match(Str8 str, Str8 other, Str8_match_flags flags);
tu_specific Str8 str8_chop_back_if_match(Str8 str, Str8 other, Str8_match_flags flags);
tu_specific Str8 str8_front(Str8 str, U64 n);
tu_specific Str8 str8_back(Str8 str, U64 n);
tu_specific Str8 str8_trim_front(Str8 str);
tu_specific Str8 str8_trim_back(Str8 str);
tu_specific Str8 str8_trim(Str8 str);
tu_specific Str8 str8_to_lower(Arena* arena, Str8 str);
tu_specific Str8 str8_to_upper(Arena* arena, Str8 str);

// - chars
tu_specific B32 char_is_lower(U8 ch);
tu_specific B32 char_is_upper(U8 ch);
tu_specific U8 char_to_lower(U8 ch); 
tu_specific U8 char_to_upper(U8 ch); 
tu_specific U8 char_normalise_slash(U8 ch);
tu_specific B32 char_is_alpha(U8 ch);
tu_specific B32 char_is_numeric(U8 ch);
tu_specific B32 char_is_alnum(U8 ch);
tu_specific B32 char_is_word_char(U8 ch);

// - list stuff
tu_specific Str8_list str8_list_copy_shallow(Arena* arena, Str8_list other);
tu_specific Str8_list str8_list_copy_deep(Arena* arena, Str8_list other);
tu_specific void str8_list_append_view(Arena* arena, Str8_list* list, Str8 str);
tu_specific void str8_list_append_copy(Arena* arena, Str8_list* list, Str8 str); 
tu_specific Str8_list str8_split_ex(Arena* arena, Str8 str, Str8 spliter, Str8_match_flags flags, B32 include_empty_once_after_split);
tu_specific Str8_list str8_split(Arena* arena, Str8 str, Str8 spliter, Str8_match_flags flags);
// NOTE: These are not done and are kind of old
tu_specific void str8_list_begin_temp_section(Str8_list* list); // todo: Fix these up, only 1 section is possible at a time
tu_specific void str8_list_end_temp_section(Str8_list* list);
#define Str8ListSection(list_p) DeferLoop(str8_list_begin_temp_section((list_p)), str8_list_end_temp_section((list_p)))

// - other
tu_specific Str8 str8_get_filename(Str8 str);
tu_specific Str8 str8_get_basename(Str8 str);
tu_specific Str8 str8_chop_extenstion(Str8 str);
tu_specific Str8 str8_from_month(Month month);
tu_specific Str8 str8_from_day(Day day);
tu_specific Comparison str8_compare(Str8 str, Str8 other, Str8_match_flags flags);

#define MakeSureClit(clit) "" clit "" 
#define Str8FromC(clit) Str8 { (U8*)MakeSureClit(clit), ArrayCount(clit) - 1 }
#define String(clit) Str8FromC(clit) // note: Testing a faster macro
tu_specific Str8 str8_manual_view(U8* buffer, U64 count);
tu_specific Str8 str8_manual_alloc(Arena* arena, U8* str, U64 len);
tu_specific Str8 str8_from_cstr(Arena* arena, U8* str);
tu_specific Str8 str8_from_cstr_view(U8* cstr);
tu_specific Str8 str8_copy(Arena* arena, Str8 str);
tu_specific Str8 str8_from_list(Arena* arena, Str8_list* list); // todo: There is not point in the list beeing passed by a pointer here. It shoud either be by value. Or a const pointer
tu_specific Str8 str8_from_list_ex(Arena* arena, const Str8_list list, Str8 str_to_put_before, Str8 str_to_put_between, Str8 str_to_put_after); 
tu_specific Str8 str8_fmt(Arena* arena, const char* fmt, ...);
tu_specific Str8 str8_valist(Arena* arena, const char* fmt, va_list valist);
tu_specific U64 str8_fmt_count(const char* fmt, ...);
tu_specific U64 __str8_fmt_count_valist(const char* fmt, va_list valist);
tu_specific char* cstr_from_str8(Arena* arena, Str8 str);


#endif 