#include "kf.h"

#include <windows.h>

typedef struct {
	WNDCLASSEX hWnd;
} WInfo;
static WInfo winfo;

PlatformSpecificContext kf_get_platform_specific_context(void)
{
	return (PlatformSpecificContext)&winfo;
}

void kf_init_video(PlatformSpecificContext ctx, gbString title, isize x, isize y, isize w, isize h)
{
	
}