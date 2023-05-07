#include "kf.h"

/* C include */
#include "mem.c"
#include "time.c"
#include "math.c"
#include "translation.c"
#include "font.c"
#include "ui.c"
#include "str_util.c"
#include "io_util.c"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


typedef struct {
	kf_Allocator heap_alloc, global_alloc, temp_alloc, ui_alloc;
	kf_TempAllocatorData global_backing, temp_backing, ui_backing;

	kf_PlatformSpecificContext platform_context;

	kf_Time debug_timer; /* for measuring perf. */

	kf_IRect win_bounds;
	kf_EventState event_state;
	kf_TranslationRecord translation_record;
	kf_UIContext ui_context;

	KF_ARRAY(kf_String) available_fonts_list; /* list of paths to avail. system fonts */
	kf_Font canon_editing_font;

	kf_Font font_std32;

	KF_ARRAY(kf_EditBox) opened_tabs;
	isize currently_opened_tab;
} GlobalVars;
static GlobalVars g;




void kf_load_ttf_font(kf_Font *out, kf_Allocator alloc, kf_Allocator temp_alloc, kf_String path, kf_IVector2 glyphset, isize pt_size)
{
	if (!kf_path_exists(path)) {
		kfd_printf("Error: '%s' doesn't exist", path.ptr);
	}

	kf_FileContents contents = kf_file_read_contents(alloc, true, path);
	if (!(contents.data != NULL && contents.size > 0)) {
		KF_PANIC("kf_load_ttf_font(): invalid file path given.");
	}

	/* now we can init our arrays since we know how many runes we'll be needing */
	isize num_runes = glyphset.y - glyphset.x;
	out->glyphs = kf_array_make(alloc, sizeof(kf_GlyphInfo), 0, num_runes, KF_DEFAULT_GROW);

	stbtt_InitFont(&out->stb_font, contents.data, 0);
	f32 scale = stbtt_ScaleForPixelHeight(&out->stb_font, pt_size);
	out->size = pt_size;

	/* idk if we actually need to do any of the stuff in this block or not */
	{
		int ascent, descent, line_gap;
	    stbtt_GetFontVMetrics(&out->stb_font, &ascent, &descent, &line_gap);

	    ascent = roundf(ascent * scale); /* from <stdlib.h> apparently */
	    descent = roundf(descent * scale);

	    out->ascent = (isize)ascent;
	    out->descent = (isize)descent;
	    out->line_gap = (isize)line_gap;

		out->scale = scale;
	}

	isize bitmap_w, bitmap_h;
	bitmap_w = (isize)(1.10f * (f32)(pt_size + 1)); /* 10% more than pt_size */
	bitmap_h = (isize)(1.10f * (f32)(pt_size + 1));

	KF_ASSERT(pt_size < bitmap_h);

	/* allocate a bitmap for the glyph */
	isize bitmap_size = bitmap_w * bitmap_h;
	u8 *bitmap = (u8 *)kf_alloc(alloc, bitmap_size * sizeof(u8));
	memset(bitmap, 0, bitmap_size);

	/*
	Go through each rune from the list and do a couple of things:
	1) Create a corresponding glyph index entry (faster than using codepoints the entire time)
	2) Grab metrics
	3) Render to bitmap
	4) Generate gl textures from bitmap; obviously store the tex IDs into the font too
	*/

	kf_write_current_time(&g.debug_timer); /* curious how long this loop takes, so let's time it */
	int x = 0;

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	isize i;
	for (i = 0; i < num_runes; i++) {
		/* Step 1 */
		kf_array_append(&out->glyphs, &(kf_GlyphInfo){0});
		kf_GlyphInfo *this_glyph = kf_array_get(out->glyphs, i);

		int glyph_index = stbtt_FindGlyphIndex(&out->stb_font, i);
		this_glyph->index = glyph_index;

		int ax, lsb;
		stbtt_GetGlyphHMetrics(&out->stb_font, glyph_index, &ax, &lsb);
		this_glyph->ax = ax;
		this_glyph->lsb = lsb;

		/* Step 2 */
		int c_x1, c_y1, c_x2, c_y2;
		stbtt_GetGlyphBitmapBox(&out->stb_font, glyph_index, scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);
		/*printf("BM BOX: %d %d %d %d\n", c_x1, c_y1, c_x2, c_y2);*/
		this_glyph->x1 = c_x1;
		this_glyph->y1 = c_y1;

		int glyph_w = c_x2 - c_x1;
		int glyph_h = c_y2 - c_y1;

		this_glyph->width = (isize)glyph_w;
		this_glyph->height = (isize)glyph_h;

		/* Step 3 */
		int byte_offset = 0; /* idk how this works */
		u8 *final_bm = bitmap + byte_offset; /* first glyph pixel in the bitmap (not always 0,0 i guesss) */

		/* this renders the ONE-CHANNEL glyph into the bitmap
		but since we can't tint a 1-channel (GL_RED) tex (i think) we're gonna have to
		expand it to RGBA; this is why we allocated 4 * sizeof(u8) earlier */
		stbtt_MakeGlyphBitmap(&out->stb_font, bitmap, glyph_w, glyph_h, bitmap_w, scale, scale, glyph_index);

		/* Step 4 */
		GLuint gl_tex;
		glPixelStorei(GL_UNPACK_ROW_LENGTH, bitmap_w);

		glGenTextures(1, &gl_tex);
		glBindTexture(GL_TEXTURE_2D, gl_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, glyph_w, glyph_h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, bitmap);
		//glGenerateMipmap(GL_TEXTURE_2D);

		this_glyph->gl_tex = gl_tex;
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	kf_print_time_since(&g.debug_timer);
}


kf_IVector2 kf_center_rect(kf_IRect largerRect, kf_IRect smallerRect) {
	kf_IVector2 res;
    res.x = (isize)((largerRect.w - smallerRect.w) / 2) + largerRect.x;
    res.y = (isize)((largerRect.h - smallerRect.h) / 2) + largerRect.y;

    return res;
}


int main(int argc, char **argv)
{
	/* Allocators init */
	{
		#define GLOBAL_SIZE		(isize)KF_MEGA(2)
		#define TEMP_SIZE		(isize)KF_MEGA(1)
		#define UI_SIZE			(isize)KF_KILO(512)
		/* #define GEN_SIZE		(isize)kf_megabytes(1) */ /* Not cleared each frame; manually clear memory */

		g.heap_alloc = kf_heap_allocator();

		/* Temp allocators */
		kf_init_temp_allocator_data(&g.global_backing, g.heap_alloc, GLOBAL_SIZE);
		g.global_alloc = kf_temp_allocator(&g.global_backing);
		kf_init_temp_allocator_data(&g.temp_backing, g.heap_alloc, TEMP_SIZE);
		g.temp_alloc = kf_temp_allocator(&g.temp_backing);
		kf_init_temp_allocator_data(&g.ui_backing, g.heap_alloc, UI_SIZE);
		g.ui_alloc = kf_temp_allocator(&g.ui_backing);

		#ifdef KF_DEBUG
			printf("Allocators successfully have been initialized\n");
		#endif
	}

	/* Gfx init */
	{
		g.platform_context = kf_get_platform_specific_context();
		kf_init_video(g.platform_context, kf_string_set_from_cstring("hmm suspicious"), 0, 0, -1, -1, KF_VIDEO_MAXIMIZED);
		kf_set_vsync(g.platform_context, 1);

		#ifdef KF_PLATFORM_APPLE
			kf_ui_init_menubars(g.platform_context, g.global_alloc, &g.opened_tabs, &g.currently_opened_tab);
		#endif

		#ifdef KF_DEBUG
			printf("GFX successfully have been initialized\n");
		#endif
	}

	/* Translations init */
	{
		static const char *test_str = "en;;nl\nTEST;;TEST";
		kf_load_translations_from_csv_buffer(g.global_alloc, &g.translation_record, (u8 *)test_str, strlen(test_str));

		#ifdef KF_DEBUG
			printf("Translations successfully have been initialized\n");
		#endif
	}

	/* Font init */
	kf_free_all(g.temp_alloc); {
		/* Allocates a KF_ARRAY(kf_String) (inner strings allocated with 2nd param) with paths to system fonts */
		g.available_fonts_list = kf_array_make(g.heap_alloc, sizeof(kf_String), 0, 512, KF_DEFAULT_GROW);

		kf_query_system_fonts(g.temp_alloc, &g.available_fonts_list);
		kfd_printf("Initial font query returned %ld fonts", (g.available_fonts_list).length);
		kfd_print_system_fonts(g.available_fonts_list);

		kf_load_ttf_font(&g.font_std32, g.global_alloc, g.temp_alloc, kf_string_set_from_cstring("res/eurostile.ttf"), KF_IVECTOR2(0, 255), 32);

		#ifdef KF_DEBUG
			printf("Fonts successfully have been initialized\n");
		#endif
	} kf_free_all(g.temp_alloc);

	/* Other global initializations. */
	{
		g.opened_tabs = kf_array_make(g.heap_alloc, sizeof(kf_EditBox), 0, 8, 8);
		g.currently_opened_tab = -1;

		#ifdef KF_DEBUG
			printf("Other global initializations successfully have been initialized\n");
		#endif
	}

	//#define KF_PRINT_ALLOCS

	while (true) {
		kf_analyze_events(g.platform_context, &g.event_state, true);

		kf_EventState *e = &g.event_state;
		if (e->exited) {
			break;
		}

		/* NOTE(EimaMei): Prints how much memory is allocated out of the available limit in kilobytes. */
		{
			#ifdef KF_DEBUG
				#ifdef KF_PRINT_ALLOCS
					kf_TempAllocatorData *alloc_data = (kf_TempAllocatorData *)g.global_alloc.user;
					printf("MEMORY ALLOCATIONS:\n\tGlobal alloc - %lld/%lld KB\n", alloc_data->position / KF_KILO(1), alloc_data->size / KF_KILO(1));

					alloc_data = (kf_TempAllocatorData *)g.temp_alloc.user;
					printf("\tsTemp alloc - %lld/%lld KB\n", alloc_data->position / KF_KILO(1), alloc_data->size / KF_KILO(1));

					alloc_data = (kf_TempAllocatorData *)g.ui_alloc.user;
					printf("\tUI alloc - %lld/%lld KB\n", alloc_data->position / KF_KILO(1), alloc_data->size / KF_KILO(1));
				#endif
			#endif
		}


		/* Gfx start */
		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		#ifdef KF_PLATFORM_LINUX /* NOTE(EimaMei): linux sucks */
			kf_write_window_size(g.platform_context, &g.win_bounds.w, &g.win_bounds.h);
			glViewport(0, 0, g.win_bounds.w, g.win_bounds.h);
		#endif

		/* UI */
		{
			kf_UIContext *ui = &g.ui_context;

			kf_String debug_text = kf_string_set_from_cstring("hmm suspicious");

			/* UI BEGIN */
			kf_ui_begin(ui, g.ui_alloc, 196, &g.event_state); {
				kf_ui_push_origin(ui, 0, 0); {
					/*
					kf_ui_color(ui, KF_RGBA(255, 255, 255, 255));
					kf_ui_rect(ui, KF_IRECT(0, 0, 200, 200));

					kf_ui_color(ui, KF_RGBA(255, 0, 0, 255));
					kf_ui_rect(ui, KF_IRECT(20, 20, 40, 40));

					kf_ui_color(ui, KF_RGBA(255, 0, 0, 255));
					kf_ui_font(ui, &g.font_std32);
					kf_ui_text(ui, debug_text, 0, 0);
					*/

					for (isize i = 0; i < g.opened_tabs.length; i++) {
						if (i == g.currently_opened_tab) {
							kf_ui_color(ui, KF_RGB(128, 128, 128));
						}
						else {
							kf_ui_color(ui, KF_RGB(64, 64, 64));
						}
						kf_ui_rect(ui, KF_IRECT(128 * i, 0, 128, 32));

						kf_EditBox *tab = kf_array_get(g.opened_tabs, i);

						kf_IVector2 txt_pos = kf_center_rect(KF_IRECT(128 * i, 0, 128, 32), KF_IRECT(0, 0, 0, g.font_std32.size)); /* NOTE(EimaMei): Replace width with the text's actual width. */
						kf_ui_color(ui, KF_RGB(255, 255, 255));
						kf_ui_font(ui, &g.font_std32);
						kf_ui_text(ui, kf_string_set_from_cstring(tab->display), 128 * i + 8, txt_pos.y);
					}
				} kf_ui_pop_origin(ui);

				kf_ui_push_origin(ui, 100, 100); {
					if (g.currently_opened_tab != -1) {
						kf_EditBox *tab = kf_array_get(g.opened_tabs, g.currently_opened_tab);
						tab->scroll_offset += e->mousewheel_y;

						kf_ui_color(ui, KF_RGB(255, 255, 255));
						kf_ui_font(ui, &g.font_std32);
						kf_ui_text(ui, tab->content, 0, tab->scroll_offset);
					}
				} kf_ui_pop_origin(ui);
			} kf_ui_end(ui, false);
		}

		/* Render UI */
		isize ui_index, num_ui_elements;
		num_ui_elements = g.ui_context.draw_commands.length;

		/* TODO (maybe): implement running status system where glColor()
		is not re-called if it would set the color to the already-in-use color */

		for (ui_index = 0; ui_index < num_ui_elements; ui_index++) {
			kf_UIDrawCommand *cmd = (kf_UIDrawCommand *)kf_array_get(g.ui_context.draw_commands, ui_index);

			/* Rip out the common (header) info since every cmd uses it anyway */
			kf_Color color = cmd->common.color;

			switch (cmd->common.type) {
			case KF_DRAW_RECT: {
				kf_UIDrawRect *rect_cmd = &cmd->rect;
				kf_UVRect rect_as_uv = kf_screen_to_uv(g.win_bounds, rect_cmd->rect);

				glColor4b(color.r >> 1, color.g >> 1, color.b >> 1, color.a >> 1);
				glRectf(rect_as_uv.x, rect_as_uv.y, rect_as_uv.x2, rect_as_uv.y2);
				break;
			}

			case KF_DRAW_TEXT: {
				kf_UIDrawText *text_cmd = &cmd->text;

				kf_String text = text_cmd->text;
				kf_Font *font = text_cmd->font;
				kf_IVector2 begin = text_cmd->begin;

				{ /* GL state setup*/
					glColor4b(color.r >> 1, color.g >> 1, color.b >> 1, color.a >> 1);
					glEnable(GL_BLEND);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					glEnable(GL_TEXTURE_2D);

					/* this can be replaced with a shader later ig. basically it will
					fabricate the RGB component from the Alpha, and current glColor() */
					glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

					glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
					glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PRIMARY_COLOR);
					glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);

					glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
					glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE);
					glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

					/* Client state setup */
					glEnableClientState(GL_VERTEX_ARRAY);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				}

				isize text_length = text.length;
				KF_ARRAY(rune) runes_to_render = kf_array_make(g.temp_alloc, sizeof(rune), 0, text_length, KF_DEFAULT_GROW);

				kf_decode_utf8_string_to_rune_array(text, &runes_to_render);
				isize num_runes = runes_to_render.length;

				isize rune_index;
				kf_IVector2 text_pos = begin;

				rune *this_rune_ptr, *next_rune_ptr; /* next_rune is the next rune in the string to kern against */
				for (rune_index = 0; rune_index < num_runes; rune_index++) {
					/*
					Text rendering steps:
					1) Grab internal index from rune (this is used to reference other data in the Font struct)
					2) Grab texture id from the Font struct for the rune
					3) Render the texture
					*/
					this_rune_ptr = kf_array_get(runes_to_render, rune_index);

					if (*this_rune_ptr == '\n') {
						text_pos.x = begin.x;
						text_pos.y += font->size;
						continue;
					}

					/* Step 2 */
					kf_GlyphInfo *this_glyph = kf_array_get(font->glyphs, *this_rune_ptr);

					if (*this_rune_ptr == '\t') {
						text_pos.x += this_glyph->width;
						continue;
					}

					/* Step 3a - calculate pixel box for text */
					kf_IRect text_box = KF_IRECT(text_pos.x, text_pos.y, this_glyph->width, this_glyph->height);
					text_box.y += font->ascent + this_glyph->y1;

					/* Step 3b - Draw text */
					kf_UVRect uv = kf_screen_to_uv(g.win_bounds, text_box);
					{ /* GL render */
						glBindTexture(GL_TEXTURE_2D, this_glyph->gl_tex);

						GLfloat Vertices[] = {
							(float)uv.x,  (float)uv.y,  0,
							(float)uv.x2, (float)uv.y,  0,
							(float)uv.x2, (float)uv.y2, 0,
							(float)uv.x,  (float)uv.y2, 0
						};
						GLfloat TexCoord[] = {
							0, 0,
							1, 0,
							1, 1,
							0, 1,
						};
						GLubyte indices[] = {
							0, 1, 2, /* first triangle (bottom left - top left - top right) */
							0, 2, 3 /* second triangle (bottom left - top right - bottom right) */
						};


						glVertexPointer(3, GL_FLOAT, 0, Vertices);
						glTexCoordPointer(2, GL_FLOAT, 0, TexCoord);

						glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
					}

					/* Apply X advance if not the last char */
					if (rune_index + 1 < num_runes) {
						next_rune_ptr = kf_array_get(runes_to_render, rune_index + 1);

						/* Add const spacing */
						f32 scale = font->scale;
						int ax = this_glyph->ax;
						int lsb = this_glyph->lsb;
						text_pos.x += roundf(ax * scale);

						/* Add kerning */
						kf_GlyphInfo *next_glyph_info = kf_array_get(font->glyphs, *next_rune_ptr);
						int kern = stbtt_GetGlyphKernAdvance(&font->stb_font, (int)this_glyph->index, (int)next_glyph_info->index);
						text_pos.x += roundf(kern * scale);
					}
				}

				{ /* Reset GL state */
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
					glDisableClientState(GL_VERTEX_ARRAY);
					glBindTexture(GL_TEXTURE_2D, 0);
					glBindTexture(GL_TEXTURE_2D, 0);
					glDisable(GL_BLEND);
					glDisable(GL_TEXTURE_2D);
				}
				break;
			}
			}
		}

		kf_swap_buffers(g.platform_context);

		kf_free_all(g.temp_alloc); /* clear temp buffer every frame (can grow infinitely if this isn't called periodically) */
		kf_free_all(g.ui_alloc);
	}
	kf_terminate_video(g.platform_context);
	printf("KfIDE has quit\n");

	return 0;
}