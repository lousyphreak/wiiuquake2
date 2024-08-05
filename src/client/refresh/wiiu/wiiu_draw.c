/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2016-2017 Daniel Gibson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * Drawing of all images that are not textures
 *
 * =======================================================================
 */

#include "header/local.h"
#include <malloc.h>

unsigned d_8to24table[256];

gl3image_t *draw_chars;

//static uint32_t vbo2D = 0, vao2D = 0, vao2Dcolor = 0; // vao2D is for textured rendering, vao2Dcolor for color-only


void
WiiU_Draw_InitLocal(void)
{
	/* load console characters */
	draw_chars = R_FindPic("conchars", (findimage_t)WiiU_FindImage);
	if (!draw_chars)
	{
		ri.Sys_Error(ERR_FATAL, "%s: Couldn't load pics/conchars.pcx",
			__func__);
	}
}

void
WiiU_Draw_ShutdownLocal(void)
{
}



// bind the texture before calling this
static void
drawTexturedRectangle(float x, float y, float w, float h,
                      float sl, float tl, float sh, float th)
{
	//	x,y,w,h,sl,tl,sh,th);
	/*
	 *  x,y+h      x+w,y+h
	 * sl,th--------sh,th
	 *  |             |
	 *  |             |
	 *  |             |
	 * sl,tl--------sh,tl
	 *  x,y        x+w,y
	 */

	uint32_t abSize = sizeof(float) * 16;
	float* ab = WiiU_ABAlloc(abSize);
	ab[0]  = x;   ab[1]  = y+h; ab[2]  = sl; ab[3]  = th;
	ab[4]  = x;   ab[5]  = y;   ab[6]  = sl; ab[7]  = tl;
	ab[8]  = x+w; ab[9]  = y+h; ab[10] = sh; ab[11] = th;
	ab[12] = x+w; ab[13] = y;   ab[14] = sh; ab[15] = tl;

	//DCStoreRange(ab, abSize);
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_ATTRIBUTE_BUFFER, ab, abSize);
	GX2SetAttribBuffer(0, abSize, 4 * sizeof(float), ab);
    GX2DrawEx(GX2_PRIMITIVE_MODE_TRIANGLE_STRIP, 4, 0, 1);
}

static void
drawColoredRectangle(float x, float y, float w, float h)
{
	uint32_t abSize =sizeof(float) * 8;
	float* ab = WiiU_ABAlloc(abSize);
	//      X,           Y
	ab[0] = x;   ab[1] = y+h;
	ab[2] = x;   ab[3] = y;
	ab[4] = x+w; ab[5] = y+h;
	ab[6] = x+w; ab[7] = y;

	//DCStoreRange(ab, abSize);
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_ATTRIBUTE_BUFFER, ab, abSize);
	GX2SetAttribBuffer(0, abSize, 2 * sizeof(float), ab);
    GX2DrawEx(GX2_PRIMITIVE_MODE_TRIANGLE_STRIP, 4, 0, 1);
}

/*
 * Draws one 8*8 graphics character with 0 being transparent.
 * It can be clipped to the top of the screen to allow the console to be
 * smoothly scrolled off.
 */
void
WiiU_Draw_CharScaled(int x, int y, int num, float scale)
{
	int row, col;
	float frow, fcol, size, scaledSize;
	num &= 255;

	if ((num & 127) == 32)
	{
		return; /* space */
	}

	if (y <= -8)
	{
		return; /* totally off screen */
	}

	row = num >> 4;
	col = num & 15;

	frow = row * 0.0625;
	fcol = col * 0.0625;
	size = 0.0625;

	scaledSize = 8*scale;

	// TODO: batchen?

	WiiU_UseProgram(&gl3state.si2D);
	WiiU_Bind(&draw_chars->texture);
	drawTexturedRectangle(x, y, scaledSize, scaledSize, fcol, frow, fcol+size, frow+size);
}

gl3image_t *
WiiU_Draw_FindPic(char *name)
{
	return R_FindPic(name, (findimage_t)WiiU_FindImage);
}

void
WiiU_Draw_GetPicSize(int *w, int *h, char *pic)
{
	gl3image_t *gl;

	gl = R_FindPic(pic, (findimage_t)WiiU_FindImage);

	if (!gl)
	{
		*w = *h = -1;
		return;
	}

	*w = gl->width;
	*h = gl->height;
}

