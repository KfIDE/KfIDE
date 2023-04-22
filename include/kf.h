#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include <time.h>

#ifdef KF_PLATFORM_APPLE
	#define GL_SILENCE_DEPRECATION
	#include <OpenGL/gl.h>
#else
	#include <GL/gl.h>
#endif

#include "stb_truetype.h"

/* #define printf(message, __VA_ARGS__...) printf("%s:%i: " message "\n", __FILE__, __LINE__, ## __VA_ARGS__) */

#ifdef KF_DEBUG
#define kfd_printf(message, __VA_ARGS__...) printf("%s:%i: " message "\n", __FILE__, __LINE__, ## __VA_ARGS__)
#else
/* Do nothing, but force a ';' to exist after usage */
#define kfd_printf(message, __VA_ARGS__...) do {} while(0)
#endif

#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
	#ifndef KF_CPU_X86
	#define KF_CPU_X86 1
	#endif
	#ifndef KF_CACHE_LINE_SIZE
	#define KF_CACHE_LINE_SIZE 64
	#endif

#elif defined(_M_PPC) || defined(__powerpc__) || defined(__powerpc64__)
	#ifndef KF_CPU_PPC
	#define KF_CPU_PPC 1
	#endif
	#ifndef KF_CACHE_LINE_SIZE
	#define KF_CACHE_LINE_SIZE 128
	#endif

#elif defined(__arm__)
	#ifndef KF_CPU_ARM
	#define KF_CPU_ARM 1
	#endif
	#ifndef KF_CACHE_LINE_SIZE
	#define KF_CACHE_LINE_SIZE 64
	#endif

#elif defined(__MIPSEL__) || defined(__mips_isa_rev)
	#ifndef KF_CPU_MIPS
	#define KF_CPU_MIPS 1
	#endif
	#ifndef KF_CACHE_LINE_SIZE
	#define KF_CACHE_LINE_SIZE 64
	#endif

#else
	#error Unknown CPU Type
#endif

typedef float f32;
typedef double f64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define KF_BIT(x) (1<<(x))

#define KF_KILO(x) (       (x) * (i64)(1024))
#define KF_MEGA(x) (KF_KILO(x) * (i64)(1024))
#define KF_GIGA(x) (KF_MEGA(x) * (i64)(1024))
#define KF_TERA(x) (KF_GIGA(x) * (i64)(1024))

typedef size_t usize;
typedef ptrdiff_t isize;

typedef usize bool;
#define false ((0 != 0))
#define true ((0 == 0))

typedef i32 Rune; /* Unicode codepoint */
#define KF_RUNE_INVALID (Rune)(0xfffd)
#define KF_RUNE_MAX     (Rune)(0x0010ffff)
#define KF_RUNE_BOM     (Rune)(0xfeff)
#define KF_RUNE_EOF     (Rune)(-1)

typedef struct kf_Utf8AcceptRange {
	u8 lo, hi;
} kf_Utf8AcceptRange;



/* MEMORY */
#define KF_ASSERT(cond) if (!(cond)) do { kfd_printf("Assert failed: %s", #cond); exit(1); } while(0)
#define KF_ASSERT_NOT_NULL(v) if (!(v)) do { printf("Assert not-null failed"); exit(1); } while(0)
#define KF_PANIC(msg, __VA_ARGS__...) do { kfd_printf(msg, ## __VA_ARGS__); exit(1); } while(0)

/* void *kf_memcopy(void *dst, void *src, isize n); */

#define KF_DEFAULT_MEMORY_ALIGNMENT 16
isize kf_align_ceil(isize n);

/* NOTE(ps4sar): this enum is completely unused outside of debug mode; see below */
typedef enum {
	KF_ALLOC_HEAP,
	KF_ALLOC_TEMP,
	KF_ALLOC_PERMA,
} kf_AllocatorType;

