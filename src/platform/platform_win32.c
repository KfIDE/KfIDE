#include "kf.h"

#include <windows.h>


typedef struct {
	HWND hwnd;
	HDC hdc;
	HGLRC hglrc;

	kf_IRect frame;
	MSG proc_msg; /* NOTE(EimaMei): This comes from the WndProc function. */
} WInfo;


static WInfo winfo;


/* =============== IO stuffz by ps2star but also from EimaMei a bit ===============     */
/* IO stuff */
const u8 kf_path_separator = '\\';
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

	kf_FileInfo this;
	kf_String asterisk  = kf_string_copy_from_cstring(str_alloc, "*", KF_DEFAULT_GROW);
	kf_String full_path = kf_join_paths(str_alloc, path, asterisk); /* Gets full path by joining dir path to file name */
	kf_string_free(asterisk);

	HANDLE handle;
	WIN32_FIND_DATA file_data;

	handle = FindFirstFileW(full_path.ptr, &file_data);

	if (handle == INVALID_HANDLE_VALUE)
	{
		KF_PANIC("kf_read_dir: 'FindFirstFileW' outputted a 'INVALID_HANDLE_VALUE' value.");
	}

	do {
		//name_as_kf = 				kf_string_copy_from_cstring(str_alloc, entry->d_name, KF_DEFAULT_STRING_GROW); /* Only the filename */
		//kf_String full_path =		kf_join_paths(str_alloc, path, name_as_kf); /* Gets full path by joining dir path to file name */
		printf("\t%s %s\n", file_data.cFileName, file_data.cAlternateFileName);

		this.path =				    kf_string_copy_from_cstring(str_alloc, file_data.cFileName, KF_DEFAULT_STRING_GROW);
		this.is_file =				!(file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
		this.is_dir =				file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
		this.is_link =				this.is_file;
		kf_array_append(entries, &this);
	} while (FindNextFile(handle, &file_data) != 0); /* NOTE(EimaMei): do while statements suck. */

	FindClose(handle);
	printf("readh the end\n");
}



LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_CLOSE || msg == WM_QUIT) {
		winfo.proc_msg.hwnd = hwnd;
		winfo.proc_msg.message = msg;
		winfo.proc_msg.lParam = lparam;
		winfo.proc_msg.wParam = wparam;
	}
	else
	{
		winfo.proc_msg.message = 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}


kf_PlatformSpecificContext kf_get_platform_specific_context(void)
{
	return (kf_PlatformSpecificContext)&winfo;
}

void kf_init_video(kf_PlatformSpecificContext ctx, kf_String title, isize x, isize y, isize w, isize h, kf_VideoFlags flags)
{
	DWORD window_style = WS_CAPTION | WS_SYSMENU | WS_BORDER | WS_SIZEBOX | WS_MAXIMIZEBOX | WS_MINIMIZEBOX;
	HINSTANCE inh = GetModuleHandle(NULL);
	isize show_value = SW_NORMAL;

	WNDCLASSA class = {0};
	class.lpszClassName = "KfIDE-Class";
	class.hInstance = inh;
	class.hCursor = LoadCursor(NULL, IDC_ARROW);
	class.lpfnWndProc = WindowProc;
	RegisterClassA(&class);

	if (flags & KF_VIDEO_HIDDEN_WINDOW)
	{
		show_value = SW_HIDE;
	}
	else if (flags & KF_VIDEO_MAXIMIZED || (w == -1 && h == -1))
	{
		DEVMODE dm = {0};
    	dm.dmSize = sizeof(dm);
		isize index = 0;

		while (EnumDisplaySettings(NULL, index, &dm) != 0)
		{
			index++;
		}
		EnumDisplaySettingsW(NULL, index, &dm);


		w = dm.dmPelsWidth;
		h = dm.dmPelsHeight;
		show_value = SW_MAXIMIZE;
	}


	RECT adjust_rect = {x, y, w, h};
	AdjustWindowRect(&adjust_rect, window_style, 0);

	HWND hwnd = CreateWindowA("KfIDE-Class", title.ptr, window_style, x, y, w, h, NULL, NULL, inh, NULL);
	HDC hdc = GetDC(hwnd);


	PIXELFORMATDESCRIPTOR pfd = {0};
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;

	isize format = ChoosePixelFormat(hdc, &pfd);
	SetPixelFormat(hdc, format, &pfd);
	HGLRC glrc = wglCreateContext(hdc);
	wglMakeCurrent(hdc, glrc);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	ShowWindow(hwnd, show_value);
	UpdateWindow(hwnd);

	WInfo *context = ctx;
	context->hwnd = hwnd;
	context->hdc = hdc;
	context->hglrc = glrc;
	context->frame = KF_IRECT(adjust_rect.left, adjust_rect.top, adjust_rect.right, adjust_rect.bottom);
}

void kf_set_vsync(kf_PlatformSpecificContext ctx, isize vsync)
{
	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
	wglSwapIntervalEXT(vsync);
}

void kf_resize_window(kf_PlatformSpecificContext ctx, isize w, isize h)
{
	WInfo *context = ctx;
	MoveWindow(context->hwnd, context->frame.x, context->frame.y, w, h, true);
}

void kf_write_window_size(kf_PlatformSpecificContext ctx, isize *w, isize *h)
{
	WInfo *context = ctx;
	RECT frame;
	GetWindowRect(context->hwnd, &frame);

	if (w != NULL)
		*w = (isize)frame.right;
	if (h != NULL)
		*h = (isize)frame.bottom;
}

void kf_terminate_video(kf_PlatformSpecificContext ctx)
{
	WInfo *context = ctx;

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(context->hglrc);
	DestroyWindow(context->hwnd);
	DeleteDC(context->hdc);
}

void kf_analyze_events(kf_PlatformSpecificContext ctx, kf_EventState *out, bool await)
{
	WInfo *context = ctx;
	MSG msg = {0};

	if (winfo.proc_msg.message != 0)
		printf("%i\n", winfo.proc_msg.message);

	switch (winfo.proc_msg.message) {
		case WM_CLOSE:
		case WM_QUIT: {
			out->exited = true;
			break;
		}
		default: break;
	}

	while (PeekMessage(&msg, context->hwnd, 0u, 0u, PM_REMOVE)) {
		switch (msg.message) {
			case WM_MOUSEWHEEL: {
				out->mousewheel_y = GET_WHEEL_DELTA_WPARAM(msg.wParam);
				break;
			}
			default: break;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void kf_swap_buffers(kf_PlatformSpecificContext ctx) {
	WInfo *context = ctx;
	SwapBuffers(context->hdc);
}


/* Define system fonts list */
const u8 *kf_system_font_paths[KF_NUM_SYSTEM_FONT_PATHS] = {
	"C:/Windows/Fonts",
	"",
};