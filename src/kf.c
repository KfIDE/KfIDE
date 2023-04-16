#define GB_IMPLEMENTATION
#include "gb.h"
#include "kf.h"


typedef struct {
	PlatformSpecificContext platform_context;

	IRect win_bounds;
	EventState event_state;
} GlobalVars;


static GlobalVars g;


/* To test if OpenGL is properly initialized. */
void opengl_test()
{
	glClearColor(0.1, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); /* Not sure if this should be a function on its own, so I'll just leave it here (Based on https://www.khronos.org/opengl/wiki/Common_Mistakes#Swap_Buffers). */

	glBegin(GL_TRIANGLES);
		glColor3f(1, 0, 0); glVertex2f(-1.0f, -1.0f);
		glColor3f(0, 1, 0); glVertex2f( 1.0f, -1.0f);
		glColor3f(0, 0, 1); glVertex2f( 0.0f,  1.0f);
	glEnd();

	kf_flush_buffer(g.platform_context);
}


int main(int argc, char **argv)
{
	g.platform_context = kf_get_platform_specific_context();
	kf_init_video(g.platform_context, "hmm suspicious", 0, 0, 320, 240, true);

	while (true) {
		kf_analyze_events(g.platform_context, &g.event_state);
		EventState *e = &g.event_state;

		if (e->exited) {
			break;
		}

		opengl_test();
	}
	kf_terminate_video(g.platform_context);

	return 0;
}