typedef enum {
	KF_ALLOC_TYPE_ALLOC,
	KF_ALLOC_TYPE_RESIZE,
	KF_ALLOC_TYPE_FREE,
	KF_ALLOC_TYPE_FREE_ALL,
	KF_ALLOC_TYPE_QUERY_AVAIL_SPACE,
} kf_AllocationType;

typedef struct {
	void *allocate;
	void *user;

#ifdef KF_DEBUG
	kf_AllocatorType type;
#endif

} kf_Allocator;
typedef void *(*kf_AllocatorProc)(kf_Allocator alloc, kf_AllocationType type, void *ptr, isize num_bytes, isize old_size);

void *kf_alloc(kf_Allocator alloc, isize num_bytes);
void kf_free(kf_Allocator alloc, void *ptr);
void kf_free_all(kf_Allocator alloc);

/* Debug */
void kfd_require_allocator_type(kf_Allocator alloc, kf_AllocatorType type_required);
void kfd_require_allocator_space(kf_Allocator alloc, isize at_least);






/* ARRAY */
#define KF_DEFAULT_GROW 32 /* measured in number of elements, not bytes */
#define KF_ARRAY(t) kf_Array
typedef struct {
	u8 *ptr;
	kf_Allocator backing;
	isize type_width;
	isize length;
	isize capacity;
	isize grow;
} kf_Array;

kf_Array kf_array_make(kf_Allocator alloc, isize type_width, isize initial_length, isize initial_capacity, isize grow);

kf_Array kf_array_set_frozen_from_ptr(void *ptr, isize type_width, isize num_elements);
kf_Array kf_array_set_dynamic_from_ptr(kf_Allocator alloc, void *ptr, isize type_width, isize num_elements, isize initial_capacity, isize grow);

void *kf_array_get(kf_Array array, isize index);
void *kf_array_get_type(kf_Array array, isize index, isize type_width);

void kf_array_set(kf_Array *array, isize index, u8 *data);

void kf_array_append(kf_Array *array, void *new_element);
void kf_array_append_multi(kf_Array *array, void *new_elements, isize num);

void kf_freeze(kf_Array *array);
void kf_unfreeze(kf_Array *array, kf_Allocator alloc, /*@SpecialVal(-1)*/isize capacity, /*@SpecialVal(-1)*/isize grow);
bool kf_is_frozen(kf_Array array);

void kf_array_pop(kf_Array *array);
void kf_array_clear(kf_Array *array);

void kf_array_free(kf_Array array);

/* Debug */
void kfd_array_print(kf_Array array);



/* STRINGS */
/* NOTE(ps4star): (this goes for arrays too) pass kf_String* if you're going to WRITE to the str, do kf_String for readonly

This combines frozen and un-frozen (static and dynamic) strings into 1 concept.

A string is "frozen" if (<string>.backing.allocate == NULL).
It is "unfrozen" if the opposite is true.
You can unfreeze a string by calling kf_string_unfreeze() and providing an allocator with which to append new chars.
You can freeze a string by calling kf_string_freeze()
*/
#define KF_DEFAULT_STRING_GROW 128
typedef kf_Array kf_String;

kf_String kf_string_set_from_cstring(u8 *cstr);
kf_String kf_string_set_from_cstring_len(u8 *cstr, isize length);

kf_String kf_string_copy_from_cstring(kf_Allocator alloc, u8 *cstr, isize grow);
kf_String kf_string_copy_from_cstring_len(kf_Allocator alloc, u8 *cstr, isize len, isize cap, isize grow);

kf_String kf_string_copy_from_string(kf_Allocator alloc, kf_String to_clone);
kf_String kf_string_copy_from_string_len(kf_Allocator alloc, kf_String to_clone, isize substr_length, isize capacity, isize grow);

kf_String kf_string_make(kf_Allocator alloc, isize length, isize capacity, isize grow);
kf_String kf_string_make_zeroed(kf_Allocator alloc, isize length, isize capacity, isize grow);

isize kf_string_next_capacity(kf_String str);
void kf_string_resize(kf_String *str, isize new_capacity);