void
WiiU_Draw_StretchPic(int x, int y, int w, int h, char *pic)
{
	gl3image_t *gl = R_FindPic(pic, (findimage_t)WiiU_FindImage);

	if (!gl)
	{
		R_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	WiiU_UseProgram(&gl3state.si2D);
	WiiU_Bind(&gl->texture);

	drawTexturedRectangle(x, y, w, h, gl->sl, gl->tl, gl->sh, gl->th);
}

void
WiiU_Draw_PicScaled(int x, int y, char *pic, float factor)
{
	gl3image_t *gl = R_FindPic(pic, (findimage_t)WiiU_FindImage);
	if (!gl)
	{
		R_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	WiiU_UseProgram(&gl3state.si2D);
	WiiU_Bind(&gl->texture);

	drawTexturedRectangle(x, y, gl->width*factor, gl->height*factor, gl->sl, gl->tl, gl->sh, gl->th);
}


/*
 * This repeats a 64*64 tile graphic to fill
 * the screen around a sized down
 * refresh window.
 */
void
WiiU_Draw_TileClear(int x, int y, int w, int h, char *pic)
{
	gl3image_t *image = R_FindPic(pic, (findimage_t)WiiU_FindImage);
	if (!image)
	{
		R_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	WiiU_UseProgram(&gl3state.si2D);
	WiiU_Bind(&image->texture);

	drawTexturedRectangle(x, y, w, h, x/64.0f, y/64.0f, (x+w)/64.0f, (y+h)/64.0f);
}

void
WiiU_DrawFrameBufferObject(int x, int y, int w, int h, GX2Texture *fboTexture, const float v_blend[4])
{
	qboolean underwater = true;//(gl3_newrefdef.rdflags & RDF_UNDERWATER) != 0;
	gl3ShaderInfo_t* shader = underwater ? &gl3state.si2DpostProcessWater
	                                     : &gl3state.si2DpostProcess;
	WiiU_UseProgram(shader);
	WiiU_Bind(fboTexture);
	WiiU_UpdateUBOPostProcess(gl3_newrefdef.time, v_blend);

	drawTexturedRectangle(x, y, w, h, 0, 0, 1, 1);
}

/*
 * Fills a box of pixels with a single color
 */
void
WiiU_Draw_Fill(int x, int y, int w, int h, int c)
{
	union
	{
		unsigned c;
		byte v[4];
	} color;
	int i;

	if ((unsigned)c > 255)
	{
		ri.Sys_Error(ERR_FATAL, "Draw_Fill: bad color");
	}

	color.c = d_8to24table[c];

	for(i=0; i<3; ++i)
	{
		gl3state.uniCommonData.color.Elements[i] = color.v[i] * (1.0f/255.0f);
	}
	gl3state.uniCommonData.color.A = 1.0f;

	WiiU_UpdateUBOCommon();

	WiiU_UseProgram(&gl3state.si2Dcolor);

	drawColoredRectangle(x,y,w,h);
}

// in GL1 this is called R_Flash() (which just calls R_PolyBlend())
// now implemented in 2D mode and called after SetGL2D() because
// it's pretty similar to WiiU_Draw_FadeScreen()
void
WiiU_Draw_Flash(const float color[4], float x, float y, float w, float h)
{
	if (gl_polyblend->value == 0)
	{
		return;
	}

	int i=0;

	WiiU_EnableBlend(true);

	for(i=0; i<4; ++i)  gl3state.uniCommonData.color.Elements[i] = color[i];

	WiiU_UpdateUBOCommon();

	WiiU_UseProgram(&gl3state.si2Dcolor);
	drawColoredRectangle(x,y,w,h);

	WiiU_EnableBlend(false);
}

void
WiiU_Draw_FadeScreen(void)
{
	float color[4] = {0, 0, 0, 0.6f};
	WiiU_Draw_Flash(color, 0, 0, vid.width, vid.height);
}


// Used for video rendering
GX2Texture srTex;

void
WiiU_Draw_StretchRaw(int x, int y, int w, int h, int cols, int rows, const byte *data, int bits)
{
	if(srTex.surface.pitch == 0 || srTex.surface.width != cols)
	{
		srTex.surface.format = GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8;
		srTex.surface.width = cols;
		srTex.surface.height = rows;
		srTex.surface.depth = 1;
		srTex.surface.mipLevels = 1;
		srTex.surface.dim = GX2_SURFACE_DIM_TEXTURE_2D;
		srTex.surface.tileMode = GX2_TILE_MODE_LINEAR_ALIGNED;
		srTex.surface.use = GX2_SURFACE_USE_TEXTURE;
		srTex.viewNumSlices = 1;
		srTex.compMap = GX2_COMP_MAP(GX2_SQ_SEL_R, GX2_SQ_SEL_G, GX2_SQ_SEL_B, GX2_SQ_SEL_A);

		BOOL res;
		res = GX2RCreateSurface(&srTex.surface,
						(GX2RResourceFlags)(GX2R_RESOURCE_BIND_TEXTURE |
											GX2R_RESOURCE_USAGE_CPU_WRITE |
											GX2R_RESOURCE_USAGE_GPU_READ));
		GX2InitTextureRegs(&srTex);

		if(res != true)
		{
			WHBLogWritef("Failed to create surface\n");
		}
	}

	int i, j;

	uint32_t *texData = (uint32_t *)GX2RLockSurfaceEx(&srTex.surface, 0, GX2R_RESOURCE_BIND_NONE);
	if (bits == 32)
	{
		//texData = data;
	}
	else
	{
		for(i=0; i<rows; ++i)
		{
			int srcRowOffset = i * cols;
			int dstRowOffset = i * srTex.surface.pitch;
			for(j=0; j<cols; ++j)
			{
				byte palIdx = data[srcRowOffset+j];
				uint32_t src = gl3_rawpalette[palIdx];
				texData[dstRowOffset + j] = src;
			}
		}
	}
	GX2RUnlockSurfaceEx(&srTex.surface, 0, GX2R_RESOURCE_BIND_NONE);
	//GX2SetPixelTexture(&srTex, 0);
	WiiU_UseProgram(&gl3state.si2D);
	WiiU_UpdateUBOCommon();
	WiiU_UpdateUBO2D();
	WiiU_Bind(&srTex);
	GX2SetPixelSampler(&gl3state.samplerVideo, 0);

	drawTexturedRectangle(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f);
}
