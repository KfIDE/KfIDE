#define GB_IMPLEMENTATION
#include "gb.h"
#include "kf.h"

/* C include */
#include "translation.c"
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>


typedef struct {
	gbAllocator heap_alloc, global_alloc, temp_alloc;
	gbArena temp_backing, global_backing;
	PlatformSpecificContext platform_context;

	IRect win_bounds;
	EventState event_state;
	TranslationRecord translation_record;
} GlobalVars;


static GlobalVars g;

unsigned char ttf_buffer[1<<20];
unsigned char temp_bitmap[512*512];

stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs
GLuint ftex;


bool kf_file_exists(gbString path)
{
	struct stat buffer;
  	return (stat(path, &buffer) == 0);
}

bool find_font(gbString font)
{
	bool font_exists;
	isize count = 0;
	gbString new;

	while (true) {
		font_exists = kf_file_exists(font);
		printf("%i %s", font_exists, font);
		if (!font_exists) {
			switch (count) {
				case 0: {
					new = gb_string_make(g.temp_alloc, "/System/Library/Fonts/");
					gb_string_append(new, font);
					printf("%s", new);
					break;
				}
				case 1: {
					//original_name = gb_string_make(gb_alloc() );
					//font = gb_string_append("/Users/<USERNAME>/Library", font);
					break;
				}
				case 2: return false;
			}
			count++;
		}

		return true;
	}
	return false;
}


void my_stbtt_initfont(const gbString font)
{
	bool result = find_font(font);

	if (!result) {
		printf("Error: '%s' doesn't exist", font);
	}

   fread(ttf_buffer, 1, 1<<20, fopen("/System/Library/Fonts/geneva.ttf", "rb"));
   stbtt_BakeFontBitmap(ttf_buffer,0, 32.0, temp_bitmap, 512,512, 32,96, cdata); // no guarantee this fits!
   // can free ttf_buffer at this point
   glGenTextures(1, &ftex);
   glBindTexture(GL_TEXTURE_2D, ftex);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 512,512, 0, GL_ALPHA, GL_UNSIGNED_BYTE, temp_bitmap);
   // can free temp_bitmap at this point
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void my_stbtt_print(float x, float y, char *text)
{
   // assume orthographic projection with units = screen pixels, origin at top left
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, ftex);
   glPushMatrix(); // text matrix
            // change the x/y world cords to real cords
    glOrtho(0, 512, 512, 0, 0, 2);
   glBegin(GL_QUADS);
   while (*text) {
      if (*text >= 32 && *text < 128) {
         stbtt_aligned_quad q;
         stbtt_GetBakedQuad(cdata, 512,512, *text-32, &x,&y,&q,1);//1=opengl & d3d10+,0=d3d9
         glTexCoord2f(q.s0,q.t0); glVertex2f(q.x0,q.y0);
         glTexCoord2f(q.s1,q.t0); glVertex2f(q.x1,q.y0);
         glTexCoord2f(q.s1,q.t1); glVertex2f(q.x1,q.y1);
         glTexCoord2f(q.s0,q.t1); glVertex2f(q.x0,q.y1);
      }
      ++text;
   }
   glEnd();
   glPopMatrix();
}


/* To test if OpenGL is properly initialized. */
void opengl_test()
{
	glClearColor(0.1, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); /* NOTE(EimaMei): Not sure if this should be a function on its own, so I'll just leave it here (Based on https://www.khronos.org/opengl/wiki/Common_Mistakes#Swap_Buffers). */

	glBegin(GL_TRIANGLES);
		glColor3f(1, 0, 0); glVertex2f(-1.0f, -1.0f);
		glColor3f(0, 1, 0); glVertex2f( 1.0f, -1.0f);
		glColor3f(0, 0, 1); glVertex2f( 0.0f,  1.0f);
	glEnd();

	my_stbtt_print(256, 256, "BEEP REPAIR");

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
	my_stbtt_initfont("moyai.png");

	/* Translations init */
	static const char *test_str = "en;;nl\nTEST;;TEST";
	kf_load_translations_from_csv_string(g.global_alloc, &g.translation_record, test_str, strlen(test_str));

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