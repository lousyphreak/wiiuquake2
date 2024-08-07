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
 * Local header for the OpenGL3 refresher.
 *
 * =======================================================================
 */


#ifndef SRC_CLIENT_REFRESH_WiiU_HEADER_LOCAL_H_
#define SRC_CLIENT_REFRESH_WiiU_HEADER_LOCAL_H_

//#define USE_FBO

#include <coreinit/cache.h>
#include <whb/log.h>
#include <whb/gfx.h>
#include <whb/proc.h>
#include <gx2/draw.h>
#include <gx2/clear.h>
#include <gx2/mem.h>
#include <gx2/registers.h>
#include <gx2/texture.h>
#include <gx2/utils.h>
#include <gx2r/buffer.h>
#include <gx2r/draw.h>
#include <gx2r/surface.h>

#include "../../ref_shared.h"
#include "HandmadeMath.h"

#if 0 // only use this for development ..
#define STUB_ONCE(msg) do { \
		static int show=1; \
		if(show) { \
			show = 0; \
			R_Printf(PRINT_ALL, "STUB: %s() %s\n", __FUNCTION__, msg); \
		} \
	} while(0);
#else // .. so make this a no-op in released code
#define STUB_ONCE(msg)
#endif

// attribute locations for vertex shaders
enum {
	WiiU_ATTRIB_POSITION   = 0,
	WiiU_ATTRIB_TEXCOORD   = 1, // for normal texture
	WiiU_ATTRIB_LMTEXCOORD = 2, // for lightmap
	WiiU_ATTRIB_COLOR      = 3, // per-vertex color
	WiiU_ATTRIB_NORMAL     = 4, // vertex normal
	WiiU_ATTRIB_LIGHTFLAGS = 5  // uint, each set bit means "dyn light i affects this surface"
};

extern unsigned gl3_rawpalette[256];
extern unsigned d_8to24table[256];

typedef struct
{
	const char *renderer_string;
	const char *vendor_string;
	const char *version_string;
	const char *glsl_version_string;

	int major_version;
	int minor_version;

	// ----

	qboolean anisotropic; // is GL_EXT_texture_filter_anisotropic supported?
	qboolean debug_output; // is GL_ARB_debug_output supported?
	qboolean stencil; // Do we have a stencil buffer?

	// ----

	float max_anisotropy;
} gl3config_t;

typedef struct
{
	GX2Texture colourTexture;
	GX2ColorBuffer colourBuffer;
	GX2DepthBuffer depth;
	GX2ContextState* ctx;
} framebuffer_t;

typedef struct
{
	WHBGfxShaderGroup shaderGroup;
	const char* name;
} gl3ShaderInfo_t;

typedef struct
{
	float gamma;
	float intensity;
	float intensity2D; // for HUD, menus etc

		// entries of std140 UBOs are aligned to multiples of their own size
		// so we'll need to pad accordingly for following vec4
		float _padding;

	hmm_vec4 color;
} gl3UniCommon_t;

typedef struct
{
	hmm_mat4 transMat4;
} gl3Uni2D_t;

typedef struct
{
	hmm_mat4 transProjViewMat4; // gl3state.projMat3D * gl3state.viewMat3D - so we don't have to do this in the shader
	hmm_mat4 transModelMat4;

	float scroll; // for SURF_FLOWING
	float time; // for warping surfaces like water & possibly other things
	float alpha; // for translucent surfaces (water, glass, ..)
	float overbrightbits; // gl3_overbrightbits, applied to lightmaps (and elsewhere to models)
	float particleFadeFactor; // gl3_particle_fade_factor, higher => less fading out towards edges

	float lightScaleForTurb; // surfaces with SURF_DRAWTURB (water, lava) don't have lightmaps, use this instead
		float _padding[2]; // again, some padding to ensure this has right size
} gl3Uni3D_t;

extern const hmm_mat4 gl3_identityMat4;

typedef struct
{
	vec3_t origin;
	float _padding;
	vec3_t color;
	float intensity;
} gl3UniDynLight;

typedef struct
{
	gl3UniDynLight dynLights[MAX_DLIGHTS];
} gl3UniLights_t;

enum {
	// width and height used to be 128, so now we should be able to get the same lightmap data
	// that used 32 lightmaps before into one, so 4 lightmaps should be enough
	BLOCK_WIDTH = 1024,
	BLOCK_HEIGHT = 512,
	LIGHTMAP_BYTES = 4,
	MAX_LIGHTMAPS = 4,
	MAX_LIGHTMAPS_PER_SURFACE = MAXLIGHTMAPS // 4
};

