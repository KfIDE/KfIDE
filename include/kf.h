#pragma once
#include "gb.h"

#include <stdlib.h>
#include <GL/gl.h>


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


/* TRANSLATION */
#define MAX_TRANSLATIONS 256
typedef struct {
	isize lang_head;
	u8 *lang_keys[MAX_TRANSLATIONS];
} TranslationRecord;