#include "kf.h"

#include <Silicon/silicon.h>


typedef struct {
	NSWindow *window;
	NSOpenGLContext *opengl_ctx;
	CVDisplayLinkRef display_link;
	bool exited;
} MInfo;

typedef enum {
	thing1 = KF_BIT(0),
	thing2 = KF_BIT(1),
};


static MInfo minfo;


static CVReturn display_callback(CVDisplayLinkRef displayLink, const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext)
{
	return kCVReturnSuccess;
}

static bool on_window_close()
{
	minfo.exited = 1;
	return true;
}


kf_PlatformSpecificContext kf_get_platform_specific_context(void)
{
	return (kf_PlatformSpecificContext)&minfo;
}

bool kf_file_exists(gbString path)
{
	struct stat buffer;
  	return (stat(path, &buffer) == 0);
}

gbString find_font(gbString font)
{
	bool font_exists;
	isize count = 0;
	gbString selected_path = font;

	while (true) {
		font_exists = kf_file_exists(selected_path);

		if (!font_exists) {
			switch (count) {
				case 0: {
					selected_path = gb_string_make(g.heap_alloc, "/System/Library/Fonts/");
					selected_path = gb_string_append(selected_path, font);
					break;
				}
				case 1: {
					/* /Users/<USERNAME>/Library/<FONT> */
					break;
				}
				case 2: return NULL;
			}
			count++;
			continue;
		}

		return selected_path;
	}
	return NULL;
}


void kf_init_video(kf_PlatformSpecificContext ctx, gbString title, isize x, isize y, isize w, isize h, kf_VideoFlags flags)
{
	/* Create the window. */
	NSWindowStyleMask window_style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;

	NSWindow *window = NSWindow_init(NSMakeRect(x, y, w, h), window_style, NSBackingStoreBuffered, false);
	NSWindow_setTitle(window, title);

	/* Prepare to initialize OpenGL */
	NSOpenGLPixelFormatAttribute attributes[] = {
		NSOpenGLPFANoRecovery,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAColorSize, 24,
		NSOpenGLPFAAlphaSize, 8,
		NSOpenGLPFADepthSize, 24,
		NSOpenGLPFAStencilSize, 8,
		0,
	};
	CVDisplayLinkRef display_link;
	NSOpenGLPixelFormat *format = NSOpenGLPixelFormat_initWithAttributes(attributes);
	NSOpenGLView *view = NSOpenGLView_initWithFrame(NSMakeRect(x, y, w, h), format);
	NSOpenGLView_prepareOpenGL(view);


	GLint swap_int = 1;
	NSOpenGLContext *context = NSOpenGLView_openGLContext(view);
	NSOpenGLContext_setValues(context, &swap_int, NSOpenGLContextParameterSwapInterval);

	CGDirectDisplayID displayID = CGMainDisplayID();
	CVDisplayLinkCreateWithCGDisplay(displayID, &display_link);
	CVDisplayLinkSetOutputCallback(display_link, display_callback, window);
	CVDisplayLinkStart(display_link);

	NSOpenGLContext_makeCurrentContext(context);
	NSWindow_setContentView(window, (NSView*)view);
	NSWindow_setIsVisible(window, true);
	NSWindow_makeMainWindow(window);

	if (flags & KF_VIDEO_MAXIMIZED) {
		NSWindow_setFrameAndDisplay(window, NSScreen_visibleFrame(NSScreen_mainScreen()), true, false);
	}
	minfo.window = window;
	minfo.opengl_ctx = context;
	minfo.display_link = display_link;
	minfo.exited = 0;

	func_to_SEL(on_window_close);
	funcs[0] = on_window_close;

	NSApplication_sharedApplication();
	NSApplication_setActivationPolicy(NSApp, NSApplicationActivationPolicyRegular);
	NSApplication_finishLaunching(NSApp);

}

void kf_resize_window(kf_PlatformSpecificContext ctx, isize w, isize h)
{
	MInfo *context = ctx;
	NSWindow_setFrameAndDisplay(context->window, NSMakeRect(0, 0, w, h), true, true);
}

void kf_terminate_video(kf_PlatformSpecificContext ctx)
{
	MInfo *context = ctx;
	CVDisplayLinkStop(context->display_link);
	CVDisplayLinkRelease(context->display_link);
	NSView_release(NSWindow_contentView(context->window));
	NSApplication_terminate(NSApp, (id)context->window);
}


void kf_analyze_events(kf_PlatformSpecificContext ctx, kf_EventState *out, bool await)
{
	NSEvent *e = NSApplication_nextEventMatchingMask(NSApp, NSEventMaskAny, NSDate_distantFuture(), 0, true);
	NSApplication_sendEvent(NSApp, e);
	NSApplication_updateWindows(NSApp);

	out->exited = minfo.exited;
}


void kf_swap_buffers(kf_PlatformSpecificContext ctx) {
	MInfo *context = ctx;
	NSOpenGLContext_flushBuffer(context->opengl_ctx);
}