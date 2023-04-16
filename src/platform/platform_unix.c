#include "kf.h"

#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>


typedef struct {
	Display *display;
	Window window;
	XEvent event;
	int screen;
	Atom del_atom;
	long mask;

	GLXContext glx_context;
	GLXWindow glx_window;
	GLXFBConfig *fb_configs;
	isize swap_flag;
} XInfo;


static XInfo xinfo;

/* Prefer double_buf_attributes but fallback to single if it doesn't work
Also set up widths to R8G8B8A8 */
static GLint double_buf_attributes[] = { GLX_RGBA, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 8, GLX_DOUBLEBUFFER, None };
static GLint single_buf_attributes[] = { GLX_RGBA, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 8, None };


PlatformSpecificContext kf_get_platform_specific_context(void)
{
	return (PlatformSpecificContext)&xinfo;
}

static void (*glXSwapIntervalEXT)(Display *display, GLXDrawable drawable, int vsync);
void kf_init_video(PlatformSpecificContext ctx, gbString title, isize x, isize y, isize w, isize h, bool maximized)
{
	XInfo *xinfo;
	isize num_returned = 0;

	xinfo = ctx;
	xinfo->display = XOpenDisplay(NULL);
	if (xinfo->display == NULL) {
		GB_PANIC("Could not open Xorg display");
	}

	xinfo->screen = DefaultScreen(xinfo->display);
	xinfo->window = XCreateSimpleWindow(xinfo->display, RootWindow(xinfo->display, xinfo->screen), x, y, w, h, 1, WhitePixel(xinfo->display, xinfo->screen), BlackPixel(xinfo->display, xinfo->screen));
	xinfo->del_atom = XInternAtom(xinfo->display, "WM_DELETE_WINDOW", 0);
	XSetWMProtocols(xinfo->display, xinfo->window, &xinfo->del_atom, 1);

	xinfo->mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
	XSelectInput(xinfo->display, xinfo->window, xinfo->mask);
	XStoreName(xinfo->display, xinfo->window, title);
	XMapWindow(xinfo->display, xinfo->window);

	xinfo->swap_flag = True;
	xinfo->fb_configs = glXChooseFBConfig(xinfo->display, DefaultScreen(xinfo->display), double_buf_attributes, &num_returned);
	GB_ASSERT(num_returned > 0);
	if (xinfo->fb_configs == NULL) /* resort to single-buffer if DB not available */
	{
		xinfo->fb_configs = glXChooseFBConfig(xinfo->display, DefaultScreen(xinfo->display), single_buf_attributes, &num_returned);
		GB_ASSERT(num_returned > 0);
		xinfo->swap_flag = False;
	}

	xinfo->glx_context = glXCreateNewContext(xinfo->display, xinfo->fb_configs[0], GLX_RGBA_TYPE, NULL, True);
	xinfo->glx_window = glXCreateWindow(xinfo->display, xinfo->fb_configs[0], xinfo->window, NULL);
	glXMakeContextCurrent(xinfo->display, xinfo->glx_window, xinfo->glx_window, xinfo->glx_context);

	/* Load extensions */
	glXSwapIntervalEXT = glXGetProcAddress("glXSwapIntervalEXT");
}

void kf_set_vsync(PlatformSpecificContext ctx, int vsync)
{
	XInfo *xinfo;

	xinfo = ctx;
	glXSwapIntervalEXT(xinfo->display, xinfo->glx_window, vsync);
}

void kf_resize_window(PlatformSpecificContext ctx, isize w, isize h)
{
	XInfo *xinfo;

	xinfo = ctx;
	XResizeWindow(xinfo->display, xinfo->window, (unsigned int)w, (unsigned int)h);
}

void kf_swap_buffers(PlatformSpecificContext ctx)
{
	XInfo *xinfo;

	xinfo = ctx;
	glXSwapBuffers(xinfo->display, xinfo->glx_window);
}

void kf_terminate_video(PlatformSpecificContext ctx)
{
	XInfo *xinfo;

	xinfo = ctx;
	glXDestroyContext(xinfo->display, xinfo->glx_context);
	XCloseDisplay(xinfo->display);
}



void kf_analyze_events(PlatformSpecificContext ctx, EventState *out)
{
	XInfo *xinfo;

	xinfo = ctx;
	while (XPending(xinfo->display)) {
		XEvent evt;
		XNextEvent(xinfo->display, &evt);

		if (evt.type == ClientMessage)
		{
			if ((Atom)evt.xclient.data.l[0] == xinfo->del_atom)
			{
				out->exited = true;
			}
		}
	}
}