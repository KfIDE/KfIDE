#define GB_IMPLEMENTATION
#include "gb.h"
#include "kf.h"

/* C include */
#include "translation.c"
#include "math.c"
#include "ui.c"
#include "time.c"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


typedef struct {
	gbAllocator heap_alloc, global_alloc, temp_alloc, ui_alloc;
	gbArena temp_backing, global_backing, ui_backing;

	kf_PlatformSpecificContext platform_context;

	/* DEBUG */
	kf_Time debug_timer;

	kf_IRect win_bounds;
	kf_EventState event_state;
	kf_TranslationRecord translation_record;
	kf_UIContext ui_context;

	kf_Font font_std32;
} GlobalVars;
static GlobalVars g;




void kf_load_ttf_font(kf_Font *out, gbAllocator alloc, gbAllocator temp_alloc, gbString path, gbString glyphset, isize pt_size)
{
	if (!gb_file_exists(path)) {
		printf("Error: '%s' doesn't exist", path);
	}

	gbFileContents contents = gb_file_read_contents(alloc, true, path);
	if (!(contents.data != NULL && contents.size > 0)) {
		GB_PANIC("kf_load_ttf_font(): invalid file path given.");
	}

	/* input string is UTF8 formatted; this writes the Rune[] it represents into the font struct */
	isize glyphset_length = gb_string_length(glyphset);
	gb_array_init_reserve(out->runes, alloc, glyphset_length);
	isize i;
	for (i = 0; i < glyphset_length;) {
		if (glyphset[i] == 0) { break; }

		Rune r = GB_RUNE_INVALID;
		isize original_i = i;
		i += gb_utf8_decode(&glyphset[i], (glyphset_length - i), &r);
		GB_ASSERT(r != GB_RUNE_INVALID);
		gb_array_append(out->runes, r);
	}

	/* now we can init our arrays since we know how many runes we'll be needing */
	isize num_runes = gb_array_count(out->runes);
	gb_array_init_reserve(out->glyphs, alloc, num_runes);

	stbtt_InitFont(&out->stb_font, contents.data, 0);
	f32 scale = stbtt_ScaleForPixelHeight(&out->stb_font, pt_size);

	/* idk if we actually need to do any of the stuff in this block or not */
	{
		int ascent, descent, line_gap;
	    stbtt_GetFontVMetrics(&out->stb_font, &ascent, &descent, &line_gap);

	    ascent = roundf(ascent * scale); /* from <stdlib.h> apparently */
	    descent = roundf(descent * scale);

	    out->ascent = (usize)ascent;
	    out->descent = (usize)descent;
	    out->line_gap = (usize)line_gap;
	}

	isize bitmap_w, bitmap_h;
	bitmap_w = (isize)(1.5f * (f32)(pt_size + 1)); /* 50% more than pt_size */
	bitmap_h = (isize)(1.5f * (f32)(pt_size + 1));

	GB_ASSERT(pt_size < bitmap_h);

	/* allocate a bitmap for the glyph */
	isize bitmap_size = bitmap_w * bitmap_h;
	u8 *bitmap = (u8 *)gb_alloc(temp_alloc, bitmap_size * sizeof(u8) * 3); /* RGB */
	gb_zero_size(bitmap, bitmap_size);

	/*
	Go through each rune from the list and do a couple of things:
	1) Create a corresponding glyph index entry (faster than using codepoints the entire time)
	2) Grab metrics
	3) Render to bitmap
	4) Generate gl textures from bitmap; obviously store the tex IDs into the font too
	*/

	kf_write_current_time(&g.debug_timer); /* curious how long this loop takes, so let's time it */
	int x = 0;
	for (i = 0; i < num_runes; i++) {
		/* Step 1 */
		gb_array_append(out->glyphs, (kf_GlyphInfo){0}); /* just to increase the count */
		kf_GlyphInfo *this_glyph = &out->glyphs[i];

		int glyph_index = stbtt_FindGlyphIndex(&out->stb_font, (int)out->runes[i]);
		this_glyph->index = glyph_index;

		int ax, lsb;
		stbtt_GetGlyphHMetrics(&out->stb_font, glyph_index, &ax, &lsb);

		/* Step 2 */
		int c_x1, c_y1, c_x2, c_y2;
		stbtt_GetGlyphBitmapBox(&out->stb_font, glyph_index, scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);
		/*printf("BM BOX: %d %d %d %d\n", c_x1, c_y1, c_x2, c_y2);*/

		int glyph_w = c_x2 - c_x1;
		int glyph_h = c_y2 - c_y1;

		this_glyph->width = (isize)glyph_w;
		this_glyph->height = (isize)glyph_h;

		/* Step 3 */
		int byte_offset = roundf(lsb * scale) + bitmap_w; /* idk how this works */
		u8 *final_bm = bitmap + byte_offset; /* first glyph pixel in the bitmap (not always 0,0 i guesss) */

		/* this renders the ONE-CHANNEL glyph into the bitmap
		but since we can't tint a 1-channel (GL_RED) tex (i think) we're gonna have to
		expand it to RGB; this is why we allocated 3 * sizeof(u8) earlier */
		stbtt_MakeGlyphBitmap(&out->stb_font, final_bm, glyph_w, glyph_h, bitmap_w, scale, scale, glyph_index);

		/* for debug, we could make it write out .png images here using stb_image_write.h
		in order to inspect what the glyph img actually looks like, or just render it with gl */

		/* expand to rgb bitmap */
		{
			/* we could make this loop smarter by making it calcluate the src/dest rects
			and only copying pixels actually in the glyph rect and not the entire image
			but whatever. dumb loop may be slightly faster anyway. */
			isize j, this;
			for (j = 0; j < bitmap_w * bitmap_h; j++) { /* should be ~60K iterations per glyph */
				this = final_bm[j];
				final_bm[j*3] = this;
				final_bm[j*3 + 1] = this;
				final_bm[j*3 + 2] = this;
			}
		}

		/* Step 4 */
		GLuint gl_tex;
		glGenTextures(1, &gl_tex);
		glBindTexture(GL_TEXTURE_2D, gl_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bitmap_w, bitmap_h, 0, GL_RGB, GL_UNSIGNED_BYTE, bitmap);
		glGenerateMipmap(GL_TEXTURE_2D);

		this_glyph->gl_tex = gl_tex;

		/* probably unnecessary */
		gb_zero_size(bitmap, bitmap_size);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	kf_print_time_since(&g.debug_timer);
}

static void _draw_tex_rect(GLuint texture, kf_UVRect uv, bool blend_flag)
{
	
}



int main(int argc, char **argv)
{
	/* Allocators init */
	#define GLOBAL_SIZE		(isize)gb_megabytes(2)
	#define TEMP_SIZE		(isize)gb_megabytes(2)
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
	glEnable(GL_TEXTURE_2D);*/

	/* Translations init */
	static const char *test_str = "en;;nl\nTEST;;TEST";
	kf_load_translations_from_csv_string(g.global_alloc, &g.translation_record, (u8 *)test_str, strlen(test_str));
	kf_write_window_size(g.platform_context, &g.win_bounds.w, &g.win_bounds.h);

	/* Font init */
	{
		kf_load_ttf_font(&g.font_std32, g.global_alloc, g.temp_alloc, gb_string_make(g.global_alloc, "/home/ps4star/Documents/eurostile.ttf"), gb_string_make(g.global_alloc, "qwertyuiopasdfghjklzxcvbnm "), 64);
	}
	while (true) {
		kf_analyze_events(g.platform_context, &g.event_state, true);

		kf_EventState *e = &g.event_state;
		if (e->exited) {
			break;
		}

		if (e->was_window_resized) {
			g.win_bounds = (kf_IRect){ 0, 0, e->window_resize_width, e->window_resize_height };
		}

		/* Gfx start */
		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glViewport(0, 0, g.win_bounds.w, g.win_bounds.h);

		/* UI */
		{
			kf_UIContext *ui = &g.ui_context;

			gbString debug_text = gb_string_make(g.ui_alloc, "hmm suspicious");

			/* UI BEGIN */
			kf_ui_begin(ui, g.ui_alloc, 196, &g.event_state); {
				kf_ui_push_origin(ui, 10, 0); {
					kf_ui_color(ui, KF_RGBA(255, 255, 255, 255));
					kf_ui_rect(ui, (kf_IRect){ 0, 0, 200, 200 });

					kf_ui_color(ui, KF_RGBA(255, 0, 0, 255));
					kf_ui_rect(ui, (kf_IRect){ 20, 20, 40, 40 });

					kf_ui_color(ui, KF_RGBA(255, 255, 255, 255));
					kf_ui_font(ui, &g.font_std32);
					kf_ui_text(ui, debug_text, 0, 0);
				} kf_ui_pop_origin(ui);
			} kf_ui_end(ui, false);
		}

		/* Render UI */
		isize ui_index, num_ui_elements;
		num_ui_elements = gb_array_count(g.ui_context.draw_commands);

		/* TODO (maybe): implement running status system where glColor()
		is not re-called if it would set the color to the already-in-use color */

		for (ui_index = 0; ui_index < num_ui_elements; ui_index++) {
			kf_UIDrawCommand *cmd = &g.ui_context.draw_commands[ui_index];

			/* Rip out the common (header) info since every cmd uses it anyway */
			kf_Color color = cmd->common.color;

			switch (cmd->common.type) {
			case KF_DRAW_RECT: {
				kf_UIDrawRect *rect_cmd = &cmd->rect;
				kf_UVRect rect_as_uv = kf_screen_to_uv(g.win_bounds, rect_cmd->rect);

				/*glColor4b(color.r >> 1, color.g >> 1, color.b >> 1, color.a >> 1);
				glRectf(rect_as_uv.x, rect_as_uv.y, rect_as_uv.x2, rect_as_uv.y2);*/
			}
				break;

			case KF_DRAW_TEXT: {
				kf_UIDrawText *text_cmd = &cmd->text;
				kf_UVRect rect_as_uv = kf_screen_to_uv(g.win_bounds, (kf_IRect){ text_cmd->begin.x, text_cmd->begin.y, 0, 0 });

				gbString text = text_cmd->text;
				kf_Font *font = text_cmd->font;
				kf_IVector2 begin = text_cmd->begin;

				glColor4b(color.r >> 1, color.g >> 1, color.b >> 1, color.a >> 1);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glEnable(GL_TEXTURE_2D);

				/*glEnable(GL_TEXTURE_2D);
				glPushMatrix(); // text matrix
				// change the x/y world cords to real cords
				glOrtho(0, 512, 512, 0, 0, 2);
				glBegin(GL_QUADS);*/

				isize text_length = gb_string_length(text);
				isize this_char;
				f32 debug_x = -0.75f;
				f32 debug_y = 0.75f;
				kf_IVector2 text_pos = begin;
				for (this_char = 0; this_char < text_length;) {
					/*
					Text rendering steps:
					1) Decode rune out of string
					2) Grab internal index from Rune (this is used to reference other data in the Font struct)
					3) Grab texture id from the Font struct for the Rune
					4) Render the texture
					*/

					/* Step 1 */
					Rune r;
					isize char_start = this_char; /* keep 'old' position just in case we need it */
					this_char += gb_utf8_decode(&text[this_char], (text_length - this_char), &r);

					/* Step 2 - look up index by Rune */
					isize internal_index = -1;
					int glyph_index;
					{
						isize i;
						for (i = 0; i < gb_array_count(font->runes); i++) {
							if (font->runes[i] == r) {
								internal_index = i;
								break;
							}
						}
						GB_ASSERT(internal_index > -1);
					}

					/* Step 3 */
					kf_GlyphInfo *glyph_info = &font->glyphs[internal_index];

					/* Step 4a - calculate pixel box for text */
					text_pos.x += 50;

					/* Step 4b - Draw text */
					kf_UVRect uv = kf_screen_to_uv(g.win_bounds, (kf_IRect){ text_pos.x, text_pos.y, glyph_info->width, glyph_info->height });
					/*printf("GLYPH BOX: %f:%f:%f:%f ::: %d:%d:%d:%d ::: %d:%d:%d:%d\n", uv.x, uv.y, uv.x2, uv.y2, text_pos.x, text_pos.y, glyph_info->width, glyph_info->height, g.win_bounds.x, g.win_bounds.y, g.win_bounds.w, g.win_bounds.h);*/
					{
						glBindTexture(GL_TEXTURE_2D, glyph_info->gl_tex);

						GLfloat Vertices[] = {(float)uv.x, (float)uv.y, 0,
											(float)uv.x2, (float)uv.y, 0,
											(float)uv.x2, (float)uv.y2, 0,
											(float)uv.x, (float)uv.y2, 0};
						GLfloat TexCoord[] = {0, 0,
											1, 0,
											1, 1,
											0, 1,
						};
						GLubyte indices[] = {0,1,2, // first triangle (bottom left - top left - top right)
											0,2,3}; // second triangle (bottom left - top right - bottom right)

						glEnableClientState(GL_VERTEX_ARRAY);
						glVertexPointer(3, GL_FLOAT, 0, Vertices);

						glEnableClientState(GL_TEXTURE_COORD_ARRAY);
						glTexCoordPointer(2, GL_FLOAT, 0, TexCoord);

						glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);

						glDisableClientState(GL_TEXTURE_COORD_ARRAY);
						glDisableClientState(GL_VERTEX_ARRAY);
						glBindTexture(GL_TEXTURE_2D, 0);
					}
				}

				/*glEnd();
				glPopMatrix();*/
			}
				break;
			}
		}

		glFlush();
		glFinish();
		kf_swap_buffers(g.platform_context);

		gb_free_all(g.temp_alloc); /* clear temp buffer every frame (can grow infinitely if this isn't called periodically) */
		gb_free_all(g.ui_alloc);
	}
	kf_terminate_video(g.platform_context);
	return 0;
}