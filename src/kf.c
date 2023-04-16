#define GB_IMPLEMENTATION
#include "gb.h"
#include "kf.h"

/* C include */
#include "translation.c"


typedef struct {
	gbAllocator heap_alloc, global_alloc, temp_alloc;
	PlatformSpecificContext platform_context;

	IRect win_bounds;
	EventState event_state;
	TranslationRecord translation_record;
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

	glFlush();
	glFinish();

	kf_swap_buffers(g.platform_context);
}


int main(int argc, char **argv)
{
	/* Allocators init */
	#define GLOBAL_SIZE		(isize)gb_megabytes(1)
	#define TEMP_SIZE		(isize)gb_megabytes(1)

	g.heap_alloc = gb_heap_allocator();

	gb_arena_init_from_allocator(&g.global_backing, g.heap_alloc, GLOBAL_SIZE);
	g.global_alloc = gb_arena_allocator(&g.global_backing);
	gb_arena_init_from_allocator(&g.temp_backing, g.heap_alloc, TEMP_SIZE);
	g.temp_alloc = gb_arena_allocator(&g.temp_backing);

	/* Gfx init */
	g.platform_context = kf_get_platform_specific_context();
	kf_init_video(g.platform_context, "hmm suspicious", 0, 0, 320, 240, true);
	kf_set_vsync(g.platform_context, true);

	/* Translations init */
	kf_load_translations_from_file(&g.translation_record);

	while (true) {
		kf_analyze_events(g.platform_context, &g.event_state);
		EventState *e = &g.event_state;

		if (e->exited) {
			break;
		}

		opengl_test();
		gb_free_all(g.temp_alloc); /* clear temp buffer every frame (can grow infinitely if this isn't called periodically) */
	}
	kf_terminate_video(g.platform_context);
	return 0;
}