typedef struct
{
	// TODO: what of this do we need?
	qboolean fullscreen;

	int prev_mode;
	GX2Texture *currenttexture; // bound to GL_TEXTURE0
	int currentlightmap; // lightmap_textureIDs[currentlightmap] bound to GL_TEXTURE1
#ifdef __WIIU_TODO__

	// FBO for postprocess effects (like under-water-warping)
	uint32_t ppFBO;
	uint32_t ppFBtex; // ppFBO's texture for color buffer
	int ppFBtexWidth, ppFBtexHeight;
	uint32_t ppFBrbo; // ppFBO's renderbuffer object for depth and stencil buffer
	qboolean ppFBObound; // is it currently bound (rendered to)?
#endif

	gl3ShaderInfo_t *currentShaderProgram;

	// NOTE: make sure si2D is always the first shaderInfo (or adapt WiiU_ShutdownShaders())
	gl3ShaderInfo_t si2D;      // shader for rendering 2D with textures
	gl3ShaderInfo_t si2Dcolor; // shader for rendering 2D with flat colors
	gl3ShaderInfo_t si2DpostProcess; // shader to render postprocess FBO, when *not* underwater
	gl3ShaderInfo_t si2DpostProcessWater; // shader to apply water-warp postprocess effect

	// lightmap shader separated by number of lights
	gl3ShaderInfo_t si3Dlm;        // a regular opaque face (e.g. from brush) with lightmap
	gl3ShaderInfo_t si3Dlm0;        // a regular opaque face (e.g. from brush) with lightmap
	gl3ShaderInfo_t si3Dlm1;        // a regular opaque face (e.g. from brush) with lightmap
	gl3ShaderInfo_t si3Dlm2;        // a regular opaque face (e.g. from brush) with lightmap
	gl3ShaderInfo_t si3Dlm4;        // a regular opaque face (e.g. from brush) with lightmap
	gl3ShaderInfo_t si3Dlm8;        // a regular opaque face (e.g. from brush) with lightmap
	gl3ShaderInfo_t si3Dlm16;        // a regular opaque face (e.g. from brush) with lightmap

	// TODO: lm-only variants for gl_lightmap 1
	gl3ShaderInfo_t si3Dtrans;     // transparent is always w/o lightmap
	gl3ShaderInfo_t si3DcolorOnly; // used for beams - no lightmaps
	gl3ShaderInfo_t si3Dturb;      // for water etc - always without lightmap
	gl3ShaderInfo_t si3DlmFlow;    // for flowing/scrolling things with lightmap (conveyor, ..?)
	gl3ShaderInfo_t si3DlmFlow0;    // for flowing/scrolling things with lightmap (conveyor, ..?)
	gl3ShaderInfo_t si3DlmFlow1;    // for flowing/scrolling things with lightmap (conveyor, ..?)
	gl3ShaderInfo_t si3DlmFlow2;    // for flowing/scrolling things with lightmap (conveyor, ..?)
	gl3ShaderInfo_t si3DlmFlow4;    // for flowing/scrolling things with lightmap (conveyor, ..?)
	gl3ShaderInfo_t si3DlmFlow8;    // for flowing/scrolling things with lightmap (conveyor, ..?)
	gl3ShaderInfo_t si3DlmFlow16;    // for flowing/scrolling things with lightmap (conveyor, ..?)
	gl3ShaderInfo_t si3DtransFlow; // for transparent flowing/scrolling things (=> no lightmap)
	gl3ShaderInfo_t si3Dsky;       // guess what..
	gl3ShaderInfo_t si3Dsprite;    // for sprites
	gl3ShaderInfo_t si3DspriteAlpha; // for sprites with alpha-testing

	GX2Sampler samplerVideo;
	GX2Sampler sampler2D;
	GX2Sampler sampler2DUnfiltered;
	GX2Sampler sampler3D;
	GX2Sampler samplerLightmaps;

	gl3ShaderInfo_t si3Dalias;      // for models
	gl3ShaderInfo_t si3DaliasColor; // for models w/ flat colors

	// NOTE: make sure siParticle is always the last shaderInfo (or adapt WiiU_ShutdownShaders())
	gl3ShaderInfo_t siParticle; // for particles. surprising, right?
	gl3ShaderInfo_t siDrcCopy; // copy to drc

	// UBOs and their data
	gl3UniCommon_t uniCommonData;
	gl3Uni2D_t uni2DData;
	gl3Uni3D_t uni3DData;
	gl3UniLights_t uniLightsData;

	hmm_mat4 projMat3D;
	hmm_mat4 viewMat3D;

	// where everything gets drawn to so we can them copy it to TV&DRC
	framebuffer_t mainFBO;
	// where the world gets drawn to for underwater post effects
	framebuffer_t worldFBO;
	framebuffer_t *activeFBO;
} gl3state_t;

