#pragma once
#include "gb.h"

#include <stdlib.h>
#include <stdio.h>

#ifdef __APPLE__
	#define GL_SILENCE_DEPRECATION
	#include <OpenGL/gl.h>
#else
	#include <GL/gl.h>
	#include <EGL/egl.h>
#endif

#define printf(message, __VA_ARGS__...) printf("%s:%i: " message "\n", __FILE__, __LINE__, ## __VA_ARGS__)


/* MATH */
typedef struct {
	isize x, y, w, h;
} IRect;

/* Color type. */
typedef u32 Color;
typedef struct {
	u8 r, g, b, a;
} ColorStruct;
/* Convert RGB to Hex. */
#define RGB(r, g, b) (u32)((u32)(0) | (u32)(r << 24) | (u32)(g << 16) | (u32)(b << 8) | (u32)(255))




/* PLATFORM */
/* Platform context (opaque ptr) */
typedef void *PlatformSpecificContext;

/* Returns platform specific variables as a struct. */
PlatformSpecificContext kf_get_platform_specific_context(void);
/* Checks and returns a boolean if a file exists. */
bool kf_file_exists(gbString path);
/* Finds provided Truetype font. The function returns the path of where the font is in the system. */
gbString find_font(gbString font);


/* WINDOW */
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

/* TEXT RENDERING */
/* Initializes a font and returns its Opengl texture ID. */
GLuint init_font(gbString font, isize size);
/* Draws text on the screen. */
void draw_text(GLuint font_id, gbString text, isize x, isize y, Color color);


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


typedef struct {
	gbAllocator heap_alloc, global_alloc, temp_alloc;
	gbArena temp_backing, global_backing;
	PlatformSpecificContext platform_context;

	IRect win_bounds;
	EventState event_state;
	TranslationRecord translation_record;
} GlobalVars;

GlobalVars g;