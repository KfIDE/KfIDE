#include "kf.h"

#include <X11/Xlib.h>

typedef struct {
	Display *display;
	Window window;
	XEvent event;
	int screen;
	Atom del_atom;
	long mask;
} XInfo;
static XInfo xinfo;

PlatformSpecificContext kf_get_platform_specific_context(void)
{
	return (PlatformSpecificContext)&xinfo;
}

void kf_init_video(PlatformSpecificContext ctx, gbString title, isize x, isize y, isize w, isize h)
{
	XInfo *xinfo;

	xinfo = ctx;
	xinfo->display = XOpenDisplay(NULL);
	if (xinfo->display == NULL) {
		GB_PANIC("Could not open Xorg display");
	}

	xinfo->screen = DefaultScreen(xinfo->display);
	xinfo->window = XCreateSimpleWindow(xinfo->display, RootWindow(xinfo->display, xinfo->screen), x, y, w, h, 1, BlackPixel(xinfo->display, xinfo->screen), WhitePixel(xinfo->display, xinfo->screen));
	xinfo->del_atom = XInternAtom(xinfo->display, "WM_DELETE_WINDOW", 0);
	XSetWMProtocols(xinfo->display, xinfo->window, &xinfo->del_atom, 1);

	xinfo->mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
	XSelectInput(xinfo->display, xinfo->window, xinfo->mask);
	XStoreName(xinfo->display, xinfo->window, title);
	XMapWindow(xinfo->display, xinfo->window);
}

void kf_analyze_events(PlatformSpecificContext ctx, EventState *out)
{
	XInfo *xinfo;

	xinfo = ctx;
	while (XPending(xinfo->display)) {
		XEvent evt;
		XNextEvent(xinfo->display, &evt);
	}
}