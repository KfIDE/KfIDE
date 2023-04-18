#pragma once
#include "gb.h"

#include <stdlib.h>
#include <stdio.h>

#ifdef KF_PLATFORM_APPLE
	#define GL_SILENCE_DEPRECATION
	#include <OpenGL/gl.h>
#else
	#include <GL/gl.h>
#endif

#include "stb_truetype.h"

#define printf(message, __VA_ARGS__...) printf("%s:%i: " message "\n", __FILE__, __LINE__, ## __VA_ARGS__)


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
#define KF_RGBA(r, g, b, a) (kf_Color){ r, g, b, a }
#define KF_RGB(r, g, b) KF_RGBA(r, g, b, 255)




/* TIME */
typedef struct {
	time_t _time;
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
	KF_VIDEO_MAXIMIZED = GB_BIT(0),
} kf_VideoFlags;
void kf_init_video(kf_PlatformSpecificContext ctx, gbString title, isize x, isize y, isize w, isize h, kf_VideoFlags flags);
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
} kf_TranslationRecord;


void kf_load_translations_from_csv_string(gbAllocator alloc, kf_TranslationRecord *record, u8 *data, isize length);






/* RENDERING */
/* Text stuff */

/*
There are a few different indices going on here that need explanation.
So for one you have TTF glyph indices. This is used to call stbtt_*.
You have to get this by finding the internal index and then getting
the TTF index out of the kf_GlyphInfo list.

You use the runes gbArray to convert a Rune
to an index by finding the index of where that rune is, and then using it
to index the kf_GlyphInfo array to fetch glyph data like TTF index.
*/
typedef struct {
	isize 						width, height; /* glyph size */
	GLuint						gl_tex; /* gl text */
	int							index; /* ttf glyph index */ 
} kf_GlyphInfo;

typedef struct {
	stbtt_fontinfo				stb_font;
	gbArray(Rune) 				runes; /* map Rune -> GlyphInfo index */
	gbArray(kf_GlyphInfo)		glyphs; /* glyph-specific metrics/data */

	isize						ascent, descent, line_gap; /* global font metrics */
} kf_Font;





/* UI */
typedef enum { /* which UIDraw* we're dealing with */
	KF_DRAW_RECT,
	KF_DRAW_TEXT,
} kf_UIDrawType;

typedef struct { /* use ".common" on the UIDrawCommand union to access this without type-checking */
	kf_UIDrawType type;
	kf_Color color;
} kf_UIDrawCommon;

typedef struct { /* rectangle draw (in screen coords) */
	kf_UIDrawCommon common;
	kf_IRect rect;
} kf_UIDrawRect;

typedef struct { /* text draw (screen coords) */
	kf_UIDrawCommon common;
	gbString text;
	kf_Font *font;
	kf_IVector2 begin;
} kf_UIDrawText;

typedef union { /* what the user recv's for rendering */
	kf_UIDrawCommon common;
	kf_UIDrawRect rect;
	kf_UIDrawText text;
} kf_UIDrawCommand;

typedef struct { /* running UI context (should be in GlobalVars on user side) */
	/* Input state reference (set in kf_ui_begin()) */
	kf_EventState *ref_state;

	/* Main UI processing */
	gbAllocator begin_allocator;
	gbArray(kf_UIDrawCommand) draw_commands;
	gbArray(kf_IRect) origin_stack;

	/* Style */
	kf_Color color;
	kf_IRect margin;
	kf_Font *font;
} kf_UIContext;

/* Begin the UI session; allocate draw_commands etc */
void kf_ui_begin(kf_UIContext *ctx, gbAllocator alloc, isize expected_num_draw_commands, kf_EventState *ref_state);
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
void kf_ui_text(kf_UIContext *ctx, gbString text, isize x, isize y);