void kf_string_set_char(kf_String *str, isize at, u8 new_char);
void kf_string_write_cstring_at(kf_String *str, u8 *cstr, isize at);
void kf_string_write_cstring_at_len(kf_String *str, u8 *cstr, isize at, isize length);
void kf_string_write_string_at(kf_String *str, kf_String other, isize at);
void kf_string_write_string_at_len(kf_String *str, kf_String other, isize at, isize length);

void kf_string_append_cstring(kf_String *str, u8 *cstr);
void kf_string_append_cstring_len(kf_String *str, u8 *cstr, isize length);
void kf_string_append_string(kf_String *str, kf_String other);
void kf_string_append_string_len(kf_String *str, kf_String other, isize slice_other_upto);
/* void kf_string_append_rune(kf_String *str, Rune r, bool null_terminate); */ /* NOTE(ps4star): moved to string util section */

u8 kf_string_pop(kf_String *str);
void kf_string_clear(kf_String *str);

void kf_string_free(kf_String str);













/* HEAP ALLOCATOR */
void *kf_heap_allocator_proc(kf_Allocator alloc, kf_AllocationType type, void *ptr, isize num_bytes, isize old_size);
kf_Allocator kf_heap_allocator(void);

/* TEMP (fixed linear buffer) ALLOCATOR */
typedef struct {
	u8 *ptr;
	isize position;
	isize size;
	isize num_allocations;
} kf_TempAllocatorData;

void kf_init_temp_allocator_data(kf_TempAllocatorData *data, kf_Allocator backing, isize capacity);
kf_Allocator kf_temp_allocator(kf_TempAllocatorData *data);

void *kf_temp_allocator_proc(kf_Allocator alloc, kf_AllocationType type, void *ptr, isize num_bytes, isize old_size);

/* PERMA ALLOCATOR */
#define KF_PERMA_ALLOCATOR_RECORD_DEFAULT_SIZE 256
#define KF_PERMA_ALLOCATOR_DEFAULT_BLOCKS_TO_ADD_ON_RESIZE 8
typedef struct { /* the only real diff. between this and the temp one is this one can do frees and also is expandable */
	u8 					*blocks;
	kf_Allocator		backing;
	isize				block_size;
	isize				num_blocks;
	isize				blocks_to_add_on_resize;
	KF_ARRAY(bool)		block_record; /* indices of free blocks followed by how many */
} kf_PermaAllocatorData;

void kf_init_perma_allocator_data(kf_PermaAllocatorData *data, kf_Allocator backing, isize initial_num_blocks, isize block_size, isize blocks_to_add_on_resize);
kf_Allocator kf_perma_allocator(kf_PermaAllocatorData *data);

void *kf_perma_allocator_proc(kf_Allocator alloc, kf_AllocationType type, void *ptr, isize num_bytes, isize old_size);









/* IO (platform) */
extern const u8 kf_path_separator;
typedef enum {
	KF_FILE_ERROR_NONE,
	KF_FILE_ERROR_INVALID,
	KF_FILE_ERROR_INVALID_FILE_NAME,
	KF_FILE_ERROR_EXISTS,
	KF_FILE_ERROR_DOES_NOT_EXIST,
	KF_FILE_ERROR_PERMISSION,
	KF_FILE_ERROR_TRUNCATION_FAILURE,
	KF_FILE_ERROR_UNSPECIFIED,
} kf_FileError;

typedef enum {
	KF_FILE_MODE_READ = KF_BIT(0),
	KF_FILE_MODE_WRITE = KF_BIT(1),
	KF_FILE_MODE_READ_WRITE = KF_BIT(2),
	KF_FILE_MODE_APPEND = KF_BIT(3),
	KF_FILE_MODE_APPEND_READ = KF_BIT(4),
	KF_FILE_MODE_CREATE = KF_BIT(5),
} kf_FileMode;

typedef struct {
	kf_String		full_path;
	FILE			*libc_file; /* Should be NULL if invalid */
	kf_FileMode		operating_mode;
} kf_File;

