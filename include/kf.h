#pragma once
#include "gb.h"

#include <stdlib.h>

typedef struct {
	isize x, y, w, h;
} IRect;

/* Platform */
typedef void *PlatformSpecificContext;

PlatformSpecificContext kf_get_platform_specific_context(void);
void kf_init_video(PlatformSpecificContext ctx, gbString title, isize x, isize y, isize w, isize h);

typedef struct {
	isize mouse_x, mouse_y, mouse_xrel, mouse_yrel;
} EventState;

/* Grabs events (non-blocking/non-vsync) and sets EventState accordingly */
void kf_analyze_events(PlatformSpecificContext ctx, EventState *out);