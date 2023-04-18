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

#define printf(message, __VA_ARGS__...) printf("%s:%i: " message "\n", __FILE__, __LINE__, ## __VA_ARGS__)


/* MATH */
typedef struct {
	isize x, y;
} IVector2;

typedef struct {
	isize x, y, w, h;
} IRect;

typedef struct {
	f32 x, y, w, h;
} FRect;

IRect kf_apply_rect_margin(IRect r, IRect margin);
IRect kf_apply_rect_translation(IRect r, isize x, isize y);

typedef u32 Color;
typedef struct { u8 r, g, b, a; } ColorStruct;
#define KF_RGBA(r, g, b, a) (Color)( ((u32)(r) << 24) | ((u32)(g) << 16) | ((u32)(b) << 8) | (u32)(a) )
#define KF_RGB(r, g, b) KF_RGBA(r, g, b, 255)
static ColorStruct *kf_color_to_struct(Color *color) { return (ColorStruct *)color; }


/* PLATFORM */
/* Platform context (opaque ptr) */
typedef void *PlatformSpecificContext;

/* Returns platform specific variables as a struct. */
PlatformSpecificContext kf_get_platform_specific_context(void);
/* Checks and returns a boolean if a file exists. */
bool kf_file_exists(gbString path);
/* Finds provided Truetype font. The function returns the path of where the font is in the system. */
gbString find_font(gbString font);


/* VIDEO (platform) */

/* Initializes a window and an OpenGL instance.
If w || h are -1, uses max size */
typedef enum {
	KF_VIDEO_MAXIMIZED = GB_BIT(0),
} VideoFlags;
void kf_init_video(PlatformSpecificContext ctx, gbString title, isize x, isize y, isize w, isize h, VideoFlags flags);
/* Set vsync on/off (1 for on, 0 for off, -1 for adaptive (may not work if the -EXT version can't be loaded)) */
void kf_set_vsync(PlatformSpecificContext ctx, isize vsync);
/* Resizes the window. */
/*void kf_resize_window(PlatformSpecificContext ctx, isize w, isize h);*/
/* Flushes everything drawn with OpenGL to the buffer. */
void kf_swap_buffers(PlatformSpecificContext ctx);
/* Frees the window and OpenGL instance. */
void kf_terminate_video(PlatformSpecificContext ctx);


/* EVENTS (platform) */

/* TEXT RENDERING */
/* Initializes a font and returns its Opengl texture ID. */
GLuint init_font(gbString font, isize size);
/* Draws text on the screen. */
void draw_text(GLuint font_id, gbString text, isize x, isize y, Color color);


typedef struct {
	isize mouse_x, mouse_y, mouse_xrel, mouse_yrel;
	bool exited;
} EventState;

/* Grabs events and sets EventState accordingly
If await is true, then this halts the program until there is actually an event
to grab */
void kf_analyze_events(PlatformSpecificContext ctx, EventState *out, bool await);




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



/* UI */
typedef enum { /* which UIDraw* we're dealing with */
	KF_DRAW_RECT,
	KF_DRAW_TEXT,
} UIDrawType;

typedef struct { /* use ".common" on the UIDrawCommand union to access this without type-checking */
	UIDrawType type;
	Color color;
} UIDrawCommon;

typedef struct { /* rectangle draw (in screen coords) */
	IRect rect;
} UIDrawRect;

typedef struct { /* text draw (screen coords) */
	gbString text;
	IVector2 begin;
} UIDrawText;

typedef union { /* what the user recv's for rendering */
	UIDrawCommon common;
	UIDrawRect rect;
	UIDrawText text;
} UIDrawCommand;

typedef struct { /* running UI context (should be in GlobalVars on user side) */
	/* Input state reference (set in kf_ui_begin()) */
	EventState *ref_state;

	/* Main UI processing */
	gbAllocator begin_allocator;
	gbArray(UIDrawCommand) draw_commands;
	gbArray(IRect) origin_stack;

	/* Style */
	Color color;
	IRect margin;
} UIContext;

/* Begin the UI session; allocate draw_commands etc */
void kf_ui_begin(UIContext *ctx, gbAllocator alloc, isize expected_num_draw_commands, EventState *ref_state);
/* End the UI session; frees memory allocated at _begin() if free_memory is true */
void kf_ui_end(UIContext *ctx, bool free_memory);

/* Viewport (->rect_stack) control */
void kf_ui_push_origin(UIContext *ctx, isize x, isize y);
void kf_ui_push_origin_relative(UIContext *ctx, isize x, isize y); /* same as push_origin but not absolute (relative to last origin) */
void kf_ui_pop_origin(UIContext *ctx);

/* Set color to use for future UI element pushes */
void kf_ui_color(UIContext *ctx, Color color);
/* Set margin to use for future UI element pushes */
static const IRect KF_NO_MARGIN = (IRect){ 0, 0, 0, 0 };
void kf_ui_margin(UIContext *ctx, IRect margin);

/* Push rect */
void kf_ui_rect(UIContext *ctx, IRect rect);