/* kf_FileError kf_file_open(kf_File *file, kf_String path); */
kf_FileError kf_file_open(kf_File *file, kf_String path, kf_FileMode mode);
kf_String kf_file_read(kf_File file, kf_Allocator str_alloc, isize bytes_to_read, isize at);
void kf_file_read_into_cstring(kf_File file, u8 *buf, isize bytes_to_read, isize at);
u64 kf_file_size(kf_File file);
bool kf_path_exists(kf_String path);
void kf_file_close(kf_File file);

/* Higher-level IO API (std_io.c) */
typedef struct {
	kf_Allocator	backing;
	void      		*data;
	isize       	size;
} kf_FileContents;

kf_FileContents kf_file_read_contents(kf_Allocator a, bool null_terminate, kf_String path);
void kf_file_free_contents(kf_FileContents *contents);


typedef struct {
	kf_String		path;
	bool			is_file;
	bool			is_dir;
	bool			is_link;
} kf_FileInfo;

void kf_read_dir(kf_String path, KF_ARRAY(kf_FileInfo) *entries, kf_Allocator str_alloc);

typedef int (*kf_WalkTreeProc)(kf_Allocator temp_alloc, kf_String this_path, void *user);
typedef enum {
	KF_WALK_TREE_EXCLUDE_DIRS = KF_BIT(0),
	KF_WALK_TREE_EXCLUDE_LINKS = KF_BIT(1),

	KF_WALK_TREE_FILES_ONLY = KF_WALK_TREE_EXCLUDE_DIRS | KF_WALK_TREE_EXCLUDE_LINKS,
} kf_WalkTreeFlags;

void kf_walk_tree(kf_String root_dir, kf_WalkTreeProc callback, kf_Allocator temp_alloc, void *user, kf_WalkTreeFlags flags);









/* MATH */
typedef struct {
	isize x, y;
} kf_IVector2;

typedef struct {
	isize x, y, w, h;
} kf_IRect;

typedef struct {
	f32 x, y, w, h;
} kf_FRect;

typedef struct {
	f32 x, y, x2, y2;
} kf_UVRect;

#define KF_IRECT(x, y, w, h) (kf_IRect){ (x), (y), (w), (h) }
#define KF_UVRECT(x, y, x2, y2) (kf_UVRect){ (x), (y), (x2), (y2) }

/* Applies a margin rect to a rect.
For example: r={ 10, 10, 20, 20 } and margin={ 1, 2, 5, 5 }
would return { 11, 12, 15, 15 }; it essentially "pushes in" each of the 4 rect fields by the specified margin
obviously if there is a 0 at any part of the margin then it does nothing to the respective field
a margin of {0, 0, 0, 0} will just return the same rect you put into it */
kf_IRect kf_apply_rect_margin(kf_IRect r, kf_IRect margin);
/* Moves the x/w of a rect over by the specified amounts and returns a copy of the translated rect */
kf_IRect kf_apply_rect_translation(kf_IRect r, isize x, isize y);
/* Returns a UV FRect from a pixels IRect. viewport determines the rect to scale to */
kf_UVRect kf_screen_to_uv(kf_IRect viewport, kf_IRect r);

typedef struct { u8 r, g, b, a; } kf_Color;
#define KF_RGBA(r, g, b, a) (kf_Color){ (r), (g), (b), (a) }
#define KF_RGB(r, g, b) KF_RGBA((r), (g), (b), 255)

#define KF_IS_BETWEEN(n, b1, b2) (bool)((n) >= (b1) && (n) <= (b2))









/* STRING UTIL */

/* Rune stuff */
/* Decode a utf8-formatted string and write into the given KF_ARRAY(Rune) */
void kf_decode_utf8_string_to_rune_array(kf_String str, KF_ARRAY(Rune) *rune_array);
void kf_append_rune_to_string(kf_String *str, Rune r);

