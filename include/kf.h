#pragma once
#include "gb.h"

#include <stdlib.h>


/* Platform */
typedef void *PlatformSpecificContext;


typedef struct {
	isize x, y, w, h;
} IRect;

typedef struct {
	isize mouse_x, mouse_y, mouse_xrel, mouse_yrel;
} EventState;


/* Returns platform specific variables as a struct. */
PlatformSpecificContext kf_get_platform_specific_context(void);
/* Initializes a window and an OpenGL instance. */
void kf_init_video(PlatformSpecificContext ctx, gbString title, isize x, isize y, isize w, isize h);

/* Grabs events (non-blocking/non-vsync) and sets EventState accordingly */
void kf_analyze_events(PlatformSpecificContext ctx, EventState *out);