#define GB_IMPLEMENTATION
#include "gb.h"
#include "kf.h"

/* C include */
#include "translation.c"
#include "math.c"
#include "text.c"
#include "ui.c"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"


typedef struct {
	gbAllocator heap_alloc, global_alloc, temp_alloc, ui_alloc;
	gbArena temp_backing, global_backing, ui_backing;

	PlatformSpecificContext platform_context;

	IRect win_bounds;
	EventState event_state;
	TranslationRecord translation_record;
	UIContext ui_context;
} GlobalVars;

static GlobalVars g;



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
	#define UI_SIZE			(isize)gb_kilobytes(512)

	g.heap_alloc = gb_heap_allocator();

	gb_arena_init_from_allocator(&g.global_backing, g.heap_alloc, GLOBAL_SIZE);
	g.global_alloc = gb_arena_allocator(&g.global_backing);
	gb_arena_init_from_allocator(&g.temp_backing, g.heap_alloc, TEMP_SIZE);
	g.temp_alloc = gb_arena_allocator(&g.temp_backing);
	gb_arena_init_from_allocator(&g.ui_backing, g.heap_alloc, UI_SIZE);
	g.ui_alloc = gb_arena_allocator(&g.ui_backing);

	/* Gfx init */
	g.platform_context = kf_get_platform_specific_context();
  
	kf_init_video(g.platform_context, "hmm suspicious", 0, 0, -1, -1, KF_VIDEO_MAXIMIZED);
	kf_set_vsync(g.platform_context, 1);

	/*glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	u32 font_id = init_font(gb_string_make(g.temp_alloc, "geneva.ttf"), 64);
	/* Translations init */
	static const char *test_str = "en;;nl\nTEST;;TEST";
	kf_load_translations_from_csv_string(g.global_alloc, &g.translation_record, test_str, strlen(test_str));

	while (true) {
		kf_analyze_events(g.platform_context, &g.event_state, true);

		EventState *e = &g.event_state;
		if (e->exited) {
			break;
		}

		/* UI */
		{
			UIContext *ui = &g.ui_context;

			/* UI BEGIN */
			kf_ui_begin(ui, g.ui_alloc, 196, &g.event_state); {
				kf_ui_push_origin(ui, 0, 0); {
					kf_ui_color(ui, KF_RGBA(255, 255, 255, 255));
					kf_ui_rect(ui, (IRect){ 0, 0, 200, 200 });
				} kf_ui_pop_origin(ui);
			} kf_ui_end(ui, false);
		}

		/* Render UI */
		isize ui_index;
		for (ui_index = 0; ui_index < gb_array_count(g.ui_context.draw_commands); ui_index++) {
			/* TODO(psiv): implement UI rendering */
		}
    
		gb_free_all(g.temp_alloc); /* clear temp buffer every frame (can grow infinitely if this isn't called periodically) */
		gb_free_all(g.ui_alloc);
	}
	kf_terminate_video(g.platform_context);
	return 0;
}