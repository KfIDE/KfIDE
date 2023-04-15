#define GB_IMPLEMENTATION
#include "gb.h"
#include "kf.h"

/* C includes */
#include "math.c"


typedef struct {
	PlatformSpecificContext platform_context;

	IRect win_bounds;
	EventState event_state;
} GlobalVars;


static GlobalVars g;


int main(int argc, char **argv)
{
	g.platform_context = kf_get_platform_specific_context();
	kf_init_video(g.platform_context, "hmm suspicious", 0, 0, 320, 240);

	while (true) {
		kf_analyze_events(g.platform_context, &g.event_state);
	}
	return 0;
}