extern gl3config_t gl3config;
extern gl3state_t gl3state;

extern viddef_t vid;

extern refdef_t gl3_newrefdef;

extern int gl3_visframecount; /* bumped when going to a new PVS */
extern int gl3_framecount; /* used for dlight push checking */

extern int gl3_viewcluster, gl3_viewcluster2, gl3_oldviewcluster, gl3_oldviewcluster2;

extern int c_brush_polys, c_alias_polys;

/* NOTE: struct image_s* is what re.RegisterSkin() etc return so no gl3image_s!
 *       (I think the client only passes the pointer around and doesn't know the
 *        definition of this struct, so this being different from struct image_s
 *        in ref_gl should be ok)
 */
typedef struct image_s
{
	char name[MAX_QPATH];               /* game path, including extension */
	imagetype_t type;
	int width, height;                  /* source image */
	//int upload_width, upload_height;    /* after power of two and picmip */
	int registration_sequence;          /* 0 = free */
	struct msurface_s *texturechain;    /* for sort-by-texture world drawing */
	GX2Texture texture;
	float sl, tl, sh, th;               /* 0,0 - 1,1 unless part of the scrap */
	// qboolean scrap; // currently unused
	qboolean has_alpha;
	qboolean is_lava; // DG: added for lava brightness hack

} gl3image_t;

enum {MAX_GL3TEXTURES = 1024};

// include this down here so it can use gl3image_t
#include "model.h"

typedef struct
{
	int current_lightmap_texture; // index into gl3state.lightmap_textureIDs[]

	//msurface_t *lightmap_surfaces[MAX_LIGHTMAPS]; - no more lightmap chains, lightmaps are rendered multitextured

	int allocated[BLOCK_WIDTH];

	/* the lightmap texture data needs to be kept in
	   main memory so texsubimage can update properly */
	byte lightmap_buffers[MAX_LIGHTMAPS_PER_SURFACE][4 * BLOCK_WIDTH * BLOCK_HEIGHT];
	GX2Texture lmTex[MAX_LIGHTMAPS_PER_SURFACE];
} gl3lightmapstate_t;

extern gl3model_t *gl3_worldmodel;

extern float gl3depthmin, gl3depthmax;

extern cplane_t frustum[4];

extern vec3_t gl3_origin;

extern gl3image_t *gl3_notexture; /* use for bad textures */
extern gl3image_t *gl3_particletexture; /* little dot for particles */

extern int gl_filter_min;
extern int gl_filter_max;

static inline void
WiiU_UseProgram(gl3ShaderInfo_t *shaderProgram)
{
	if(shaderProgram != gl3state.currentShaderProgram)
	{
		gl3state.currentShaderProgram = shaderProgram;
		GX2SetFetchShader(&shaderProgram->shaderGroup.fetchShader);
		GX2SetVertexShader(shaderProgram->shaderGroup.vertexShader);
		GX2SetPixelShader(shaderProgram->shaderGroup.pixelShader);
	}
}

extern void WiiU_InitDynBuffers();
extern void WiiU_ShutdownDynBuffers();
extern void* WiiU_ABAlloc(uint32_t size);
extern void* WiiU_IBAlloc(uint32_t size);
extern void* WiiU_UBAlloc(uint32_t size);

extern void WiiU_BufferAndDraw3D(const gl3_3D_vtx_t* verts, int numVerts, int drawMode);

extern void WiiU_RotateForEntity(entity_t *e);

// gl3_sdl.c
extern int WiiU_InitContext(void* win);
extern void WiiU_GetDrawableSize(int* width, int* height);
extern int WiiU_PrepareForWindow(void);
extern qboolean WiiU_IsVsyncActive(void);
extern void WiiU_EndFrame(void);
extern void WiiU_SetVsync(void);
extern void WiiU_ShutdownContext(void);
extern int WiiU_GetSDLVersion(void);

