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
 * Refresher setup and main part of the frame generation, for OpenGL3
 *
 * =======================================================================
 */

#include "../ref_shared.h"
#include "header/local.h"
#include <malloc.h>

refimport_t ri;

#define HANDMADE_MATH_IMPLEMENTATION
#include "header/HandmadeMath.h"

#define DG_DYNARR_IMPLEMENTATION
#include "header/DG_dynarr.h"

#define REF_VERSION "Yamagi Quake II WiiU Refresher"


gl3config_t gl3config;
gl3state_t gl3state;

unsigned gl3_rawpalette[256];

/* screen size info */
refdef_t gl3_newrefdef;

viddef_t vid;
gl3model_t *gl3_worldmodel;

float gl3depthmin=0.0f, gl3depthmax=1.0f;

cplane_t frustum[4];

/* view origin */
vec3_t vup;
vec3_t vpn;
vec3_t vright;
vec3_t gl3_origin;

int gl3_visframecount; /* bumped when going to a new PVS */
int gl3_framecount; /* used for dlight push checking */

int c_brush_polys, c_alias_polys;

static float v_blend[4]; /* final blending color */

int gl3_viewcluster, gl3_viewcluster2, gl3_oldviewcluster, gl3_oldviewcluster2;

const hmm_mat4 gl3_identityMat4 = {{
		{1, 0, 0, 0},
		{0, 1, 0, 0},
		{0, 0, 1, 0},
		{0, 0, 0, 1},
}};

cvar_t *gl_msaa_samples;
cvar_t *r_vsync;
cvar_t *r_retexturing;
cvar_t *r_scale8bittextures;
//cvar_t *vid_fullscreen;
cvar_t *r_mode;
cvar_t *r_customwidth;
cvar_t *r_customheight;
extern cvar_t *vid_gamma;
cvar_t *gl_anisotropic;
cvar_t *gl_texturemode;
cvar_t *r_clear;
cvar_t *gl3_particle_size;
cvar_t *gl3_particle_fade_factor;
cvar_t *gl3_particle_square;
cvar_t *gl3_colorlight;
cvar_t *gl_polyblend;

cvar_t *gl_lefthand;
cvar_t *r_gunfov;
cvar_t *r_farsee;

cvar_t *gl3_intensity;
cvar_t *gl3_intensity_2D;
cvar_t *r_lightlevel;
cvar_t *gl3_overbrightbits;

cvar_t *r_norefresh;
cvar_t *r_drawentities;
cvar_t *r_drawworld;
cvar_t *gl_nolerp_list;
cvar_t *r_lerp_list;
cvar_t *r_2D_unfiltered;
cvar_t *r_3D_unfiltered;
cvar_t *r_videos_unfiltered;
cvar_t *r_lightmaps_unfiltered;
cvar_t *gl_nobind;
cvar_t *r_lockpvs;
cvar_t *r_novis;
cvar_t *r_speeds;

cvar_t *r_cull;
cvar_t *gl_zfix;
cvar_t *r_fullbright;
cvar_t *r_modulate;
cvar_t *gl_shadows;
cvar_t *gl3_debugcontext;
cvar_t *r_fixsurfsky;
cvar_t *r_palettedtexture;
cvar_t *r_validation;
cvar_t *gl3_usefbo;

cvar_t *wiiu_debugmips;
cvar_t *wiiu_drc;



// Yaw-Pitch-Roll
// equivalent to R_z * R_y * R_x where R_x is the trans matrix for rotating around X axis for aroundXdeg
static hmm_mat4 rotAroundAxisZYX(float aroundZdeg, float aroundYdeg, float aroundXdeg)
{
	// Naming of variables is consistent with http://planning.cs.uiuc.edu/node102.html
	// and https://de.wikipedia.org/wiki/Roll-Nick-Gier-Winkel#.E2.80.9EZY.E2.80.B2X.E2.80.B3-Konvention.E2.80.9C
	float alpha = HMM_ToRadians(aroundZdeg);
	float beta = HMM_ToRadians(aroundYdeg);
	float gamma = HMM_ToRadians(aroundXdeg);

	float sinA = HMM_SinF(alpha);
	float cosA = HMM_CosF(alpha);
	// TODO: or sincosf(alpha, &sinA, &cosA); ?? (not a standard function)
	float sinB = HMM_SinF(beta);
	float cosB = HMM_CosF(beta);
	float sinG = HMM_SinF(gamma);
	float cosG = HMM_CosF(gamma);

	hmm_mat4 ret = {{
		{ cosA*cosB,                  sinA*cosB,                   -sinB,    0 }, // first *column*
		{ cosA*sinB*sinG - sinA*cosG, sinA*sinB*sinG + cosA*cosG, cosB*sinG, 0 },
		{ cosA*sinB*cosG + sinA*sinG, sinA*sinB*cosG - cosA*sinG, cosB*cosG, 0 },
		{  0,                          0,                          0,        1 }
	}};

	return ret;
}

void
WiiU_RotateForEntity(entity_t *e)
{
	// angles: pitch (around y), yaw (around z), roll (around x)
	// rot matrices to be multiplied in order Z, Y, X (yaw, pitch, roll)
	hmm_mat4 transMat = rotAroundAxisZYX(e->angles[1], -e->angles[0], -e->angles[2]);

	for(int i=0; i<3; ++i)
	{
		transMat.Elements[3][i] = e->origin[i]; // set translation
	}

	gl3state.uni3DData.transModelMat4 = HMM_MultiplyMat4(gl3state.uni3DData.transModelMat4, transMat);

	WiiU_UpdateUBO3D();
}