/* Joins 2 paths and adds in '/' in the middle
NOTE: This means you cannot put a string that ends with '/' */
kf_String kf_join_paths(kf_Allocator join_alloc, kf_String base, kf_String add);
/* Returns true if the string ends with the given sub-string, false otherwise */
bool kf_string_ends_with(kf_String s, kf_String cmp);
bool kf_string_ends_with_cstring(kf_String s, u8 *cmp, isize cmp_strlen);








/* TIME */
typedef struct {
	clock_t start_t, end_t;
} kf_Time;

void kf_write_current_time(kf_Time *t);
void kf_print_time_since(kf_Time *t);







/* PLATFORM */
/* Platform context (opaque ptr) */
typedef void *kf_PlatformSpecificContext;

/* Returns platform specific variables as a struct. */
kf_PlatformSpecificContext kf_get_platform_specific_context(void);






/* VIDEO (platform) */

/* Initializes a window and an OpenGL instance.
If w || h are -1, uses max size */
typedef enum {
	KF_VIDEO_MAXIMIZED = KF_BIT(0),
	KF_VIDEO_HIDDEN_WINDOW = KF_BIT(1),
} kf_VideoFlags;
void kf_init_video(kf_PlatformSpecificContext ctx, kf_String title, isize x, isize y, isize w, isize h, kf_VideoFlags flags);
/* Set vsync on/off (1 for on, 0 for off, -1 for adaptive (may not work if the -EXT version can't be loaded)) */
void kf_set_vsync(kf_PlatformSpecificContext ctx, isize vsync);
/* Resizes the window. */
/*void kf_resize_window(kf_PlatformSpecificContext ctx, isize w, isize h);*/
/* Writes window size to isize ptrs */
void kf_write_window_size(kf_PlatformSpecificContext ctx, isize *w, isize *h);
/* Flushes everything drawn with OpenGL to the buffer. */
void kf_swap_buffers(kf_PlatformSpecificContext ctx);
/* Frees the window and OpenGL instance. */
void kf_terminate_video(kf_PlatformSpecificContext ctx);


/* EVENTS (platform) */
typedef struct {
	bool exited;

	bool was_window_resized;
	isize window_resize_width, window_resize_height;
	isize mouse_x, mouse_y, mouse_xrel, mouse_yrel;
} kf_EventState;

/* Grabs events and sets kf_EventState accordingly
If await is true, then this halts the program until there is actually an event
to grab */
void kf_analyze_events(kf_PlatformSpecificContext ctx, kf_EventState *out, bool await);


/* FONT LOADING (platform) */
#define KF_NUM_SYSTEM_FONT_PATHS 16
extern const u8 *kf_system_font_paths[KF_NUM_SYSTEM_FONT_PATHS];

/* Writes list of available system font paths to the array */
void kf_query_system_fonts(kf_Allocator temp_alloc, KF_ARRAY(kf_String) *out);
/* DEBUG: print out the KF_ARRAY(kf_String) returned by kf_query_system_fonts() */
void kfd_print_system_fonts(KF_ARRAY(kf_String) query_result);






/* TRANSLATION
HOW THIS WORKS

after header parse, .lang_head will hold the number of langs
so what we do is have that be the stride in the keys/values index

like if you want to get <nl>[1] where 4 langs are defined and nl has ID 2
you're looking for values[(4 * 1) + 2]
*/
#define KF_MAX_TRANSLATION_LANGS 256
#define KF_MAX_TRANSLATION_ENTRIES 2048
typedef struct {
	u8 *lang_keys[KF_MAX_TRANSLATION_LANGS];

	u8 *selected_lang;
	isize selected_lang_offset; /* this and selected_lang are cached together. selected_lang can probably be removed actually, we only use this part */
	
	u8 *values[KF_MAX_TRANSLATION_LANGS * KF_MAX_TRANSLATION_ENTRIES];
	isize lang_head;
	isize num_entries; /* basically just number of lines of text in the file, excluding header line. NOT total entries of each lang combined */
} kf_TranslationRecord;


void kf_load_translations_from_csv_buffer(kf_Allocator alloc, kf_TranslationRecord *record, u8 *data, isize length);
void kfd_print_translations(kf_TranslationRecord record);