// gl3_misc.c
extern void WiiU_InitParticleTexture(void);
extern void WiiU_ScreenShot(void);
extern void WiiU_SetDefaultState(void);

// gl3_model.c
extern int registration_sequence;
extern void WiiU_Mod_Init(void);
extern void WiiU_Mod_FreeAll(void);
extern void WiiU_BeginRegistration(char *model);
extern struct model_s * WiiU_RegisterModel(char *name);
extern void WiiU_EndRegistration(void);
extern void WiiU_Mod_Modellist_f(void);
extern const byte* WiiU_Mod_ClusterPVS(int cluster, const gl3model_t *model);

// gl3_draw.c
extern void WiiU_Draw_InitLocal(void);
extern void WiiU_Draw_ShutdownLocal(void);
extern gl3image_t * WiiU_Draw_FindPic(char *name);
extern void WiiU_Draw_GetPicSize(int *w, int *h, char *pic);

extern void WiiU_Draw_PicScaled(int x, int y, char *pic, float factor);
extern void WiiU_Draw_StretchPic(int x, int y, int w, int h, char *pic);
extern void WiiU_Draw_CharScaled(int x, int y, int num, float scale);
extern void WiiU_Draw_TileClear(int x, int y, int w, int h, char *pic);
extern void WiiU_DrawFrameBufferObject(int x, int y, int w, int h, GX2Texture *fboTexture, const float v_blend[4]);
extern void WiiU_Draw_Fill(int x, int y, int w, int h, int c);
extern void WiiU_Draw_FadeScreen(void);
extern void WiiU_Draw_Flash(const float color[4], float x, float y, float w, float h);
extern void WiiU_Draw_StretchRaw(int x, int y, int w, int h, int cols, int rows, const byte *data, int bits);

// gl3_image.c
extern void WiiU_TextureMode(char *string);
extern void WiiU_Bind(GX2Texture* texture);
extern void WiiU_BindLightmap(int lightmapnum);
extern gl3image_t *WiiU_LoadPic(char *name, byte *pic, int width, int realwidth,
                               int height, int realheight, size_t data_size,
                               imagetype_t type, int bits);
extern gl3image_t *WiiU_FindImage(const char *name, imagetype_t type);
extern gl3image_t *WiiU_RegisterSkin(char *name);
extern void WiiU_ShutdownImages(void);
extern void WiiU_FreeUnusedImages(void);
extern qboolean WiiU_ImageHasFreeSpace(void);
extern void WiiU_ImageList_f(void);

// gl3_light.c
extern void WiiU_UseLMShader();
extern void WiiU_UseLMFlowShader();
extern int r_dlightframecount;
extern void WiiU_MarkSurfaceLights(dlight_t *light, int bit, mnode_t *node,
	int r_dlightframecount);
extern void WiiU_PushDlights(void);
extern void WiiU_LightPoint(entity_t *currententity, vec3_t p, vec3_t color);
extern void WiiU_BuildLightMap(msurface_t *surf, int offsetInLMbuf, int stride);

extern void WiiU_LM_InitBlock(void);
extern void WiiU_LM_UploadBlock(void);
extern qboolean WiiU_LM_AllocBlock(int w, int h, int *x, int *y);
extern void WiiU_LM_BuildPolygonFromSurface(gl3model_t *currentmodel, msurface_t *fa);
extern void WiiU_LM_CreateSurfaceLightmap(msurface_t *surf);
extern void WiiU_LM_BeginBuildingLightmaps(gl3model_t *m);
extern void WiiU_LM_EndBuildingLightmaps(void);

// gl3_warp.c
extern void WiiU_EmitWaterPolys(msurface_t *fa);
extern void WiiU_SubdivideSurface(msurface_t *fa, gl3model_t* loadmodel);

extern void WiiU_SetSky(char *name, float rotate, vec3_t axis);
extern void WiiU_DrawSkyBox(void);
extern void WiiU_ClearSkyBox(void);
extern void WiiU_AddSkySurface(msurface_t *fa);


