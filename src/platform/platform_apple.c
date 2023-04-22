#define __STDBOOL_H /* Note(EimaMei): fuck off LLVM */

#undef KF_PLATFORM_APPLE
#define KF_PLATFORM_APPLE /* NOTE(EimaMei): redefining this so that vscode can stfu */

#include "kf.h"

#include <Silicon/silicon.h>

#include <sys/stat.h>
#include <dirent.h>
#define local_array_size(array) sizeof(array) / sizeof(*array) // For convenience sake.

typedef struct {
	kf_Allocator heap_alloc;

	NSWindow *window;
	NSOpenGLContext *opengl_ctx;
	CVDisplayLinkRef display_link;
	bool exited;

	KF_ARRAY(kf_EditBox) *opened_tabs;
	isize *currently_opened_tab;
} MInfo;


static MInfo minfo;


/* =============== IO stuffz by ps2star ===============     */
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
		kfd_printf("NOTE: kf_path_exists() returned false");
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


/* =============== Window stuffz ===============     */



static CVReturn display_callback(CVDisplayLinkRef displayLink, const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext)
{
	return kCVReturnSuccess;
}

static bool on_window_close()
{
	minfo.exited = 1;
	return true;
}

// Define all of the functions that'll be activated when the menu item gets clicked.
void file_new() {
	printf("MainMenu/File/New\n");
}

void file_open() {
	KF_ARRAY(kf_EditBox) *opened_tabs = minfo.opened_tabs;

	kf_EditBox new_file;
	/* io_info */
	new_file.io_info.path = kf_string_copy_from_cstring(minfo.heap_alloc, "/Users/eimamei/Desktop/KfIDE/LICENSE", KF_DEFAULT_GROW);
	new_file.io_info.is_dir = false;
	new_file.io_info.is_link = false;
	new_file.io_info.is_file = true;

	/* display */
	new_file.display = malloc(strlen("LICENSE") + 1);
	strcpy(new_file.display, "LICENSE");

	/* content */
	kf_File file;
	kf_FileError error = kf_file_open(&file, new_file.io_info.path, KF_FILE_MODE_READ);

	if (error == KF_FILE_ERROR_NONE) {
		new_file.content = kf_file_read(file, minfo.heap_alloc, kf_file_size(file), 0);
		kf_file_close(file);
	}

	*minfo.currently_opened_tab = opened_tabs->length;

	printf("MainMenu/File/Open - %li tabs opened\n", opened_tabs->length);
	kf_array_append(opened_tabs, &new_file);
}
void file_close() { printf("MainMenu/File/Close\n"); }
void edit_undo() { printf("MainMenu/Edit/Undo\n"); }
void edit_redo() { printf("MainMenu/Edit/Redo\n"); }
void edit_cut() { printf("MainMenu/Edit/Cut\n"); }
void edit_copy() { printf("MainMenu/Edit/Copy\n"); }
void edit_paste() { printf("MainMenu/Edit/Paste\n"); }
void edit_delete() { printf("MainMenu/Edit/Delete\n"); }
void edit_select_all() { printf("MainMenu/Edit/SelectAll\n"); }

kf_PlatformSpecificContext kf_get_platform_specific_context(void)
{
	return (kf_PlatformSpecificContext)&minfo;
}

kf_String find_font(kf_Allocator alloc, kf_String font)
{
	bool font_exists;
	isize count = 0;
	kf_String selected_path = font;
	kf_String result;

	while (true) {
		font_exists = kf_path_exists(selected_path);

		if (!font_exists) {
			switch (count) {
				case 0: {
					selected_path = kf_string_copy_from_cstring(alloc, "/System/Library/Fonts/", KF_DEFAULT_STRING_GROW);
					kf_string_append_string(&selected_path, font);
					break;
				}
				case 1: {
					/* /Users/<USERNAME>/Library/<FONT> */
					break;
				}
				case 2: {
					result.ptr = NULL;
					return result;
				}
			}
			count++;
			continue;
		}

		return selected_path;
	}
	return result;
}