static void
WiiU_Register(void)
{
	gl_lefthand = ri.Cvar_Get("hand", "0", CVAR_USERINFO | CVAR_ARCHIVE);
	r_gunfov = ri.Cvar_Get("r_gunfov", "80", CVAR_ARCHIVE);
	r_farsee = ri.Cvar_Get("r_farsee", "0", CVAR_LATCH | CVAR_ARCHIVE);

	r_vsync = ri.Cvar_Get("r_vsync", "1", CVAR_ARCHIVE);
	gl_msaa_samples = ri.Cvar_Get ( "r_msaa_samples", "0", CVAR_ARCHIVE );
	r_retexturing = ri.Cvar_Get("r_retexturing", "1", CVAR_ARCHIVE);
	r_scale8bittextures = ri.Cvar_Get("r_scale8bittextures", "0", CVAR_ARCHIVE);
	gl3_debugcontext = ri.Cvar_Get("gl3_debugcontext", "0", 0);
	r_mode = ri.Cvar_Get("r_mode", "4", CVAR_ARCHIVE);
	r_customwidth = ri.Cvar_Get("r_customwidth", "1024", CVAR_ARCHIVE);
	r_customheight = ri.Cvar_Get("r_customheight", "768", CVAR_ARCHIVE);
	gl3_particle_size = ri.Cvar_Get("gl3_particle_size", "40", CVAR_ARCHIVE);
	gl3_particle_fade_factor = ri.Cvar_Get("gl3_particle_fade_factor", "1.2", CVAR_ARCHIVE);
	gl3_particle_square = ri.Cvar_Get("gl3_particle_square", "0", CVAR_ARCHIVE);
	// if set to 0, lights (from lightmaps, dynamic lights and on models) are white instead of colored
	gl3_colorlight = ri.Cvar_Get("gl3_colorlight", "1", CVAR_ARCHIVE);
	gl_polyblend = ri.Cvar_Get("gl_polyblend", "1", CVAR_ARCHIVE);

	r_norefresh = ri.Cvar_Get("r_norefresh", "0", 0);
	r_drawentities = ri.Cvar_Get("r_drawentities", "1", 0);
	r_drawworld = ri.Cvar_Get("r_drawworld", "1", 0);
	r_fullbright = ri.Cvar_Get("r_fullbright", "0", 0);
	r_fixsurfsky = ri.Cvar_Get("r_fixsurfsky", "0", CVAR_ARCHIVE);
	r_palettedtexture = ri.Cvar_Get("r_palettedtexture", "0", 0);
	r_validation = ri.Cvar_Get("r_validation", "0", CVAR_ARCHIVE);

	/* don't bilerp characters and crosshairs */
	gl_nolerp_list = ri.Cvar_Get("r_nolerp_list", "pics/conchars.pcx pics/ch1.pcx pics/ch2.pcx pics/ch3.pcx", CVAR_ARCHIVE);
	/* textures that should always be filtered, even if r_2D_unfiltered or an unfiltered gl mode is used */
	r_lerp_list = ri.Cvar_Get("r_lerp_list", "", CVAR_ARCHIVE);
	/* don't bilerp any 2D elements */
	r_2D_unfiltered = ri.Cvar_Get("r_2D_unfiltered", "1", CVAR_ARCHIVE);
	/* don't bilerp any 3d elements */
	r_3D_unfiltered = ri.Cvar_Get("r_3D_unfiltered", "1", CVAR_ARCHIVE);
	/* don't bilerp videos */
	r_videos_unfiltered = ri.Cvar_Get("r_videos_unfiltered", "0", CVAR_ARCHIVE);
	/* don't bilerp lightmaps */
	r_lightmaps_unfiltered = ri.Cvar_Get("r_lightmaps_unfiltered", "0", CVAR_ARCHIVE);
	gl_nobind = ri.Cvar_Get("gl_nobind", "0", 0);

	gl_texturemode = ri.Cvar_Get("gl_texturemode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE);
	gl_anisotropic = ri.Cvar_Get("r_anisotropic", "0", CVAR_ARCHIVE);

	vid_fullscreen = ri.Cvar_Get("vid_fullscreen", "0", CVAR_ARCHIVE);
	vid_gamma = ri.Cvar_Get("vid_gamma", "1.2", CVAR_ARCHIVE);
	gl3_intensity = ri.Cvar_Get("gl3_intensity", "1.5", CVAR_ARCHIVE);
	gl3_intensity_2D = ri.Cvar_Get("gl3_intensity_2D", "1.5", CVAR_ARCHIVE);

	r_lightlevel = ri.Cvar_Get("r_lightlevel", "0", 0);
	gl3_overbrightbits = ri.Cvar_Get("gl3_overbrightbits", "1.3", CVAR_ARCHIVE);

	gl_shadows = ri.Cvar_Get("r_shadows", "0", CVAR_ARCHIVE);

	r_modulate = ri.Cvar_Get("r_modulate", "1", CVAR_ARCHIVE);
	gl_zfix = ri.Cvar_Get("gl_zfix", "0", 0);
	r_clear = ri.Cvar_Get("r_clear", "0", 0);
	r_cull = ri.Cvar_Get("r_cull", "1", 0);
	r_lockpvs = ri.Cvar_Get("r_lockpvs", "0", 0);
	r_novis = ri.Cvar_Get("r_novis", "0", 0);
	r_speeds = ri.Cvar_Get("r_speeds", "0", 0);

	gl3_usefbo = ri.Cvar_Get("gl3_usefbo", "1", CVAR_ARCHIVE); // use framebuffer object for postprocess effects (water)

	wiiu_debugmips = ri.Cvar_Get("wiiu_debugmips", "0", 0); // turn all mipmap enabled textures to debug colors
	wiiu_drc = ri.Cvar_Get("wiiu_drc", "1", CVAR_ARCHIVE); // enable DRC output

#if 0 // TODO!
	//gl_lefthand = ri.Cvar_Get("hand", "0", CVAR_USERINFO | CVAR_ARCHIVE);
	//gl_farsee = ri.Cvar_Get("gl_farsee", "0", CVAR_LATCH | CVAR_ARCHIVE);
	//r_norefresh = ri.Cvar_Get("r_norefresh", "0", 0);
	//r_fullbright = ri.Cvar_Get("r_fullbright", "0", 0);
	//r_drawentities = ri.Cvar_Get("r_drawentities", "1", 0);
	//r_drawworld = ri.Cvar_Get("r_drawworld", "1", 0);
	//r_novis = ri.Cvar_Get("r_novis", "0", 0);
	//r_lerpmodels = ri.Cvar_Get("r_lerpmodels", "1", 0); NOTE: screw this, it looks horrible without
	//r_speeds = ri.Cvar_Get("r_speeds", "0", 0);

	//r_lightlevel = ri.Cvar_Get("r_lightlevel", "0", 0);
	//gl_overbrightbits = ri.Cvar_Get("gl_overbrightbits", "0", CVAR_ARCHIVE);

	gl1_particle_min_size = ri.Cvar_Get("gl1_particle_min_size", "2", CVAR_ARCHIVE);
	gl1_particle_max_size = ri.Cvar_Get("gl1_particle_max_size", "40", CVAR_ARCHIVE);
	//gl1_particle_size = ri.Cvar_Get("gl1_particle_size", "40", CVAR_ARCHIVE);
	gl1_particle_att_a = ri.Cvar_Get("gl1_particle_att_a", "0.01", CVAR_ARCHIVE);
	gl1_particle_att_b = ri.Cvar_Get("gl1_particle_att_b", "0.0", CVAR_ARCHIVE);
	gl1_particle_att_c = ri.Cvar_Get("gl1_particle_att_c", "0.01", CVAR_ARCHIVE);

	//gl_modulate = ri.Cvar_Get("gl_modulate", "1", CVAR_ARCHIVE);
	//r_mode = ri.Cvar_Get("r_mode", "4", CVAR_ARCHIVE);
	//gl_shadows = ri.Cvar_Get("r_shadows", "0", CVAR_ARCHIVE);
	//gl_nobind = ri.Cvar_Get("gl_nobind", "0", 0);
	gl_showtris = ri.Cvar_Get("gl_showtris", "0", 0);
	gl_showbbox = Cvar_Get("gl_showbbox", "0", 0);
	//gl1_ztrick = ri.Cvar_Get("gl1_ztrick", "0", 0); NOTE: dump this.
	//gl_zfix = ri.Cvar_Get("gl_zfix", "0", 0);
	r_clear = ri.Cvar_Get("r_clear", "0", 0);
	//gl1_flashblend = ri.Cvar_Get("gl1_flashblend", "0", 0);

	//gl_texturemode = ri.Cvar_Get("gl_texturemode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE);
	gl1_texturealphamode = ri.Cvar_Get("gl1_texturealphamode", "default", CVAR_ARCHIVE);
	gl1_texturesolidmode = ri.Cvar_Get("gl1_texturesolidmode", "default", CVAR_ARCHIVE);
	//gl_anisotropic = ri.Cvar_Get("r_anisotropic", "0", CVAR_ARCHIVE);
	//r_lockpvs = ri.Cvar_Get("r_lockpvs", "0", 0);

	//gl1_palettedtexture = ri.Cvar_Get("gl1_palettedtexture", "0", CVAR_ARCHIVE); NOPE.
	gl1_pointparameters = ri.Cvar_Get("gl1_pointparameters", "1", CVAR_ARCHIVE);

	//r_vsync = ri.Cvar_Get("r_vsync", "1", CVAR_ARCHIVE);


	//vid_fullscreen = ri.Cvar_Get("vid_fullscreen", "0", CVAR_ARCHIVE);
	//vid_gamma = ri.Cvar_Get("vid_gamma", "1.0", CVAR_ARCHIVE);

	//r_customwidth = ri.Cvar_Get("r_customwidth", "1024", CVAR_ARCHIVE);
	//r_customheight = ri.Cvar_Get("r_customheight", "768", CVAR_ARCHIVE);
	//gl_msaa_samples = ri.Cvar_Get ( "r_msaa_samples", "0", CVAR_ARCHIVE );

	//r_retexturing = ri.Cvar_Get("r_retexturing", "1", CVAR_ARCHIVE);


	gl1_stereo = ri.Cvar_Get( "gl1_stereo", "0", CVAR_ARCHIVE );
	gl1_stereo_separation = ri.Cvar_Get( "gl1_stereo_separation", "-0.4", CVAR_ARCHIVE );
	gl1_stereo_anaglyph_colors = ri.Cvar_Get( "gl1_stereo_anaglyph_colors", "rc", CVAR_ARCHIVE );
	gl1_stereo_convergence = ri.Cvar_Get( "gl1_stereo_convergence", "1", CVAR_ARCHIVE );
#endif // 0

	//ri.Cmd_AddCommand("imagelist", WiiU_ImageList_f);
	//ri.Cmd_AddCommand("screenshot", WiiU_ScreenShot);
	//ri.Cmd_AddCommand("modellist", WiiU_Mod_Modellist_f);
	//ri.Cmd_AddCommand("gl_strings", WiiU_Strings);
}

/*
 * Changes the video mode
 */

// the following is only used in the next to functions,
// no need to put it in a header
enum
{
	rserr_ok,

	rserr_invalid_mode,

	rserr_unknown
};

static int
SetMode_impl(int *pwidth, int *pheight, int mode, int fullscreen)
{
	R_Printf(PRINT_ALL, "Setting mode %d:", mode);

	/* mode -1 is not in the vid mode table - so we keep the values in pwidth
	   and pheight and don't even try to look up the mode info */
	if ((mode >= 0) && !ri.Vid_GetModeInfo(pwidth, pheight, mode))
	{
		R_Printf(PRINT_ALL, " invalid mode\n");
		return rserr_invalid_mode;
	}

	/* We trying to get resolution from desktop */
	if (mode == -2)
	{
		if(!ri.GLimp_GetDesktopMode(pwidth, pheight))
		{
			R_Printf( PRINT_ALL, " can't detect mode\n" );
			return rserr_invalid_mode;
		}
	}

	R_Printf(PRINT_ALL, " %dx%d (vid_fullscreen %i)\n", *pwidth, *pheight, fullscreen);


	if (!ri.GLimp_InitGraphics(fullscreen, pwidth, pheight))
	{
		return rserr_invalid_mode;
	}

	return rserr_ok;
}

static qboolean
WiiU_SetMode(void)
{
	int err;
	int fullscreen;

	fullscreen = (int)vid_fullscreen->value;

	/* a bit hackish approach to enable custom resolutions:
	   Glimp_SetMode needs these values set for mode -1 */
	vid.width = r_customwidth->value;
	vid.height = r_customheight->value;

	if ((err = SetMode_impl(&vid.width, &vid.height, r_mode->value, fullscreen)) == rserr_ok)
	{
		if (r_mode->value == -1)
		{
			gl3state.prev_mode = 4; /* safe default for custom mode */
		}
		else
		{
			gl3state.prev_mode = r_mode->value;
		}
	}
	else
	{
		if (err == rserr_invalid_mode)
		{
			R_Printf(PRINT_ALL, "ref_gl3::WiiU_SetMode() - invalid mode\n");

			if (gl_msaa_samples->value != 0.0f)
			{
				R_Printf(PRINT_ALL, "gl_msaa_samples was %d - will try again with gl_msaa_samples = 0\n", (int)gl_msaa_samples->value);
				ri.Cvar_SetValue("r_msaa_samples", 0.0f);
				gl_msaa_samples->modified = false;

				if ((err = SetMode_impl(&vid.width, &vid.height, r_mode->value, 0)) == rserr_ok)
				{
					return true;
				}
			}
			if(r_mode->value == gl3state.prev_mode)
			{
				// trying again would result in a crash anyway, give up already
				// (this would happen if your initing fails at all and your resolution already was 640x480)
				return false;
			}

			ri.Cvar_SetValue("r_mode", gl3state.prev_mode);
			r_mode->modified = false;
		}

		/* try setting it back to something safe */
		if ((err = SetMode_impl(&vid.width, &vid.height, gl3state.prev_mode, 0)) != rserr_ok)
		{
			R_Printf(PRINT_ALL, "ref_gl3::WiiU_SetMode() - could not revert to safe mode\n");
			return false;
		}
	}

	return true;
}

static qboolean
WiiU_Init(void)
{
	WiiU_InitDynBuffers();
	// __WIIU_TODO__
	// gl3config.stencil = true;

	byte *colormap;

	Swap_Init(); // FIXME: for fucks sake, this doesn't have to be done at runtime!

	R_Printf(PRINT_ALL, "Refresh: " REF_VERSION "\n");
	R_Printf(PRINT_ALL, "Client: " YQ2VERSION "\n\n");

	GetPCXPalette (&colormap, d_8to24table);
	free(colormap);

	WiiU_Register();

	/* create the window and set up the context */
	if (!WiiU_SetMode())
	{
		R_Printf(PRINT_ALL, "ref_gl3::R_Init() - could not R_SetMode()\n");
		return false;
	}

	ri.Vid_MenuInit();

	// generate texture handles for all possible lightmaps
	//glGenTextures(MAX_LIGHTMAPS*MAX_LIGHTMAPS_PER_SURFACE, gl3state.lightmap_textureIDs[0]);

	WiiU_SetDefaultState();

	if(WiiU_InitShaders())
	{
		R_Printf(PRINT_ALL, "Loading shaders succeeded.\n");
	}
	else
	{
		R_Printf(PRINT_ALL, "Loading shaders failed.\n");
		return false;
	}

	registration_sequence = 1; // from R_InitImages() (everything else from there shouldn't be needed anymore)

	WiiU_Mod_Init();

	WiiU_InitParticleTexture();

	WiiU_Draw_InitLocal();

	WiiU_SurfInit();

	{
		GX2TexXYFilterMode filter = (r_videos_unfiltered->value == 0) ? GX2_TEX_XY_FILTER_MODE_LINEAR : GX2_TEX_XY_FILTER_MODE_POINT;
		GX2InitSampler(&gl3state.samplerVideo, GX2_TEX_CLAMP_MODE_CLAMP, filter);
	}
	{
		GX2TexXYFilterMode filter = (r_2D_unfiltered->value == 0) ? GX2_TEX_XY_FILTER_MODE_LINEAR : GX2_TEX_XY_FILTER_MODE_POINT;
		GX2InitSampler(&gl3state.sampler2D, GX2_TEX_CLAMP_MODE_CLAMP, filter);
	}
	{
		GX2TexXYFilterMode filter = GX2_TEX_XY_FILTER_MODE_POINT;
		GX2InitSampler(&gl3state.sampler2DUnfiltered, GX2_TEX_CLAMP_MODE_CLAMP, filter);
	}
	{
		GX2TexXYFilterMode filter = (r_3D_unfiltered->value == 0) ? GX2_TEX_XY_FILTER_MODE_LINEAR : GX2_TEX_XY_FILTER_MODE_POINT;
		GX2InitSampler(&gl3state.sampler3D, GX2_TEX_CLAMP_MODE_WRAP, filter);
		//GX2InitSamplerZMFilter(&gl3state.sampler3D, GX2_TEX_Z_FILTER_MODE_LINEAR, GX2_TEX_Z_FILTER_MODE_LINEAR);
		GX2InitSamplerZMFilter(&gl3state.sampler3D, GX2_TEX_Z_FILTER_MODE_POINT, GX2_TEX_Z_FILTER_MODE_POINT);
		//GX2InitSamplerLOD(&gl3state.sampler3D, 0,16, 2);
	}
	{
		GX2TexXYFilterMode filter = (r_lightmaps_unfiltered->value == 0) ? GX2_TEX_XY_FILTER_MODE_LINEAR : GX2_TEX_XY_FILTER_MODE_POINT;
		GX2InitSampler(&gl3state.samplerLightmaps, GX2_TEX_CLAMP_MODE_WRAP, filter);
	}

#ifdef USE_FBO
	WiiU_FBO_Init(&gl3state.mainFBO, 1280, 600);
	WiiU_FBO_Init(&gl3state.worldFBO, 1280, 600);
#endif
#ifdef __WIIU_TODO__
	glGenFramebuffers(1, &gl3state.ppFBO);
	// the rest for the FBO is done dynamically in WiiU_RenderView() so it can
	// take the viewsize into account (enforce that by setting invalid size)
	gl3state.ppFBtexWidth = gl3state.ppFBtexHeight = -1;

	R_Printf(PRINT_ALL, "\n");
#endif
	return true;
}

void
WiiU_Shutdown(void)
{
	ri.Cmd_RemoveCommand("modellist");
	ri.Cmd_RemoveCommand("screenshot");
	ri.Cmd_RemoveCommand("imagelist");
	ri.Cmd_RemoveCommand("gl_strings");

	WiiU_Mod_FreeAll();
	WiiU_ShutdownMeshes();
	WiiU_ShutdownImages();
	WiiU_SurfShutdown();
	WiiU_Draw_ShutdownLocal();
	WiiU_ShutdownShaders();

#ifdef __WIIU_TODO__
	// free the postprocessing FBO and its renderbuffer and texture
	if(gl3state.ppFBrbo != 0)
		glDeleteRenderbuffers(1, &gl3state.ppFBrbo);
	if(gl3state.ppFBtex != 0)
		glDeleteTextures(1, &gl3state.ppFBtex);
	if(gl3state.ppFBO != 0)
		glDeleteFramebuffers(1, &gl3state.ppFBO);
	gl3state.ppFBrbo = gl3state.ppFBtex = gl3state.ppFBO = 0;
	gl3state.ppFBObound = false;
	gl3state.ppFBtexWidth = gl3state.ppFBtexHeight = -1;
#endif

	WiiU_ShutdownDynBuffers();
	/* shutdown OS specific OpenGL stuff like contexts, etc.  */
	WiiU_ShutdownContext();
}

// assumes gl3state.v[ab]o3D are bound
// buffers and draws gl3_3D_vtx_t vertices
// drawMode is something like GL_TRIANGLE_STRIP or GL_TRIANGLE_FAN or whatever
void
WiiU_BufferAndDraw3D(const gl3_3D_vtx_t* verts, int numVerts, int drawMode)
{
	uint32_t one = sizeof(gl3_3D_vtx_t);
	uint32_t abSize = one * numVerts;
	void* ab = WiiU_ABAlloc(abSize);
	memcpy(ab, verts, abSize);

	//DCStoreRange(ab, abSize);
    GX2Invalidate(GX2_INVALIDATE_MODE_ATTRIBUTE_BUFFER | GX2_INVALIDATE_MODE_CPU,
		ab, abSize);
	GX2SetAttribBuffer(0, abSize, one, ab);
    GX2DrawEx(drawMode, numVerts, 0, 1);
}

static void
WiiU_DrawBeam(entity_t *e)
{
	int i;
	float r, g, b;

	enum { NUM_BEAM_SEGS = 6 };

	vec3_t perpvec;
	vec3_t direction, normalized_direction;
	vec3_t start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	vec3_t oldorigin, origin;

	gl3_3D_vtx_t verts[NUM_BEAM_SEGS*4];
	unsigned int pointb;

	oldorigin[0] = e->oldorigin[0];
	oldorigin[1] = e->oldorigin[1];
	oldorigin[2] = e->oldorigin[2];

	origin[0] = e->origin[0];
	origin[1] = e->origin[1];
	origin[2] = e->origin[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if (VectorNormalize(normalized_direction) == 0)
	{
		return;
	}

	PerpendicularVector(perpvec, normalized_direction);
	VectorScale(perpvec, e->frame / 2, perpvec);

	for (i = 0; i < 6; i++)
	{
		RotatePointAroundVector(start_points[i], normalized_direction, perpvec,
		                        (360.0 / NUM_BEAM_SEGS) * i);
		VectorAdd(start_points[i], origin, start_points[i]);
		VectorAdd(start_points[i], direction, end_points[i]);
	}

	WiiU_EnableBlend(true);
	WiiU_EnableDepthWrite(false);

	WiiU_UseProgram(&gl3state.si3DcolorOnly);

	r = (LittleLong(d_8to24table[e->skinnum & 0xFF])) & 0xFF;
	g = (LittleLong(d_8to24table[e->skinnum & 0xFF]) >> 8) & 0xFF;
	b = (LittleLong(d_8to24table[e->skinnum & 0xFF]) >> 16) & 0xFF;

	r *= 1 / 255.0F;
	g *= 1 / 255.0F;
	b *= 1 / 255.0F;

	gl3state.uniCommonData.color = HMM_Vec4(r, g, b, e->alpha);
	WiiU_UpdateUBOCommon();

	for ( i = 0; i < NUM_BEAM_SEGS; i++ )
	{
		VectorCopy(start_points[i], verts[4*i+0].pos);
		VectorCopy(end_points[i], verts[4*i+1].pos);

		pointb = ( i + 1 ) % NUM_BEAM_SEGS;

		VectorCopy(start_points[pointb], verts[4*i+2].pos);
		VectorCopy(end_points[pointb], verts[4*i+3].pos);
	}

	WiiU_BufferAndDraw3D(verts, NUM_BEAM_SEGS*4, GX2_PRIMITIVE_MODE_TRIANGLE_STRIP);

	WiiU_EnableBlend(false);
	WiiU_EnableDepthWrite(true);
}

static void
WiiU_DrawSpriteModel(entity_t *e, gl3model_t *currentmodel)
{
	float alpha = 1.0F;
	gl3_3D_vtx_t verts[4];
	dsprframe_t *frame;
	float *up, *right;
	dsprite_t *psprite;
	gl3image_t *skin;

	/* don't even bother culling, because it's just
	   a single polygon without a surface cache */
	psprite = (dsprite_t *)currentmodel->extradata;

	e->frame %= psprite->numframes;
	frame = &psprite->frames[e->frame];

	/* normal sprite */
	up = vup;
	right = vright;

	if (e->flags & RF_TRANSLUCENT)
	{
		alpha = e->alpha;
	}

	if (alpha != gl3state.uni3DData.alpha)
	{
		gl3state.uni3DData.alpha = alpha;
		WiiU_UpdateUBO3D();
	}

	skin = currentmodel->skins[e->frame];
	if (!skin)
	{
		skin = gl3_notexture; /* fallback... */
	}

	WiiU_Bind(&skin->texture);

	if (alpha == 1.0)
	{
		// use shader with alpha test
		WiiU_UseProgram(&gl3state.si3DspriteAlpha);
	}
	else
	{
		WiiU_EnableBlend(true);

		WiiU_UseProgram(&gl3state.si3Dsprite);
	}

	verts[0].texCoord[0] = 0;
	verts[0].texCoord[1] = 1;
	verts[1].texCoord[0] = 0;
	verts[1].texCoord[1] = 0;
	verts[2].texCoord[0] = 1;
	verts[2].texCoord[1] = 0;
	verts[3].texCoord[0] = 1;
	verts[3].texCoord[1] = 1;

	VectorMA( e->origin, -frame->origin_y, up, verts[0].pos );
	VectorMA( verts[0].pos, -frame->origin_x, right, verts[0].pos );

	VectorMA( e->origin, frame->height - frame->origin_y, up, verts[1].pos );
	VectorMA( verts[1].pos, -frame->origin_x, right, verts[1].pos );

	VectorMA( e->origin, frame->height - frame->origin_y, up, verts[2].pos );
	VectorMA( verts[2].pos, frame->width - frame->origin_x, right, verts[2].pos );

	VectorMA( e->origin, -frame->origin_y, up, verts[3].pos );
	VectorMA( verts[3].pos, frame->width - frame->origin_x, right, verts[3].pos );

	WiiU_BufferAndDraw3D(verts, 4, GX2_PRIMITIVE_MODE_TRIANGLE_FAN);

	if (alpha != 1.0F)
	{
		WiiU_EnableBlend(false);
		gl3state.uni3DData.alpha = 1.0f;
		WiiU_UpdateUBO3D();
	}
}

static void
WiiU_DrawNullModel(entity_t *currententity)
{
	vec3_t shadelight;

	if (currententity->flags & RF_FULLBRIGHT)
	{
		shadelight[0] = shadelight[1] = shadelight[2] = 1.0F;
	}
	else
	{
		WiiU_LightPoint(currententity, currententity->origin, shadelight);
	}

	hmm_mat4 origModelMat = gl3state.uni3DData.transModelMat4;
	WiiU_RotateForEntity(currententity);

	gl3state.uniCommonData.color = HMM_Vec4( shadelight[0], shadelight[1], shadelight[2], 1 );
	WiiU_UpdateUBOCommon();

	WiiU_UseProgram(&gl3state.si3DcolorOnly);

	gl3_3D_vtx_t vtxA[6] = {
		{{0, 0, -16}, {0,0}, {0,0}},
		{{16 * cos( 0 * M_PI / 2 ), 16 * sin( 0 * M_PI / 2 ), 0}, {0,0}, {0,0}},
		{{16 * cos( 1 * M_PI / 2 ), 16 * sin( 1 * M_PI / 2 ), 0}, {0,0}, {0,0}},
		{{16 * cos( 2 * M_PI / 2 ), 16 * sin( 2 * M_PI / 2 ), 0}, {0,0}, {0,0}},
		{{16 * cos( 3 * M_PI / 2 ), 16 * sin( 3 * M_PI / 2 ), 0}, {0,0}, {0,0}},
		{{16 * cos( 4 * M_PI / 2 ), 16 * sin( 4 * M_PI / 2 ), 0}, {0,0}, {0,0}}
	};

	WiiU_BufferAndDraw3D(vtxA, 6, GX2_PRIMITIVE_MODE_TRIANGLE_FAN);

	gl3_3D_vtx_t vtxB[6] = {
		{{0, 0, 16}, {0,0}, {0,0}},
		vtxA[5], vtxA[4], vtxA[3], vtxA[2], vtxA[1]
	};

	WiiU_BufferAndDraw3D(vtxB, 6, GX2_PRIMITIVE_MODE_TRIANGLE_FAN);

	gl3state.uni3DData.transModelMat4 = origModelMat;
	WiiU_UpdateUBO3D();
}

static void
WiiU_DrawParticles(void)
{
	// TODO: stereo
	//qboolean stereo_split_tb = ((gl_state.stereo_mode == STEREO_SPLIT_VERTICAL) && gl_state.camera_separation);
	//qboolean stereo_split_lr = ((gl_state.stereo_mode == STEREO_SPLIT_HORIZONTAL) && gl_state.camera_separation);

	//if (!(stereo_split_tb || stereo_split_lr))
	{
		int i;
		int numParticles = gl3_newrefdef.num_particles;
		YQ2_ALIGNAS_TYPE(unsigned) byte color[4];
		const particle_t *p;
		// assume the size looks good with window height 480px and scale according to real resolution
		float pointSize = gl3_particle_size->value * (float)gl3_newrefdef.height/480.0f;

		typedef struct part_vtx {
			float pos[3];
			float size;
			float dist;
			float color[4];
		} part_vtx;
		YQ2_STATIC_ASSERT(sizeof(part_vtx)==9*sizeof(float), "invalid part_vtx size"); // remember to update WiiU_SurfInit() if this changes!

		// Don't try to draw particles if there aren't any.
		if (numParticles == 0)
		{
			return;
		}

		//WiiU_UpdateUBO3D();
		uint32_t abSize = sizeof(part_vtx)*numParticles;
		part_vtx *particleBuffer = (part_vtx*)WiiU_ABAlloc(abSize);

		// TODO: viewOrg could be in UBO
		vec3_t viewOrg;
		VectorCopy(gl3_newrefdef.vieworg, viewOrg);

		WiiU_EnableDepthWrite(false);
		WiiU_EnableBlend(true);

		WiiU_UseProgram(&gl3state.siParticle);
		for ( i = 0, p = gl3_newrefdef.particles; i < numParticles; i++, p++ )
		{
			*(int *) color = d_8to24table [ p->color & 0xFF ];
			part_vtx* cur = &particleBuffer[i];
			vec3_t offset; // between viewOrg and particle position
			VectorSubtract(viewOrg, p->origin, offset);

			VectorCopy(p->origin, cur->pos);
			cur->size = pointSize;
			cur->dist = VectorLength(offset);

			for(int j=0; j<3; ++j)  cur->color[j] = color[j]*(1.0f/255.0f);

			cur->color[3] = p->alpha;
		}

		GX2Invalidate(GX2_INVALIDATE_MODE_ATTRIBUTE_BUFFER | GX2_INVALIDATE_MODE_CPU,
			particleBuffer, abSize);
		GX2SetAttribBuffer(0, abSize, sizeof(part_vtx), particleBuffer);
		GX2DrawEx(GX2_PRIMITIVE_MODE_POINTS, numParticles, 0, 1);

		WiiU_EnableBlend(false);
		WiiU_EnableDepthWrite(true);
	}
}

static void
WiiU_DrawEntitiesOnList(void)
{
	int i;

	if (!r_drawentities->value)
	{
		return;
	}

	WiiU_ResetShadowAliasModels();

	/* draw non-transparent first */
	for (i = 0; i < gl3_newrefdef.num_entities; i++)
	{
		entity_t *currententity = &gl3_newrefdef.entities[i];

		if (currententity->flags & RF_TRANSLUCENT)
		{
			continue; /* solid */
		}

		if (currententity->flags & RF_BEAM)
		{
			WiiU_DrawBeam(currententity);
		}
		else
		{
			gl3model_t *currentmodel = currententity->model;

			if (!currentmodel)
			{
				WiiU_DrawNullModel(currententity);
				continue;
			}

			switch (currentmodel->type)
			{
				case mod_alias:
					WiiU_DrawAliasModel(currententity);
					break;
				case mod_brush:
					WiiU_DrawBrushModel(currententity, currentmodel);
					break;
				case mod_sprite:
					WiiU_DrawSpriteModel(currententity, currentmodel);
					break;
				default:
					ri.Sys_Error(ERR_DROP, "Bad modeltype");
					break;
			}
		}
	}

	/* draw transparent entities
	   we could sort these if it ever
	   becomes a problem... */
	WiiU_EnableDepthWrite(false);

	for (i = 0; i < gl3_newrefdef.num_entities; i++)
	{
		entity_t *currententity = &gl3_newrefdef.entities[i];

		if (!(currententity->flags & RF_TRANSLUCENT))
		{
			continue; /* solid */
		}

		if (currententity->flags & RF_BEAM)
		{
			WiiU_DrawBeam(currententity);
		}
		else
		{
			gl3model_t *currentmodel = currententity->model;

			if (!currentmodel)
			{
				WiiU_DrawNullModel(currententity);
				continue;
			}

			switch (currentmodel->type)
			{
				case mod_alias:
					WiiU_DrawAliasModel(currententity);
					break;
				case mod_brush:
					WiiU_DrawBrushModel(currententity, currentmodel);
					break;
				case mod_sprite:
					WiiU_DrawSpriteModel(currententity, currentmodel);
					break;
				default:
					ri.Sys_Error(ERR_DROP, "Bad modeltype");
					break;
			}
		}
	}

	WiiU_DrawAliasShadows();

	WiiU_EnableDepthWrite(true);
}

static void
SetupFrame(void)
{
	int i;
	mleaf_t *leaf;

	gl3_framecount++;

	/* build the transformation matrix for the given view angles */
	VectorCopy(gl3_newrefdef.vieworg, gl3_origin);

	AngleVectors(gl3_newrefdef.viewangles, vpn, vright, vup);

	/* current viewcluster */
	if (!(gl3_newrefdef.rdflags & RDF_NOWORLDMODEL))
	{
		if (!gl3_worldmodel)
		{
			ri.Sys_Error(ERR_DROP, "%s: bad world model", __func__);
			return;
		}

		gl3_oldviewcluster = gl3_viewcluster;
		gl3_oldviewcluster2 = gl3_viewcluster2;
		leaf = Mod_PointInLeaf(gl3_origin, gl3_worldmodel->nodes);
		gl3_viewcluster = gl3_viewcluster2 = leaf->cluster;

		/* check above and below so crossing solid water doesn't draw wrong */
		if (!leaf->contents)
		{
			/* look down a bit */
			vec3_t temp;

			VectorCopy(gl3_origin, temp);
			temp[2] -= 16;
			leaf = Mod_PointInLeaf(temp, gl3_worldmodel->nodes);

			if (!(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != gl3_viewcluster2))
			{
				gl3_viewcluster2 = leaf->cluster;
			}
		}
		else
		{
			/* look up a bit */
			vec3_t temp;

			VectorCopy(gl3_origin, temp);
			temp[2] += 16;
			leaf = Mod_PointInLeaf(temp, gl3_worldmodel->nodes);

			if (!(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != gl3_viewcluster2))
			{
				gl3_viewcluster2 = leaf->cluster;
			}
		}
	}

	for (i = 0; i < 4; i++)
	{
		v_blend[i] = gl3_newrefdef.blend[i];
	}

	c_brush_polys = 0;
	c_alias_polys = 0;

#ifdef __WIIU_TODO__
	/* clear out the portion of the screen that the NOWORLDMODEL defines */
	if (gl3_newrefdef.rdflags & RDF_NOWORLDMODEL)
	{
		glEnable(GL_SCISSOR_TEST);
		glClearColor(0.3, 0.3, 0.3, 1);
		glScissor(gl3_newrefdef.x,
				vid.height - gl3_newrefdef.height - gl3_newrefdef.y,
				gl3_newrefdef.width, gl3_newrefdef.height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(1, 0, 0.5, 0.5);
		glDisable(GL_SCISSOR_TEST);
	}
#endif
}

static void
WiiU_SetGL2D(void)
{
	int x = 0;
	int w = vid.width;
	int y = 0;
	int h = vid.height;

	WiiU_SetViewport(x, y, w, h);

	hmm_mat4 transMatr = HMM_Orthographic(0, vid.width, vid.height, 0, -99999, 99999);

	gl3state.uni2DData.transMat4 = transMatr;

	WiiU_UpdateUBO2D();
	WiiU_UpdateUBOCommon();

	WiiU_EnableDepthTest(false);
	WiiU_EnableCullFace(false);
	WiiU_EnableBlend(false);
	GX2SetPixelSampler(&gl3state.sampler2D, 0);
}

// equivalent to R_x * R_y * R_z where R_x is the trans matrix for rotating around X axis for aroundXdeg
static hmm_mat4 rotAroundAxisXYZ(float aroundXdeg, float aroundYdeg, float aroundZdeg)
{
	float alpha = HMM_ToRadians(aroundXdeg);
	float beta = HMM_ToRadians(aroundYdeg);
	float gamma = HMM_ToRadians(aroundZdeg);

	float sinA = HMM_SinF(alpha);
	float cosA = HMM_CosF(alpha);
	float sinB = HMM_SinF(beta);
	float cosB = HMM_CosF(beta);
	float sinG = HMM_SinF(gamma);
	float cosG = HMM_CosF(gamma);

	hmm_mat4 ret = {{
		{  cosB*cosG,  sinA*sinB*cosG + cosA*sinG, -cosA*sinB*cosG + sinA*sinG, 0 }, // first *column*
		{ -cosB*sinG, -sinA*sinB*sinG + cosA*cosG,  cosA*sinB*sinG + sinA*cosG, 0 },
		{  sinB,      -sinA*cosB,                   cosA*cosB,                  0 },
		{  0,          0,                           0,                          1 }
	}};

	return ret;
}

// equivalent to R_MYgluPerspective() but returning a matrix instead of setting internal OpenGL state
hmm_mat4
WiiU_MYgluPerspective(float fovy, float aspect, float zNear, float zFar)
{
	// calculation of left, right, bottom, top is from R_MYgluPerspective() of old gl backend
	// which seems to be slightly different from the real gluPerspective()
	// and thus also from HMM_Perspective()
	float left, right, bottom, top;
	float A, B, C, D;

	top = zNear * tan(fovy * M_PI / 360.0);
	bottom = -top;

	left = bottom * aspect;
	right = top * aspect;

	// the following emulates glFrustum(left, right, bottom, top, zNear, zFar)
	// see https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glFrustum.xml
	//  or http://docs.gl/gl2/glFrustum#description (looks better in non-Firefox browsers)
	A = (right+left)/(right-left);
	B = (top+bottom)/(top-bottom);
	C = -(zFar+zNear)/(zFar-zNear);
	D = -(2.0*zFar*zNear)/(zFar-zNear);

	hmm_mat4 ret = {{
		{ (2.0*zNear)/(right-left), 0, 0, 0 }, // first *column*
		{ 0, (2.0*zNear)/(top-bottom), 0, 0 },
		{ A, B, C, -1.0 },
		{ 0, 0, D, 0 }
	}};

	return ret;
}

static void WiiU_Clear(void);

static void
SetupGL(void)
{
	int x, x2, y2, y, w, h;

	/* set up viewport */
	x = floor(gl3_newrefdef.x * vid.width / vid.width);
	x2 = ceil((gl3_newrefdef.x + gl3_newrefdef.width) * vid.width / vid.width);
	y = floor(vid.height - gl3_newrefdef.y * vid.height / vid.height);
	y2 = ceil(vid.height - (gl3_newrefdef.y + gl3_newrefdef.height) * vid.height / vid.height);

	w = x2 - x;
	h = y - y2;

#ifdef __WIIU_TODO__
	// set up the FBO accordingly, but only if actually rendering the world
	// (=> don't use FBO when rendering the playermodel in the player menu)
	// also, only do this when under water, because this has a noticeable overhead on some systems
	if (gl3_usefbo->value && gl3state.ppFBO != 0
		&& (gl3_newrefdef.rdflags & (RDF_NOWORLDMODEL|RDF_UNDERWATER)) == RDF_UNDERWATER)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, gl3state.ppFBO);
		gl3state.ppFBObound = true;
		if(gl3state.ppFBtex == 0)
		{
			gl3state.ppFBtexWidth = -1; // make sure we generate the texture storage below
			glGenTextures(1, &gl3state.ppFBtex);
		}

		if(gl3state.ppFBrbo == 0)
		{
			gl3state.ppFBtexWidth = -1; // make sure we generate the RBO storage below
			glGenRenderbuffers(1, &gl3state.ppFBrbo);
		}

		// even if the FBO already has a texture and RBO, the viewport size
		// might have changed so they need to be regenerated with the correct sizes
		if(gl3state.ppFBtexWidth != w || gl3state.ppFBtexHeight != h)
		{
			gl3state.ppFBtexWidth = w;
			gl3state.ppFBtexHeight = h;
			WiiU_Bind(gl3state.ppFBtex);
			// create texture for FBO with size of the viewport
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			WiiU_Bind(0);
			// attach it to currently bound FBO
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gl3state.ppFBtex, 0);

			// also create a renderbuffer object so the FBO has a stencil- and depth-buffer
			glBindRenderbuffer(GL_RENDERBUFFER, gl3state.ppFBrbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			// attach it to the FBO
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
			                          GL_RENDERBUFFER, gl3state.ppFBrbo);

			int fbState = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if(fbState != GL_FRAMEBUFFER_COMPLETE)
			{
				R_Printf(PRINT_ALL, "GL3 SetupGL(): WARNING: FBO is not complete, status = 0x%x\n", fbState);
				gl3state.ppFBtexWidth = -1; // to try again next frame; TODO: maybe give up?
				gl3state.ppFBObound = false;
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
		}

		WiiU_Clear(); // clear the FBO that's bound now

		WiiU_SetViewport(0, 0, w, h); // this will be moved to the center later, so no x/y offset
	}
	else // rendering directly (not to FBO for postprocessing)
#endif
	{
#ifdef USE_FBO
		WiiU_FBO_Activate(&gl3state.worldFBO);
		WiiU_Clear(); // clear the FBO that's bound now
#else
		WiiU_SetViewport(x, y2, w, h);
#endif
	}

	/* set up projection matrix (eye coordinates -> clip coordinates) */
	{
		float screenaspect = (float)gl3_newrefdef.width / gl3_newrefdef.height;
		float dist = (r_farsee->value == 0) ? 4096.0f : 8192.0f;
		gl3state.projMat3D = WiiU_MYgluPerspective(gl3_newrefdef.fov_y, screenaspect, 4, dist);
	}

	WiiU_CullFront();

	/* set up view matrix (world coordinates -> eye coordinates) */
	{
		// first put Z axis going up
		hmm_mat4 viewMat = {{
			{  0, 0, -1, 0 }, // first *column* (the matrix is column-major)
			{ -1, 0,  0, 0 },
			{  0, 1,  0, 0 },
			{  0, 0,  0, 1 }
		}};

		// now rotate by view angles
		hmm_mat4 rotMat = rotAroundAxisXYZ(-gl3_newrefdef.viewangles[2], -gl3_newrefdef.viewangles[0], -gl3_newrefdef.viewangles[1]);

		viewMat = HMM_MultiplyMat4( viewMat, rotMat );

		// .. and apply translation for current position
		hmm_vec3 trans = HMM_Vec3(-gl3_newrefdef.vieworg[0], -gl3_newrefdef.vieworg[1], -gl3_newrefdef.vieworg[2]);
		viewMat = HMM_MultiplyMat4( viewMat, HMM_Translate(trans) );

		gl3state.viewMat3D = viewMat;
	}

	// just use one projection-view-matrix (premultiplied here)
	// so we have one less mat4 multiplication in the 3D shaders
	gl3state.uni3DData.transProjViewMat4 = HMM_MultiplyMat4(gl3state.projMat3D, gl3state.viewMat3D);

	gl3state.uni3DData.transModelMat4 = gl3_identityMat4;

	gl3state.uni3DData.time = gl3_newrefdef.time;

	WiiU_UpdateUBO3D();

	/* set drawing parms */
	if (r_cull->value)
	{
		WiiU_EnableCullFace(true);
	}
	else
	{
		WiiU_EnableCullFace(false);
	}

	WiiU_EnableDepthTest(true);
}

extern int c_visible_lightmaps, c_visible_textures;

/*
 * gl3_newrefdef must be set before the first call
 */
static void
WiiU_RenderView(refdef_t *fd)
{
	if (r_norefresh->value)
	{
		return;
	}

	gl3_newrefdef = *fd;

	if (!gl3_worldmodel && !(gl3_newrefdef.rdflags & RDF_NOWORLDMODEL))
	{
		ri.Sys_Error(ERR_DROP, "R_RenderView: NULL worldmodel");
	}

	if (r_speeds->value)
	{
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	GX2SetPixelSampler(&gl3state.sampler3D, 0);

	WiiU_PushDlights();

	SetupFrame();

	R_SetFrustum(vup, vpn, vright, gl3_origin,
		gl3_newrefdef.fov_x, gl3_newrefdef.fov_y, frustum);

	SetupGL();

	WiiU_MarkLeaves(); /* done here so we know if we're in water */

	WiiU_DrawWorld();

	WiiU_DrawEntitiesOnList();

	// kick the silly gl1_flashblend poly lights
	// WiiU_RenderDlights();

	WiiU_DrawParticles();

	WiiU_DrawAlphaSurfaces();

	// Note: R_Flash() is now WiiU_Draw_Flash() and called from WiiU_RenderFrame()

	if (r_speeds->value)
	{
		R_Printf(PRINT_ALL, "%4i wpoly %4i epoly %i tex %i lmaps\n",
				c_brush_polys, c_alias_polys, c_visible_textures,
				c_visible_lightmaps);
	}
}

static void
WiiU_SetLightLevel(entity_t *currententity)
{
	vec3_t shadelight = {0};

	if (gl3_newrefdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	/* save off light value for server to look at */
	WiiU_LightPoint(currententity, gl3_newrefdef.vieworg, shadelight);

	/* pick the greatest component, which should be the
	 * same as the mono value returned by software */
	if (shadelight[0] > shadelight[1])
	{
		if (shadelight[0] > shadelight[2])
		{
			r_lightlevel->value = 150 * shadelight[0];
		}
		else
		{
			r_lightlevel->value = 150 * shadelight[2];
		}
	}
	else
	{
		if (shadelight[1] > shadelight[2])
		{
			r_lightlevel->value = 150 * shadelight[1];
		}
		else
		{
			r_lightlevel->value = 150 * shadelight[2];
		}
	}
}

static void
WiiU_RenderFrame(refdef_t *fd)
{
	if(!WHBProcIsRunning())
		ri.Cmd_ExecuteText(0, "quit");

	GX2SetPointLimits(0,100);
	WiiU_RenderView(fd);
	WiiU_SetLightLevel(NULL);
#ifdef __WIIU_TODO__
	qboolean usedFBO = gl3state.ppFBObound; // if it was/is used this frame
	if(usedFBO)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0); // now render to default framebuffer
		gl3state.ppFBObound = false;
	}
#endif
#ifdef USE_FBO
 	GX2Texture *fboTex = WiiU_FBO_ContentDone(&gl3state.worldFBO);
	WHBGfxBeginRenderDRC();

	WiiU_SetGL2D();

	int x = (vid.width - gl3_newrefdef.width)/2;
	int y = (vid.height - gl3_newrefdef.height)/2;

	//WiiU_FBO_Draw();
	//WHBGfxFinishRenderDRC();
	//WHBGfxBeginRenderTV();
	// if we're actually drawing the world and using an FBO, render the FBO's texture
	WiiU_DrawFrameBufferObject(x, y, gl3_newrefdef.width, gl3_newrefdef.height, fboTex, v_blend);
#else
	WiiU_SetGL2D();

	int x = (vid.width - gl3_newrefdef.width)/2;
	int y = (vid.height - gl3_newrefdef.height)/2;

	if(v_blend[3] != 0.0f)
	{
		WiiU_Draw_Flash(v_blend, x, y, gl3_newrefdef.width, gl3_newrefdef.height);
	}
#endif
}


static void
WiiU_Clear(void)
{
	GX2ColorBuffer *colourBuffer;
	GX2DepthBuffer *depthBuffer;
	// Check whether the stencil buffer needs clearing, and do so if need be.
	//GLbitfield stencilFlags = 0;
	if(gl3state.activeFBO == NULL)
	{
		colourBuffer = WHBGfxGetTVColourBuffer();
		depthBuffer = WHBGfxGetTVDepthBuffer();
	}
	else
	{
		colourBuffer = &gl3state.activeFBO->colourBuffer;
		depthBuffer = &gl3state.activeFBO->depth;
	}

	if (r_clear->value)
	{
		GX2ClearColor(colourBuffer, 1.0f, 0.0f, 0.5f, 1.0f);
		GX2ClearDepthStencilEx(depthBuffer, depthBuffer->depthClear, depthBuffer->stencilClear, GX2_CLEAR_FLAGS_DEPTH | GX2_CLEAR_FLAGS_STENCIL);
		// glClear(GL_COLOR_BUFFER_BIT | stencilFlags | GL_DEPTH_BUFFER_BIT);
		//WHBGfxClearColor(1.0f, 0.0f, 0.5f, 1.0f);
	}
	else
	{
		//GX2ClearColor(colourBuffer, 0.0f, (randk() %128) / 255.0f, 0.5f, 1.0f);
		GX2ClearDepthStencilEx(depthBuffer, depthBuffer->depthClear, depthBuffer->stencilClear, GX2_CLEAR_FLAGS_DEPTH | GX2_CLEAR_FLAGS_STENCIL);
		//glClear(GL_DEPTH_BUFFER_BIT | stencilFlags);
		//WHBGfxClearColor(0.0f, 0.5f, 0.5f, 1.0f);
	}
	//WHBGfxClearColor(0.0f, (randk() %128) / 255.0f, 0.5f, 1.0f);
	WHBGfxBeginRenderTV();
	gl3depthmin = 0;
	gl3depthmax = 1;

	WiiU_SetDepthRange(gl3depthmin, gl3depthmax);

	if (gl_zfix->value)
	{
		if (gl3depthmax > gl3depthmin)
		{
			WiiU_PolygonOffset(0.05, 1);
		}
		else
		{
			WiiU_PolygonOffset(-0.05, -1);
		}
	}

#ifdef __WIIU_TODO__
	/* stencilbuffer shadows */
	if (gl_shadows->value && gl3config.stencil)
	{
		glClearStencil(1);
		glClear(GL_STENCIL_BUFFER_BIT);
	}
#endif
}

void
WiiU_BeginFrame(float camera_separation)
{
	WHBGfxBeginRender();
	WHBGfxBeginRenderTV();

	GX2SetShaderMode(GX2_SHADER_MODE_UNIFORM_BLOCK);
    GX2SetDepthOnlyControl(false, false, GX2_COMPARE_FUNC_ALWAYS);
    GX2SetColorControl(GX2_LOGIC_OP_COPY, 0xFF, FALSE, TRUE);
    GX2SetBlendControl(GX2_RENDER_TARGET_0,
                       GX2_BLEND_MODE_SRC_ALPHA, GX2_BLEND_MODE_INV_SRC_ALPHA, GX2_BLEND_COMBINE_MODE_ADD,
                       TRUE,
                       GX2_BLEND_MODE_SRC_ALPHA, GX2_BLEND_MODE_INV_SRC_ALPHA, GX2_BLEND_COMBINE_MODE_ADD);
    // GX2SetBlendControl(GX2_RENDER_TARGET_0,
    //                    GX2_BLEND_MODE_ONE, GX2_BLEND_MODE_ZERO, GX2_BLEND_COMBINE_MODE_ADD,
    //                    false,
    //                    GX2_BLEND_MODE_SRC_ALPHA, GX2_BLEND_MODE_INV_SRC_ALPHA, GX2_BLEND_COMBINE_MODE_ADD);

    GX2SetCullOnlyControl(GX2_FRONT_FACE_CCW, false, false);

	if (vid_gamma->modified || gl3_intensity->modified || gl3_intensity_2D->modified)
	{
		vid_gamma->modified = false;
		gl3_intensity->modified = false;
		gl3_intensity_2D->modified = false;

		gl3state.uniCommonData.gamma = 1.0f/vid_gamma->value;
		gl3state.uniCommonData.intensity = gl3_intensity->value;
		gl3state.uniCommonData.intensity2D = gl3_intensity_2D->value;
		WiiU_UpdateUBOCommon();
	}

	// in GL3, overbrightbits can have any positive value
	if (gl3_overbrightbits->modified)
	{
		gl3_overbrightbits->modified = false;

		if(gl3_overbrightbits->value < 0.0f)
		{
			ri.Cvar_Set("gl3_overbrightbits", "0");
		}

		gl3state.uni3DData.overbrightbits = (gl3_overbrightbits->value <= 0.0f) ? 1.0f : gl3_overbrightbits->value;
		WiiU_UpdateUBO3D();
	}

	if(gl3_particle_fade_factor->modified)
	{
		gl3_particle_fade_factor->modified = false;
		gl3state.uni3DData.particleFadeFactor = gl3_particle_fade_factor->value;
		WiiU_UpdateUBO3D();
	}

	if(gl3_particle_square->modified || gl3_colorlight->modified)
	{
		gl3_particle_square->modified = false;
		gl3_colorlight->modified = false;
		WiiU_RecreateShaders();
	}


	/* go into 2D mode */

	WiiU_SetGL2D();

	/* texturemode stuff */
	if (gl_texturemode->modified || (gl3config.anisotropic && gl_anisotropic->modified)
	    || gl_nolerp_list->modified || r_lerp_list->modified
		|| r_2D_unfiltered->modified || r_videos_unfiltered->modified)
	{
		WiiU_TextureMode(gl_texturemode->string);
		gl_texturemode->modified = false;
		gl_anisotropic->modified = false;
		gl_nolerp_list->modified = false;
		r_lerp_list->modified = false;
		r_2D_unfiltered->modified = false;
		r_videos_unfiltered->modified = false;
	}

	if (r_vsync->modified)
	{
		r_vsync->modified = false;
		WiiU_SetVsync();
	}

	/* clear screen if desired */
	WiiU_Clear();
}

static void
WiiU_SetPalette(const unsigned char *palette)
{
	int i;
	byte *rp = (byte *)gl3_rawpalette;

	if (palette)
	{
		for (i = 0; i < 256; i++)
		{
			rp[i * 4 + 0] = palette[i * 3 + 0];
			rp[i * 4 + 1] = palette[i * 3 + 1];
			rp[i * 4 + 2] = palette[i * 3 + 2];
			rp[i * 4 + 3] = 0xff;
		}
	}
	else
	{
		for (i = 0; i < 256; i++)
		{
			rp[i * 4 + 0] = LittleLong(d_8to24table[i]) & 0xff;
			rp[i * 4 + 1] = (LittleLong(d_8to24table[i]) >> 8) & 0xff;
			rp[i * 4 + 2] = (LittleLong(d_8to24table[i]) >> 16) & 0xff;
			rp[i * 4 + 3] = 0xff;
		}
	}

#ifdef __WIIU_TODO__
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(1, 0, 0.5, 0.5);
#endif
}

/*
=====================
WiiU_EndWorldRenderpass
=====================
*/
static qboolean
WiiU_EndWorldRenderpass( void )
{
	return true;
}




Q2_DLL_EXPORTED refexport_t
GetRefAPIWiiU(refimport_t imp)
{
	refexport_t re = {0};
	ri = imp;

	re.api_version = API_VERSION;
	re.framework_version = WiiU_GetSDLVersion();

	re.Init = WiiU_Init;
	re.Shutdown = WiiU_Shutdown;
	re.PrepareForWindow = WiiU_PrepareForWindow;
	re.InitContext = WiiU_InitContext;
	re.GetDrawableSize = WiiU_GetDrawableSize;
	re.ShutdownContext = WiiU_ShutdownContext;
	re.IsVSyncActive = WiiU_IsVsyncActive;

	re.BeginRegistration = WiiU_BeginRegistration;
	re.RegisterModel = WiiU_RegisterModel;
	re.RegisterSkin = WiiU_RegisterSkin;

	re.SetSky = WiiU_SetSky;
	re.EndRegistration = WiiU_EndRegistration;

	re.RenderFrame = WiiU_RenderFrame;

	re.DrawFindPic = WiiU_Draw_FindPic;
	re.DrawGetPicSize = WiiU_Draw_GetPicSize;

	re.DrawPicScaled = WiiU_Draw_PicScaled;
	re.DrawStretchPic = WiiU_Draw_StretchPic;

	re.DrawCharScaled = WiiU_Draw_CharScaled;
	re.DrawTileClear = WiiU_Draw_TileClear;
	re.DrawFill = WiiU_Draw_Fill;
	re.DrawFadeScreen = WiiU_Draw_FadeScreen;

	re.DrawStretchRaw = WiiU_Draw_StretchRaw;
	re.SetPalette = WiiU_SetPalette;

	re.BeginFrame = WiiU_BeginFrame;
	re.EndWorldRenderpass = WiiU_EndWorldRenderpass;
	re.EndFrame = WiiU_EndFrame;

    // Tell the client that we're unsing the
	// new renderer restart API.
    ri.Vid_RequestRestart(RESTART_NO);

	return re;
}

void R_Printf(int level, const char* msg, ...)
{
	va_list argptr;
	va_start(argptr, msg);
	ri.Com_VPrintf(level, msg, argptr);
	va_end(argptr);
}
