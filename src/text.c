#include "gb.h"
#include "kf.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

u8 ttf_buffer[1 << 20];
u8 temp_bitmap[512 * 512];


stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs

/* NOTE(EimaMei): replace the 512x512 parts with `win_bounds` later. */
void draw_text(GLuint font_id, gbString text, isize x, isize y, Color color)
{
   // assume orthographic projection with units = screen pixels, origin at top left
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, font_id);
	float stb_x = x * 1.0f;
	float stb_y = y * 1.0f;

	glPushMatrix();
    	glOrtho(0, 512, 512, 0, 0, 2);

		ColorStruct *c = &color;
		glColor4f(c->r / 255.0f, c->g / 255.0f, c->b / 255.0f, c->a / 255.0f);

   		glBegin(GL_QUADS);
			while (*text) {
				if (*text >= 32 && *text < 128) {
					stbtt_aligned_quad q;

					stbtt_GetBakedQuad(cdata, 512, 512, *text - 32, &stb_x, &stb_y, &q, 1);//1=opengl & d3d10+,0=d3d9
					glTexCoord2f(q.s0,q.t0); glVertex2f(q.x0,q.y0);
					glTexCoord2f(q.s1,q.t0); glVertex2f(q.x1,q.y0);
					glTexCoord2f(q.s1,q.t1); glVertex2f(q.x1,q.y1);
					glTexCoord2f(q.s0,q.t1); glVertex2f(q.x0,q.y1);
				}
				text++;
			}
	   glEnd();

   glPopMatrix();
}

/* NOTE(EimaMei): replace the 512x512 parts with `win_bounds` later. */
GLuint init_font(gbString font, isize size)
{
	font = find_font(font);

	if (font == NULL) {
		printf("Error: Font '%s' doesn't exist", font);
	}

	FILE *file = fopen(font, "rb");
	GLuint texture;

	fread(ttf_buffer, 1, sizeof(ttf_buffer), file);
	stbtt_BakeFontBitmap(ttf_buffer, 0, size, temp_bitmap, 512, 512, 32, 96, cdata); // no guarantee this fits!

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 512, 512, 0, GL_ALPHA, GL_UNSIGNED_BYTE, temp_bitmap);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	fclose(file);

	return texture;
}