// Creates a submenu for the main menu.
NSMenu* create_submenu(NSMenu* main_menu, const char* title, NSMenuItem** items, size_t sizeof_items) {
	// First, create a submenu for the app's menu bar.
	NSMenuItem* submenu = malloc_class(NSMenuItem);
	NSMenu_addItem(main_menu, submenu);

	// Create a menu list for our submenu.
	NSMenu* new_menu = NSMenu_init(title);
	NSMenuItem_setSubmenu(submenu, new_menu);

	// Add the items to the new menu list.
	for (size_t i = 0; i < sizeof_items; i++) {
		NSMenu_addItem(new_menu, items[i]);
		release(items[i]);
	}
	release(submenu);

	return new_menu;
}


void kf_init_video(kf_PlatformSpecificContext ctx, kf_String title, isize x, isize y, isize w, isize h, kf_VideoFlags flags)
{
	/* Create the window. */
	NSWindowStyleMask window_style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;

	NSWindow *window = NSWindow_init(NSMakeRect(x, y, w, h), window_style, NSBackingStoreBuffered, false);
	NSWindow_setTitle(window, title.ptr);

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
	NSOpenGLContext *context = NSOpenGLView_openGLContext(view);
	NSOpenGLView_prepareOpenGL(view);


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

	si_func_to_SEL_with_name(SI_DEFAULT, "windowShouldClose", on_window_close);

	NSApplication_sharedApplication();
	NSApplication_setActivationPolicy(NSApp, NSApplicationActivationPolicyRegular);

	NSApplication_finishLaunching(NSApp);
}

void kf_set_vsync(kf_PlatformSpecificContext ctx, isize vsync)
{
	MInfo *context = ctx;
	i32 vsync_int = vsync;

	NSOpenGLContext_setValues(context->opengl_ctx, &vsync_int, NSOpenGLContextParameterSwapInterval);
}

void kf_resize_window(kf_PlatformSpecificContext ctx, isize w, isize h)
{
	MInfo *context = ctx;
	NSWindow_setFrameAndDisplay(context->window, NSMakeRect(0, 0, w, h), true, true);
}

void kf_write_window_size(kf_PlatformSpecificContext ctx, isize *w, isize *h)
{
	MInfo *context = ctx;
	NSRect frame = NSWindow_frame(context->window);

	if (w != NULL)
		*w = (isize)frame.size.width;
	if (h != NULL)
		*h = (isize)frame.size.height;
}

void kf_terminate_video(kf_PlatformSpecificContext ctx)
{
	MInfo *context = ctx;

	CVDisplayLinkStop(context->display_link);
	CVDisplayLinkRelease(context->display_link);

	release(NSWindow_contentView(context->window));
	NSApplication_terminate(NSApp, (id)context->window);
}


void kf_analyze_events(kf_PlatformSpecificContext ctx, kf_EventState *out, bool await)
{
	NSEvent *e = NSApplication_nextEventMatchingMask(NSApp, NSEventMaskAny, nil, 0, true);
	while (e == nil) {
		e = NSApplication_nextEventMatchingMask(NSApp, NSEventMaskAny, nil, 0, false);
	}
	NSApplication_sendEvent(NSApp, e);
	NSApplication_updateWindows(NSApp);

	out->exited = minfo.exited;
}


void kf_swap_buffers(kf_PlatformSpecificContext ctx) {
	MInfo *context = ctx;
	NSOpenGLContext_flushBuffer(context->opengl_ctx);
}


