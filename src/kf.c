#define GB_IMPLEMENTATION
#include "gb.h"
#include "kf.h"

/* C include */
#include "translation.c"
#include "text.c"


/* To test if OpenGL is properly initialized. */
void opengl_test(u32 font_id)
{
	glClearColor(0.1, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); /* NOTE(EimaMei): Not sure if this should be a function on its own, so I'll just leave it here (Based on https://www.khronos.org/opengl/wiki/Common_Mistakes#Swap_Buffers). */

	glBegin(GL_TRIANGLES);
		glColor3f(1, 0, 0); glVertex2f(-1.0f, -1.0f);
		glColor3f(0, 1, 0); glVertex2f( 1.0f, -1.0f);
		glColor3f(0, 0, 1); glVertex2f( 0.0f,  1.0f);
	glEnd();

	draw_text(font_id, "BEEP REPAIR", 0, 64, RGB(255, 0, 0));

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
	kf_init_video(g.platform_context, "hmm suspicious", 0, 0, 512, 512, true);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);

	u32 font_id = init_font(gb_string_make(g.temp_alloc, "geneva.ttf"), 64);
	/* Translations init */
	static const char *test_str = "en;;nl\nTEST;;TEST";
	kf_load_translations_from_csv_string(g.global_alloc, &g.translation_record, test_str, strlen(test_str));

	while (true) {
		kf_analyze_events(g.platform_context, &g.event_state);
		EventState *e = &g.event_state;

		if (e->exited) {
			break;
		}

		opengl_test(font_id);

		gb_free_all(g.temp_alloc); /* clear temp buffer every frame (can grow infinitely if this isn't called periodically) */
	}
	kf_terminate_video(g.platform_context);
	return 0;
}