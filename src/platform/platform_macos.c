#include "kf.h"

typedef struct {

} MInfo;
static MInfo minfo;

PlatformSpecificContext kf_get_platform_specific_context(void)
{
	return (PlatformSpecificContext)&minfo;
}

void kf_init_video(PlatformSpecificContext ctx, gbString title, isize x, isize y, isize w, isize h)
{

}