// gl3_surf.c
extern void WiiU_SurfInit(void);
extern void WiiU_SurfShutdown(void);
extern void WiiU_DrawGLPoly(msurface_t *fa);
extern void WiiU_DrawGLFlowingPoly(msurface_t *fa);
extern void WiiU_DrawTriangleOutlines(void);
extern void WiiU_DrawAlphaSurfaces(void);
extern void WiiU_DrawBrushModel(entity_t *e, gl3model_t *currentmodel);
extern void WiiU_DrawWorld(void);
extern void WiiU_MarkLeaves(void);

// gl3_mesh.c
extern void WiiU_DrawAliasModel(entity_t *e);
extern void WiiU_ResetShadowAliasModels(void);
extern void WiiU_DrawAliasShadows(void);
extern void WiiU_ShutdownMeshes(void);

// gl3_shaders.c

extern qboolean WiiU_RecreateShaders(void);
extern qboolean WiiU_InitShaders(void);
extern void WiiU_ShutdownShaders(void);
extern void WiiU_UpdateUBOCommon(void);
extern void WiiU_UpdateUBO2D(void);
extern void WiiU_UpdateUBO3D(void);
extern void WiiU_UpdateUBOLights(void);
extern void WiiU_UpdateUBOLMScales(hmm_vec4 lmScales[MAX_LIGHTMAPS_PER_SURFACE]);
extern void WiiU_UpdateUBOPostProcess(float time, const float v_blend[4]);

// wiiu_state.c

extern void WiiU_EnableDepthTest(qboolean enable);
extern void WiiU_EnableDepthWrite(qboolean enable);
extern void WiiU_EnableBlend(qboolean enable);
extern void WiiU_SetViewport(int x, int y, int w, int h);
extern void WiiU_SetDepthRange(float near, float far);
extern void WiiU_EnableCullFace(qboolean enable);
extern void WiiU_CullFront(void);
extern void WiiU_CullBack(void);
extern void WiiU_EnablePolygonOffset(qboolean enable);
extern void WiiU_PolygonOffset(float factor, float units);

// wiiu_framebuffer.c

//#ifdef USE_FBO
extern void WiiU_FBO_Init(framebuffer_t *fbo, int width, int height);
extern void WiiU_FBO_Shutdown(framebuffer_t *fbo);
extern void WiiU_FBO_Activate(framebuffer_t *fbo);
extern GX2Texture *WiiU_FBO_ContentDone(framebuffer_t *fbo);
//extern void WiiU_FBO_Draw();
//#endif

// ############ Cvars ###########

extern cvar_t *gl_msaa_samples;
extern cvar_t *r_vsync;
extern cvar_t *r_retexturing;
extern cvar_t *r_scale8bittextures;
extern cvar_t *vid_fullscreen;
extern cvar_t *r_mode;
extern cvar_t *r_customwidth;
extern cvar_t *r_customheight;

extern cvar_t *r_2D_unfiltered;
extern cvar_t *r_3D_unfiltered;
extern cvar_t *r_videos_unfiltered;
extern cvar_t *r_lightmaps_unfiltered;
extern cvar_t *gl_nolerp_list;
extern cvar_t *r_lerp_list;
extern cvar_t *gl_nobind;
extern cvar_t *r_lockpvs;
extern cvar_t *r_novis;

extern cvar_t *r_cull;
extern cvar_t *gl_zfix;
extern cvar_t *r_fullbright;

extern cvar_t *r_norefresh;
extern cvar_t *gl_lefthand;
extern cvar_t *r_gunfov;
extern cvar_t *r_farsee;
extern cvar_t *r_drawworld;

extern cvar_t *vid_gamma;
extern cvar_t *gl3_intensity;
extern cvar_t *gl3_intensity_2D;
extern cvar_t *gl_anisotropic;
extern cvar_t *gl_texturemode;

extern cvar_t *r_lightlevel;
extern cvar_t *gl3_overbrightbits;
extern cvar_t *gl3_particle_fade_factor;
extern cvar_t *gl3_particle_square;
extern cvar_t *gl3_colorlight;
extern cvar_t *gl_polyblend;

extern cvar_t *r_modulate;
extern cvar_t *gl_shadows;
extern cvar_t *r_fixsurfsky;
extern cvar_t *r_palettedtexture;
extern cvar_t *r_validation;

extern cvar_t *gl3_debugcontext;

extern cvar_t *wiiu_debugmips;
extern cvar_t *wiiu_drc;

#endif /* SRC_CLIENT_REFRESH_WiiU_HEADER_LOCAL_H_ */
