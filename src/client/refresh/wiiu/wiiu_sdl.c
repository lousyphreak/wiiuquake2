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
 * SDL backend for the GL3 renderer. Everything that needs to be on the
 * renderer side of thing. Also all glad (or whatever OpenGL loader I
 * end up using) specific things.
 *
 * =======================================================================
 */

#include "header/local.h"
#include <gx2/display.h>
#include <gx2/state.h>
#include <gx2/swap.h>
#include <malloc.h>

#ifdef USE_SDL3
#include <SDL3/SDL.h>
#else
#include <SDL2/SDL.h>
#endif


static qboolean vsyncActive = false;

// --------

static GX2ColorBuffer *tvCb;
static GX2Texture *tvTex;

/*
 * Swaps the buffers and shows the next frame.
 */
void WiiU_EndFrame(void)
{
	if(tvCb == NULL)
	{
		tvCb = WHBGfxGetTVColourBuffer();
		tvTex = memalign(0x100, sizeof(*tvTex));
		memset(tvTex, 0, sizeof(*tvTex));
		memcpy(&tvTex->surface, &tvCb->surface, sizeof(tvTex->surface));
		tvTex->viewNumSlices = 1;
		tvTex->compMap = GX2_COMP_MAP(GX2_SQ_SEL_R, GX2_SQ_SEL_G, GX2_SQ_SEL_B, GX2_SQ_SEL_A);
		GX2InitTextureRegs(tvTex);
	}

	//SDL_GL_SwapWindow(window);

#ifdef USE_FBO
	//WiiU_FBO_ContentDone();

	WHBGfxFinishRenderDRC();
	//WHBGfxBeginRenderTV();
	//WiiU_FBO_Draw();
	WHBGfxFinishRenderTV();
	WHBGfxFinishRender();


#else

	WHBGfxFinishRenderTV();

	if(wiiu_drc->value)
	{
		WHBGfxBeginRenderDRC();
		GX2SetShaderMode(GX2_SHADER_MODE_UNIFORM_BLOCK);

		GX2Invalidate(GX2_INVALIDATE_MODE_COLOR_BUFFER, tvTex->surface.image, tvTex->surface.imageSize);
		GX2Invalidate(GX2_INVALIDATE_MODE_TEXTURE, tvTex->surface.image, tvTex->surface.imageSize);

		{
			WiiU_EnableDepthTest(false);
			WiiU_EnableCullFace(false);
			WiiU_EnableBlend(false);

			uint32_t abSize = sizeof(float) * 16;
			float *verts = (float*)WiiU_ABAlloc(abSize);

			verts[0]  =  1.0f; verts[1]  =  1.0f; verts[2]  = 1.0f; verts[3]  = 0.0f;
			verts[4]  =  1.0f; verts[5]  = -1.0f; verts[6]  = 1.0f; verts[7]  = 1.0f;
			verts[8]  = -1.0f; verts[9]  =  1.0f; verts[10] = 0.0f; verts[11] = 0.0f;
			verts[12] = -1.0f; verts[13] = -1.0f; verts[14] = 0.0f; verts[15] = 1.0f;

			WiiU_UseProgram(&gl3state.siDrcCopy);
			WiiU_Bind(tvTex);

			GX2Invalidate(GX2_INVALIDATE_MODE_CPU_ATTRIBUTE_BUFFER, verts, abSize);
			GX2SetAttribBuffer(0, abSize, 4 * sizeof(float), verts);

			GX2DrawEx(GX2_PRIMITIVE_MODE_TRIANGLE_STRIP, 4, 0, 1);
		}

		WHBGfxFinishRenderDRC();
	}

	//WHBGfxFinishRender();
	GX2SwapScanBuffers();
	GX2Flush();
	//GX2DrawDone();
	GX2SetTVEnable(TRUE);
	GX2SetDRCEnable(TRUE);
#endif
}

/*
 * Returns whether the vsync is enabled.
 */
qboolean WiiU_IsVsyncActive(void)
{
	return vsyncActive;
}

/*
 * Enables or disables the vsync.
 */
void WiiU_SetVsync(void)
{
	// __WIIU_TODO__
}

/*
 * This function returns the flags used at the SDL window
 * creation by GLimp_InitGraphics(). In case of error -1
 * is returned.
 */
int WiiU_PrepareForWindow(void)
{
	// __WIIU_TODO__ curently unused
	return 0;
}

/*
 * Initializes the OpenGL context. Returns true at
 * success and false at failure.
 */
int WiiU_InitContext(void* win)
{
	// __WIIU_TODO__ curently unused
	return true;
}

/*
 * Fills the actual size of the drawable into width and height.
 */
void WiiU_GetDrawableSize(int* width, int* height)
{
	// figure out the TV render mode and size
	switch(GX2GetSystemTVScanMode()) {
	case GX2_TV_SCAN_MODE_480I:
	case GX2_TV_SCAN_MODE_480P:
		*width = 854;
		*height = 480;
		break;
	case GX2_TV_SCAN_MODE_1080I:
	case GX2_TV_SCAN_MODE_1080P:
		*width = 1920;
		*height = 1080;
		break;
	case GX2_TV_SCAN_MODE_720P:
	default:
		*width = 1280;
		*height = 720;
		break;
	}
}

/*
 * Shuts the GL context down.
 */
void WiiU_ShutdownContext()
{
}

/*
 * Returns the SDL major version. Implemented
 * here to not polute gl3_main.c with the SDL
 * headers.
 */
int WiiU_GetSDLVersion()
{
	SDL_version ver;

	SDL_VERSION(&ver);

	return ver.major;
}
