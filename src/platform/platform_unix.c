#include "kf.h"

#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>

#define _XOPEN_SOURCE 500
#define __USE_XOPEN_EXTENDED
#include <sys/types.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

/* IO stuff */
const u8 kf_path_separator = '/';
static kf_FileError _translate_file_error(void)
{
	if (errno == ENOENT) {
		return KF_FILE_ERROR_DOES_NOT_EXIST;
	} else if (errno == EACCES) {
		return KF_FILE_ERROR_PERMISSION;
	} else if (errno == EINVAL) {
		return KF_FILE_ERROR_INVALID;
	} else {
		return KF_FILE_ERROR_UNSPECIFIED;
	}
}

static const char *_translate_mode(kf_FileMode mode)
{
	if (mode & KF_FILE_MODE_READ) {
		return "r";
	} else if (mode & KF_FILE_MODE_WRITE) {
		return "w";
	} else if (mode & KF_FILE_MODE_READ_WRITE) {
		return "r+";
	} else if (mode & KF_FILE_MODE_CREATE) {
		return "w+";
	} else if (mode & KF_FILE_MODE_APPEND) {
		return "a";
	} else if (mode & KF_FILE_MODE_APPEND_READ) {
		return "a+";
	}

	KF_PANIC("Invalid mode passed to <linux> _translate_mode()");
}

kf_FileError kf_file_open(kf_File *file, kf_String path, kf_FileMode mode)
{
	file->libc_file = fopen(path.ptr, _translate_mode(mode));
	if (file->libc_file == NULL) {
		kfd_printf("WARN: fopen() returned NULL.");
		return _translate_file_error();
	}

	file->full_path = path;
	file->operating_mode = KF_FILE_MODE_READ_WRITE;
	return KF_FILE_ERROR_NONE;
}

kf_String kf_file_read(kf_File file, kf_Allocator str_alloc, isize bytes_to_read, isize at)
{
	kf_String out = kf_string_make(str_alloc, bytes_to_read, bytes_to_read, KF_DEFAULT_STRING_GROW);
	kf_file_read_into_cstring(file, out.ptr, bytes_to_read, at);
	return out;
}

void kf_file_read_into_cstring(kf_File file, u8 *buf, isize bytes_to_read, isize at)
{
	KF_ASSERT(file.libc_file != NULL && buf != NULL && at > -1 && bytes_to_read > 0);

	u64 old_pos = ftell(file.libc_file);
	fseek(file.libc_file, at, SEEK_SET);
	fread(buf, bytes_to_read, 1, file.libc_file);
	fseek(file.libc_file, old_pos, SEEK_SET);
}

bool kf_path_exists(kf_String path)
{
	FILE *fp;

	fp = fopen(path.ptr, "r");
	if (fp == NULL) {
		kfd_printf("NOTE: kf_file_exists() returned false");
		return false;
	}

	fclose(fp);
	return true;
}

void kf_file_close(kf_File file)
{
	KF_ASSERT(file.libc_file != NULL);

	fclose(file.libc_file);
}

u64 kf_file_size(kf_File file)
{
	u64 old_pos = ftell(file.libc_file);
	fseek(file.libc_file, 0, SEEK_END);
	u64 size_pos = ftell(file.libc_file);
	fseek(file.libc_file, old_pos, SEEK_SET);
	return size_pos;
}

void kf_read_dir(kf_String path, KF_ARRAY(kf_FileInfo) *entries, kf_Allocator str_alloc)
{
	KF_ASSERT(path.ptr != NULL && entries != NULL);

	DIR *dir;
	struct dirent *entry;
	kf_FileInfo this;
	kf_String name_as_kf;

	dir = opendir(path.ptr);
	while ((entry = readdir(dir)) != NULL) {
		name_as_kf = 				kf_string_copy_from_cstring(str_alloc, entry->d_name, KF_DEFAULT_STRING_GROW); /* Only the filename */
		kf_String full_path =		kf_join_paths(str_alloc, path, name_as_kf); /* Gets full path by joining dir path to file name */

		this.path =					full_path;
		this.is_file =				(entry->d_type == DT_REG);
		this.is_dir =				(entry->d_type == DT_DIR);
		this.is_link =				(entry->d_type == DT_LNK);
		kf_array_append(entries, &this);
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
void kf_init_video(kf_PlatformSpecificContext ctx, kf_String title, isize x, isize y, isize w, isize h, kf_VideoFlags flags)
{
	XInfo *xinfo;
	int num_returned = 0;
	isize final_w = w, final_h = h;

	xinfo = ctx;
	xinfo->display = XOpenDisplay(NULL);
	if (xinfo->display == NULL) {
		KF_PANIC("Could not open Xorg display");
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
	XStoreName(xinfo->display, xinfo->window, title.ptr);
	XMapWindow(xinfo->display, xinfo->window);

	xinfo->swap_flag = True;
	xinfo->fb_configs = glXChooseFBConfig(xinfo->display, DefaultScreen(xinfo->display), double_buf_attributes, &num_returned);
	KF_ASSERT(num_returned > 0);
	if (xinfo->fb_configs == NULL) { /* resort to single-buffer if DB not available */
		xinfo->fb_configs = glXChooseFBConfig(xinfo->display, DefaultScreen(xinfo->display), single_buf_attributes, &num_returned);
		KF_ASSERT(num_returned > 0);
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
const u8 *kf_system_font_paths[KF_NUM_SYSTEM_FONT_PATHS] = {
	"/usr/share/fonts",
	"/usr/local/share/fonts",
	"",
};