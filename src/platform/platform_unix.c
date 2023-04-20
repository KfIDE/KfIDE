#include "kf.h"

#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>


/* IO stuff */
void kf_read_dir(gbString path, gbArray(kf_FileInfo) entries, gbAllocator str_alloc)
{
	GB_ASSERT(path != NULL && entries != NULL);

	DIR *dir;
	struct dirent *entry;
	kf_FileInfo this;

	dir = opendir(path);
	while ((entry = readdir(dir)) != NULL) {
		gbString name_as_gb = 		gb_string_make(str_alloc, entry->d_name); /* Only the filename */
		gbString full_path =		kf_path_join(str_alloc, path, name_as_gb); /* Gets full path by joining dir path to file name */

		this.path =					full_path;
		this.is_dir =				(entry->d_type == DT_DIR); /* TODO(psiv): handle other DT_* possibilities */
		gb_array_append(entries, this);
	}
	closedir(dir);
}




/* GFX stuff */
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


kf_PlatformSpecificContext kf_get_platform_specific_context(void)
{
	return (kf_PlatformSpecificContext)&xinfo;
}

static void (*glXSwapIntervalEXT)(Display *display, GLXDrawable drawable, int vsync);
void kf_init_video(kf_PlatformSpecificContext ctx, gbString title, isize x, isize y, isize w, isize h, kf_VideoFlags flags)
{
	XInfo *xinfo;
	int num_returned = 0;
	isize final_w = w, final_h = h;

	xinfo = ctx;
	xinfo->display = XOpenDisplay(NULL);
	if (xinfo->display == NULL) {
		GB_PANIC("Could not open Xorg display");
	}

	xinfo->screen = DefaultScreen(xinfo->display);

	if ((flags & KF_VIDEO_MAXIMIZED) || w < 0 || h < 0) {
		final_w = XDisplayWidth(xinfo->display, xinfo->screen);
		final_h = XDisplayHeight(xinfo->display, xinfo->screen);
	}

	xinfo->window = XCreateSimpleWindow(xinfo->display, RootWindow(xinfo->display, xinfo->screen), x, y, final_w, final_h, 1, WhitePixel(xinfo->display, xinfo->screen), BlackPixel(xinfo->display, xinfo->screen));
	xinfo->del_atom = XInternAtom(xinfo->display, "WM_DELETE_WINDOW", 0);
	XSetWMProtocols(xinfo->display, xinfo->window, &xinfo->del_atom, 1);

	xinfo->mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
	XSelectInput(xinfo->display, xinfo->window, xinfo->mask);
	XStoreName(xinfo->display, xinfo->window, title);
	XMapWindow(xinfo->display, xinfo->window);

	xinfo->swap_flag = True;
	xinfo->fb_configs = glXChooseFBConfig(xinfo->display, DefaultScreen(xinfo->display), double_buf_attributes, &num_returned);
	GB_ASSERT(num_returned > 0);
	if (xinfo->fb_configs == NULL) { /* resort to single-buffer if DB not available */
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

void kf_set_vsync(kf_PlatformSpecificContext ctx, isize vsync)
{
	XInfo *xinfo;

	xinfo = ctx;
	glXSwapIntervalEXT(xinfo->display, xinfo->glx_window, vsync);
}

void kf_resize_window(kf_PlatformSpecificContext ctx, isize w, isize h)
{
	XInfo *xinfo;

	xinfo = ctx;
	XResizeWindow(xinfo->display, xinfo->window, (unsigned int)w, (unsigned int)h);
}

void kf_write_window_size(kf_PlatformSpecificContext ctx, isize *w, isize *h)
{
	XInfo *xinfo;
	XWindowAttributes attrs;
	int revert;

	xinfo = ctx;
	XGetWindowAttributes(xinfo->display, xinfo->window, &attrs);
	*w = (isize)attrs.width;
	*h = (isize)attrs.height;
}

void kf_swap_buffers(kf_PlatformSpecificContext ctx)
{
	XInfo *xinfo;

	xinfo = ctx;
	glXSwapBuffers(xinfo->display, xinfo->glx_window);
}

void kf_terminate_video(kf_PlatformSpecificContext ctx)
{
	XInfo *xinfo;

	xinfo = ctx;
	glXDestroyContext(xinfo->display, xinfo->glx_context);
	XCloseDisplay(xinfo->display);
}



void kf_analyze_events(kf_PlatformSpecificContext ctx, kf_EventState *out, bool await)
{
	XInfo *xinfo;
	XEvent evt;

	xinfo = ctx;

	/* Write initial state */
	/**out = (kf_EventState){0};*/ /* maybe this should be how we do it? idk */
	out->was_window_resized = false;

	if (await) {
		/* blocks until next event is recv'd, but DOES NOT REMOVE said event from queue so that
		it will still be caught during the while loop below */
		XPeekEvent(xinfo->display, &evt);
	}

	/* Now that we have recv'd our first event, actually go through the entire queue */
	while (XPending(xinfo->display)) {
		XNextEvent(xinfo->display, &evt);

		if (evt.type == ClientMessage) {
			if ((Atom)evt.xclient.data.l[0] == xinfo->del_atom) {
				out->exited = true;
			}
		} else if (evt.type == ResizeRequest) {
			out->was_window_resized = true;
			out->window_resize_width = evt.xresizerequest.width;
			out->window_resize_height = evt.xresizerequest.height;
		}
	}
}





/* Define system fonts list */
const u8 *kf_system_font_paths[] = {
	"/usr/share/fonts",
	"/usr/local/share/fonts",
};