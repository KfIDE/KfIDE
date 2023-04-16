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


typedef struct {
	isize x, y, w, h;
} IRect;

typedef struct {
	isize mouse_x, mouse_y, mouse_xrel, mouse_yrel;
	bool exited;
} EventState;


/* Platform specific variable. */
typedef void *PlatformSpecificContext;


/* Returns platform specific variables as a struct. */
PlatformSpecificContext kf_get_platform_specific_context(void);

/* Initializes a window and an OpenGL instance. */
void kf_init_video(PlatformSpecificContext ctx, gbString title, isize x, isize y, isize w, isize h, bool maximized);
/* Frees the window and OpenGL instance. */
void kf_terminate_video(PlatformSpecificContext ctx);
/* Resizes the window. */
void kf_resize_window(PlatformSpecificContext ctx, isize w, isize h);

/* Grabs events (non-blocking/non-vsync) and sets EventState accordingly */
void kf_analyze_events(PlatformSpecificContext ctx, EventState *out);

/* Flushes everything drawn with OpenGL to the buffer. */
void kf_flush_buffer(PlatformSpecificContext ctx);