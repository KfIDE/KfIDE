#pragma once
#include "gb.h"

#include <stdlib.h>

#ifdef __APPLE__
	#define GL_SILENCE_DEPRECATION
	#include <OpenGL/gl.h>
#else
	#include <GL/gl.h>
	#include <EGL/egl.h>
#endif


/* MATH */
typedef struct {
	isize x, y, w, h;
} IRect;


/* PLATFORM */
/* Platform context (opaque ptr) */
typedef void *PlatformSpecificContext;

/* Returns platform specific variables as a struct. */
PlatformSpecificContext kf_get_platform_specific_context(void);


/* Initializes a window and an OpenGL instance. */
void kf_init_video(PlatformSpecificContext ctx, gbString title, isize x, isize y, isize w, isize h, bool maximized);
/* Set vsync on/off (1 for on, 0 for off, -1 for adaptive (may not work if the -EXT version can't be loaded)) */
void kf_set_vsync(PlatformSpecificContext ctx, int vsync);
/* Resizes the window. */
void kf_resize_window(PlatformSpecificContext ctx, isize w, isize h);
/* Flushes everything drawn with OpenGL to the buffer. */
void kf_swap_buffers(PlatformSpecificContext ctx);
/* Frees the window and OpenGL instance. */
void kf_terminate_video(PlatformSpecificContext ctx);

typedef struct {
	isize mouse_x, mouse_y, mouse_xrel, mouse_yrel;
	bool exited;
} EventState;

/* Grabs events (non-blocking/non-vsync) and sets EventState accordingly */
void kf_analyze_events(PlatformSpecificContext ctx, EventState *out);


/* TRANSLATION
HOW THIS WORKS

after header parse, .lang_head will hold the number of langs
so what we do is have that be the stride in the keys/values index

like if you want to get <nl>[1] where 4 langs are defined and nl has ID 2
you're looking for values[(4 * 1) + 2]
*/
#define MAX_TRANSLATION_LANGS 256
#define MAX_TRANSLATION_ENTRIES 2048
typedef struct {
	u8 *lang_keys[MAX_TRANSLATION_LANGS];

	u8 *selected_lang;
	isize selected_lang_offset; /* this and selected_lang are cached together. selected_lang can probably be removed actually, we only use this part */
	
	u8 *values[MAX_TRANSLATION_LANGS * MAX_TRANSLATION_ENTRIES];
	isize lang_head;
	isize num_entries; /* basically just number of lines of text in the file, excluding header line. NOT total entries of each lang combined */
} TranslationRecord;


void kf_load_translations_from_csv_string(gbAllocator alloc, TranslationRecord *record, u8 *data, isize length);