/* RENDERING */
/* Text stuff */

/*
There are a few different indices going on here that need explanation.
So for one you have TTF glyph indices. This is used to call stbtt_*.
You have to get this by finding the internal index and then getting
the TTF index out of the kf_GlyphInfo list.

You use the runes KF_ARRAY to convert a Rune
to an index by finding the index of where that rune is, and then using it
to index the kf_GlyphInfo array to fetch glyph data like TTF index.
*/
typedef struct {
	isize 						width, height; /* glyph size */
	GLuint						gl_tex; /* gl text */
	int							ax, lsb;
	isize						x1, y1; /* x and y offsets */
	int							index; /* ttf glyph index */ 
} kf_GlyphInfo;

typedef struct {
	stbtt_fontinfo				stb_font;
	KF_ARRAY(Rune)				runes; /* map Rune -> GlyphInfo index */
	KF_ARRAY(kf_GlyphInfo)		glyphs; /* glyph-specific metrics/data */

	isize						ascent, descent, line_gap; /* global font metrics */
	f32							scale;
} kf_Font;

/* Returns internal (within kf_Font) index of the rune */
isize kf_lookup_internal_glyph_index_by_rune(kf_Font *font, Rune r);





/* UI */
typedef enum { /* which UIDraw* we're dealing with */
	KF_DRAW_RECT,
	KF_DRAW_TEXT,
} kf_UIDrawType;

typedef struct { /* use ".common" on the UIDrawCommand union to access this without type-checking */
	kf_UIDrawType		type;
	kf_Color			color;
} kf_UIDrawCommon;

typedef struct { /* rectangle draw (in screen coords) */
	kf_UIDrawCommon		common;
	kf_IRect 			rect;
} kf_UIDrawRect;

typedef struct { /* text draw (screen coords) */
	kf_UIDrawCommon		common;
	kf_String			text;
	kf_Font				*font;
	kf_IVector2			begin;
} kf_UIDrawText;

typedef union { /* what the user recv's for rendering */
	kf_UIDrawCommon		common;
	kf_UIDrawRect		rect;
	kf_UIDrawText		text;
} kf_UIDrawCommand;

typedef struct { /* running UI context (should be in GlobalVars on user side) */
	/* Input state reference (set in kf_ui_begin()) */
	kf_EventState						*ref_state;

	/* Main UI processing */
	kf_Allocator							begin_allocator;
	KF_ARRAY(kf_UIDrawCommand)			draw_commands;
	KF_ARRAY(kf_IRect)					origin_stack;

	/* Style */
	kf_Color							color;
	kf_IRect							margin;
	kf_Font								*font;
} kf_UIContext;

/* Begin the UI session; allocate draw_commands etc */
void kf_ui_begin(kf_UIContext *ctx, kf_Allocator alloc, isize expected_num_draw_commands, kf_EventState *ref_state);
/* End the UI session; frees memory allocated at _begin() if free_memory is true */
void kf_ui_end(kf_UIContext *ctx, bool free_memory);

/* Viewport (->rect_stack) control */
void kf_ui_push_origin(kf_UIContext *ctx, isize x, isize y);
void kf_ui_push_origin_relative(kf_UIContext *ctx, isize x, isize y); /* same as push_origin but not absolute (relative to last origin) */
void kf_ui_pop_origin(kf_UIContext *ctx);

/* Set color to use for future UI element pushes */
void kf_ui_color(kf_UIContext *ctx, kf_Color color);
/* Set margin to use for future UI element pushes */
static const kf_IRect KF_NO_MARGIN = (kf_IRect){ 0, 0, 0, 0 };
void kf_ui_margin(kf_UIContext *ctx, kf_IRect margin);
/* Set font to use for future UI text elements */
void kf_ui_font(kf_UIContext *ctx, kf_Font *font);

/* Push rect */
void kf_ui_rect(kf_UIContext *ctx, kf_IRect rect);
/* Push text */
void kf_ui_text(kf_UIContext *ctx, kf_String text, isize x, isize y);