void kf_ui_init_menubars(kf_PlatformSpecificContext ctx, kf_Allocator allocator, KF_ARRAY(kf_EditBox) *opened_tabs, isize *currently_opened_tab) {
	MInfo *context = ctx;

	si_func_to_SEL(SI_DEFAULT, file_new);
	si_func_to_SEL(SI_DEFAULT, file_open);
	si_func_to_SEL(SI_DEFAULT, file_close);

	si_func_to_SEL(SI_DEFAULT, edit_undo);
	si_func_to_SEL(SI_DEFAULT, edit_redo);
	si_func_to_SEL(SI_DEFAULT, edit_cut);
	si_func_to_SEL(SI_DEFAULT, edit_copy);
	si_func_to_SEL(SI_DEFAULT, edit_paste);
	si_func_to_SEL(SI_DEFAULT, edit_delete);
	si_func_to_SEL(SI_DEFAULT, edit_select_all);

	// Get the executable name.
	const char* process_name = NSProcessInfo_processName(NSProcessInfo_processInfo());

	// Create and set the main menubar
	NSMenu* main_menu = autorelease(malloc_class(NSMenu));
	NSApplication_setMainMenu(NSApp, main_menu);

	minfo.heap_alloc = allocator;
	minfo.opened_tabs = opened_tabs;
	minfo.currently_opened_tab = currently_opened_tab;

	// The items for each of our menus ('<Executable name>', 'File', 'Edit', 'View', 'Windows' and 'Help')
	// '<Executable name>' items
	NSMenuItem* process_items[] = {
		/*
		 The 'selector' macro is an equivalent to Objective-C's '@selector(<...>:)', meaning you can provide
		 any existing class method as an argument, which is why 'orderFrontStandardAboutPanel' shows information
		 about the app (even though it's technically "undefined").
		*/
		NSMenuItem_init("About", selector(orderFrontStandardAboutPanel), ""),
		NSMenuItem_separatorItem(), // Adds a separator dash between the menu items.
		NSMenuItem_init("Services", nil, ""), // Define this for later.
		NSMenuItem_init("Hide", selector(hide), "h"), // The same 'orderFrontStandardAboutPanel' behaviour happens for everything below (apart from the separator).
		NSMenuItem_init("Hide Other", selector(hideOtherApplications), "h"),
		NSMenuItem_init("Show All", selector(unhideAllApplications), ""),
		NSMenuItem_separatorItem(),
		NSMenuItem_init("Quit", selector(terminate), "q")
	};

	NSMenuItem* file_items[] = {
		NSMenuItem_init("New", selector(file_new), "n"),
		NSMenuItem_init("Open", selector(file_open), "o"),
		NSMenuItem_separatorItem(),
		NSMenuItem_init("Close", selector(file_close), "w")
	};

	NSMenuItem* edit_items[] = {
		NSMenuItem_init("Undo", selector(edit_undo), "z"),
		NSMenuItem_init("Redo", selector(edit_redo), "Z"),
		NSMenuItem_separatorItem(),
		NSMenuItem_init("Cut", selector(edit_cut), "x"),
		NSMenuItem_init("Copy", selector(edit_copy), "c"),
		NSMenuItem_init("Paste", selector(edit_paste), "v"),
		NSMenuItem_init("Delete", selector(edit_delete), "\b"),
		NSMenuItem_separatorItem(),
		NSMenuItem_init("Select all", selector(edit_select_all), "a")
	};


	// Now we create the menus themselves.
	// '<Process name>' menu
	NSMenu* process_menu = create_submenu(main_menu, process_name, process_items, local_array_size(process_items));
	NSMenu* process_services = autorelease(malloc_class(NSMenu)); // We initialize a new menu.
	NSMenuItem_setSubmenu(NSMenu_itemArray(process_menu)[2], process_services); // 'NSMenu_itemArray(process_menu)[2]' is 'Services' (refer to 'process_items' variable).
	NSApplication_setServicesMenu(NSApp, process_services); // Now 'Services' becomes a Services menu.

	// 'File' menu
	create_submenu(main_menu, "File", file_items, local_array_size(file_items));

	// 'File' menu
	create_submenu(main_menu, "Edit", edit_items, local_array_size(edit_items));

	// 'View' menu
	create_submenu(main_menu, "View", nil, 0); // For whatever reason, MacOS turns a "View" titled menu automatically into a View menu.

	// 'Windows' menu
	NSMenu* windows_menu = create_submenu(main_menu, "Windows", nil, 0);
	NSApplication_setWindowsMenu(NSApp, windows_menu); // Set our menu into a Windows menu.

	// 'Help' menu
	NSMenu* help_menu = create_submenu(main_menu, "Help", nil, 0);
	NSApplication_setHelpMenu(NSApp, help_menu); // Set our menu into a Help menu.

	NSApplication_finishLaunching(NSApp);
}



/* Define system fonts list */
const u8 *kf_system_font_paths[KF_NUM_SYSTEM_FONT_PATHS] = {
	"/System/Library/Fonts",
	"",
};