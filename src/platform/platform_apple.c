#include "kf.h"

#include <Silicon/silicon.h>


typedef struct {
	NSWindow* window;
	bool exited;
} MInfo;


static MInfo minfo;


CVReturn displayCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext)
{
	return kCVReturnSuccess;
}

bool onWindowClose() {
	minfo.exited = 1;
	return true;
}


PlatformSpecificContext kf_get_platform_specific_context(void)
{
	return (PlatformSpecificContext)&minfo;
}


void kf_init_video(PlatformSpecificContext ctx, gbString title, isize x, isize y, isize w, isize h, bool maximized)
{
	// Create the window.
	NSWindowStyleMask window_style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;

	NSWindow *window = NSWindow_init(NSMakeRect(x, y, w, h), window_style, NSBackingStoreBuffered, false);
	NSWindow_setTitle(window, title);

	// Prepare to initialize OpenGL
	NSOpenGLPixelFormatAttribute attributes[] = {
		NSOpenGLPFANoRecovery,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAColorSize, 24,
		NSOpenGLPFAAlphaSize, 8,
		NSOpenGLPFADepthSize, 24,
		NSOpenGLPFAStencilSize, 8,
		0
	};
	CVDisplayLinkRef displayLink;
	NSOpenGLPixelFormat *format = NSOpenGLPixelFormat_initWithAttributes(attributes);
	NSOpenGLView *view = NSOpenGLView_initWithFrame(NSMakeRect(x, y, w, h), format);
	NSOpenGLView_prepareOpenGL(view);


	GLint swapInt = 1;
	NSOpenGLContext *context = NSOpenGLView_openGLContext(view);
	NSOpenGLContext_setValues(context, &swapInt, NSOpenGLContextParameterSwapInterval);

	CVDisplayLinkCreateWithCGDisplay(CGMainDisplayID(), &displayLink);
	CVDisplayLinkSetOutputCallback(displayLink, displayCallback, window);
	CVDisplayLinkStart(displayLink);

	NSOpenGLContext_makeCurrentContext(context);
	NSWindow_setContentView(window, (NSView*)view);
	NSWindow_setIsVisible(window, true);
	NSWindow_makeMainWindow(window);

	if (maximized) {
		NSWindow_setFrameAndDisplay(window, NSScreen_visibleFrame(NSScreen_mainScreen()), true, false);
	}
	minfo.exited = 0;

	func_to_SEL(onWindowClose);
	funcs[0] = onWindowClose;

	NSApplication_sharedApplication();
	NSApplication_setActivationPolicy(NSApp, NSApplicationActivationPolicyRegular);
	NSApplication_finishLaunching(NSApp);

}

void kf_resize_window(PlatformSpecificContext ctx, isize w, isize h)
{
	MInfo* context = ctx;
	NSWindow_setFrameAndDisplay(context->window, NSMakeRect(0, 0, w, h), true, true);
}


void kf_analyze_events(PlatformSpecificContext ctx, EventState *out)
{
	NSEvent *e = NSApplication_nextEventMatchingMask(NSApp, NSEventMaskAny, NSDate_distantFuture(), 0, true);
	NSApplication_sendEvent(NSApp, e);
	NSApplication_updateWindows(NSApp);

	out->exited = minfo.exited;
}