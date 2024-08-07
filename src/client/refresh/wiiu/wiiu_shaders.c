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
 * OpenGL3 refresher: Handling shaders
 *
 * =======================================================================
 */

#include "header/local.h"
#include <malloc.h>

//#define SHADER_DEBUG

void DumpShaderInfo(const char *name, WHBGfxShaderGroup *shader)
{
#ifdef SHADER_DEBUG
	WHBLogPrintf("Shader: %s", name);
	WHBLogPrintf("Found: %d samplers", shader->pixelShader->samplerVarCount);

	for (uint32_t s = 0; s < shader->pixelShader->samplerVarCount; ++s) {
		GX2SamplerVar* ss = &shader->pixelShader->samplerVars[s];
		WHBLogPrintf("  Sampler: %d, name: %s, loc: %d, type: %d", s, ss->name, ss->location, ss->type);
	}

	WHBLogPrintf("Found: %d attribs", shader->vertexShader->attribVarCount);
	for (uint32_t s = 0; s < shader->vertexShader->attribVarCount; ++s) {
		GX2AttribVar* ss = &shader->vertexShader->attribVars[s];
		WHBLogPrintf("  Attrib: %d, name: %s, loc: %d, type: %d", s, ss->name, ss->location, ss->type);
	}

	WHBLogPrintf("Found: %d VS uniform blocks", shader->vertexShader->uniformBlockCount);
	for (uint32_t s = 0; s < shader->vertexShader->uniformBlockCount; ++s) {
		GX2UniformBlock* ss = &shader->vertexShader->uniformBlocks[s];
		WHBLogPrintf("  Uniform: %d, name: %s, size: %d, offset: %d", s, ss->name, ss->size, ss->offset);
	}

	WHBLogPrintf("Found: %d PS uniform blocks", shader->pixelShader->uniformBlockCount);
	for (uint32_t s = 0; s < shader->pixelShader->uniformBlockCount; ++s) {
		GX2UniformBlock* ss = &shader->pixelShader->uniformBlocks[s];
		WHBLogPrintf("  Uniform: %d, name: %s, size: %d, offset: %d", s, ss->name, ss->size, ss->offset);
	}

	WHBLogPrintf("Found: %d VS uniforms", shader->vertexShader->uniformVarCount);
	for (uint32_t s = 0; s < shader->vertexShader->uniformVarCount; ++s) {
		GX2UniformVar* ss = &shader->vertexShader->uniformVars[s];
		WHBLogPrintf("  Uniform: %d, name: %s, type: %d, block: %d, count: %d, offset: %d", s, ss->name, ss->type, ss->block, ss->count, ss->offset);
	}

	WHBLogPrintf("Found: %d PS uniforms", shader->pixelShader->uniformVarCount);
	for (uint32_t s = 0; s < shader->pixelShader->uniformVarCount; ++s) {
		GX2UniformVar* ss = &shader->pixelShader->uniformVars[s];
		WHBLogPrintf("  Uniform: %d, name: %s, type: %d, block: %d, count: %d, offset: %d", s, ss->name, ss->type, ss->block, ss->count, ss->offset);
	}
#endif
}

qboolean LoadShader(const char* name, WHBGfxShaderGroup* target)
{
#ifdef SHADER_DEBUG
	WHBLogPrintf("loading \"%s\" shader...", name);
#endif

	char fullName[128];
	sprintf(fullName, "shaders/%s.gsh", name);

	byte* gshFileData = NULL;
	ri.FS_LoadFile(fullName, (void **)&gshFileData);

	if(gshFileData == NULL)
		return false;

	if (!WHBGfxLoadGFDShaderGroup(target, 0, gshFileData)) {
		ri.FS_FreeFile(gshFileData);
		return false;
	}
	ri.FS_FreeFile(gshFileData);

	DumpShaderInfo(name, target);

	return true;
}


enum {
	WiiU_BINDINGPOINT_UNICOMMON,
	WiiU_BINDINGPOINT_UNI2D,
	WiiU_BINDINGPOINT_UNI3D,
	WiiU_BINDINGPOINT_UNILIGHTS
};

static qboolean
initShader2D(gl3ShaderInfo_t* shaderInfo, const char* name)
{
	if(!LoadShader(name, &shaderInfo->shaderGroup))
	{
		return false;
	}
	shaderInfo->name = name;

	WHBGfxInitShaderAttribute(&shaderInfo->shaderGroup, "position", 0, 0 * sizeof(float), GX2_ATTRIB_FORMAT_FLOAT_32_32);
	WHBGfxInitShaderAttribute(&shaderInfo->shaderGroup, "texCoord", 0, 2 * sizeof(float), GX2_ATTRIB_FORMAT_FLOAT_32_32);

    WHBGfxInitFetchShader(&shaderInfo->shaderGroup);

	return true;
}

static qboolean
initShader3D(gl3ShaderInfo_t* shaderInfo, const char* name)
{
	if(!LoadShader(name, &shaderInfo->shaderGroup))
	{
		return false;
	}
	shaderInfo->name = name;

	WHBGfxInitShaderAttribute(&shaderInfo->shaderGroup, "position",   0, offsetof(gl3_3D_vtx_t, pos), GX2_ATTRIB_FORMAT_FLOAT_32_32_32);
	WHBGfxInitShaderAttribute(&shaderInfo->shaderGroup, "texCoord",   0, offsetof(gl3_3D_vtx_t, texCoord), GX2_ATTRIB_FORMAT_FLOAT_32_32);
	WHBGfxInitShaderAttribute(&shaderInfo->shaderGroup, "lmTexCoord", 0, offsetof(gl3_3D_vtx_t, lmTexCoord), GX2_ATTRIB_FORMAT_FLOAT_32_32);
	WHBGfxInitShaderAttribute(&shaderInfo->shaderGroup, "vertColor",  0, offsetof(gl3_alias_vtx_t, color), GX2_ATTRIB_FORMAT_FLOAT_32_32_32_32);
	WHBGfxInitShaderAttribute(&shaderInfo->shaderGroup, "normal",     0, offsetof(gl3_3D_vtx_t, normal), GX2_ATTRIB_FORMAT_FLOAT_32_32_32);
	WHBGfxInitShaderAttribute(&shaderInfo->shaderGroup, "lightFlags", 0, offsetof(gl3_3D_vtx_t, lightFlags), GX2_ATTRIB_FLAG_INTEGER | GX2_ATTRIB_TYPE_32);

    WHBGfxInitFetchShader(&shaderInfo->shaderGroup);

	return true;
}

static void initUBOs(void)
{
	gl3state.uniCommonData.gamma = 1.0f/vid_gamma->value;
	gl3state.uniCommonData.intensity = gl3_intensity->value;
	gl3state.uniCommonData.intensity2D = gl3_intensity_2D->value;
	gl3state.uniCommonData.color = HMM_Vec4(1, 1, 1, 1);

	WiiU_UpdateUBOCommon();

	// the matrix will be set to something more useful later, before being used
	gl3state.uni2DData.transMat4 = HMM_Mat4();

	WiiU_UpdateUBO2D();

	// the matrices will be set to something more useful later, before being used
	gl3state.uni3DData.transProjViewMat4 = HMM_Mat4();
	gl3state.uni3DData.transModelMat4 = gl3_identityMat4;
	gl3state.uni3DData.scroll = 0.0f;
	gl3state.uni3DData.time = 0.0f;
	gl3state.uni3DData.alpha = 1.0f;
	// gl3_overbrightbits 0 means "no scaling" which is equivalent to multiplying with 1
	gl3state.uni3DData.overbrightbits = (gl3_overbrightbits->value <= 0.0f) ? 1.0f : gl3_overbrightbits->value;
	gl3state.uni3DData.particleFadeFactor = gl3_particle_fade_factor->value;
	gl3state.uni3DData.lightScaleForTurb = 1.0f;

	WiiU_UpdateUBO3D();

	WiiU_UpdateUBOLights();

	//gl3state.currentUBO = gl3state.uniLightsUBO;
}

static qboolean createShaders(void)
{
	if(!initShader2D(&gl3state.si2D, "2d"))
	{
		R_Printf(PRINT_ALL, "WARNING: Failed to create shader program for textured 2D rendering!\n");
		return false;
	}
	if(!initShader2D(&gl3state.si2Dcolor, "2d_color"))
	{
		R_Printf(PRINT_ALL, "WARNING: Failed to create shader program for color-only 2D rendering!\n");
		return false;
	}

	if(!initShader2D(&gl3state.si2DpostProcess, "2d_postprocess"))
	{
		R_Printf(PRINT_ALL, "WARNING: Failed to create shader program to render framebuffer object!\n");
		return false;
	}
	if(!initShader2D(&gl3state.si2DpostProcessWater, "2d_postprocess_water"))
	{
		R_Printf(PRINT_ALL, "WARNING: Failed to create shader program to render framebuffer object under water!\n");
		return false;
	}

#ifdef __WIIU_TODO__
	const char* lightmappedFrag = (gl3_colorlight->value == 0.0f)
	                               ? fragmentSrc3DlmNoColor : fragmentSrc3Dlm;

	if(!initShader3D(&gl3state.si3Dlm, vertexSrc3Dlm, lightmappedFrag))
	{
		R_Printf(PRINT_ALL, "WARNING: Failed to create shader program for textured 3D rendering with lightmap!\n");
		return false;
	}
#endif
	if(!initShader3D(&gl3state.si3Dtrans, "3d_trans"))
	{
		R_Printf(PRINT_ALL, "WARNING: Failed to create shader program for rendering translucent 3D things!\n");
		return false;
	}
	if(!initShader3D(&gl3state.si3DcolorOnly, "3d_color"))
	{
		R_Printf(PRINT_ALL, "WARNING: Failed to create shader program for flat-colored 3D rendering!\n");
		return false;
	}
	if(!initShader3D(&gl3state.si3Dlm, "3d_lm"))
	{
		R_Printf(PRINT_ALL, "WARNING: Failed to create shader program for blending 3D lightmaps rendering!\n");
		return false;
	}
	if(!initShader3D(&gl3state.si3Dturb, "3d_water"))
	{
		R_Printf(PRINT_ALL, "WARNING: Failed to create shader program for water rendering!\n");
		return false;
	}
	if(!initShader3D(&gl3state.si3DlmFlow, "3d_lm_flow"))
	{
		R_Printf(PRINT_ALL, "WARNING: Failed to create shader program for scrolling textured 3D rendering with lightmap!\n");
		return false;
	}
	if(!initShader3D(&gl3state.si3DtransFlow, "3d_trans_flow"))
	{
		R_Printf(PRINT_ALL, "WARNING: Failed to create shader program for scrolling textured translucent 3D rendering!\n");
		return false;
	}
	if(!initShader3D(&gl3state.si3Dsky, "3d_sky"))
	{
		R_Printf(PRINT_ALL, "WARNING: Failed to create shader program for sky rendering!\n");
		return false;
	}
	if(!initShader3D(&gl3state.si3Dsprite, "3d_sprite"))
	{
		R_Printf(PRINT_ALL, "WARNING: Failed to create shader program for sprite rendering!\n");
		return false;
	}
	if(!initShader3D(&gl3state.si3DspriteAlpha, "3d_sprite_alpha"))
	{
		R_Printf(PRINT_ALL, "WARNING: Failed to create shader program for alpha-tested sprite rendering!\n");
		return false;
	}
	if(!initShader3D(&gl3state.si3Dalias, "alias"))
	{
		R_Printf(PRINT_ALL, "WARNING: Failed to create shader program for rendering textured models!\n");
		return false;
	}
	if(!initShader3D(&gl3state.si3DaliasColor, "alias_color"))
	{
		R_Printf(PRINT_ALL, "WARNING: Failed to create shader program for rendering flat-colored models!\n");
		return false;
	}
#ifdef __WIIU_TODO__

	const char* particleFrag = fragmentSrcParticles;
	if(gl3_particle_square->value != 0.0f)
	{
		particleFrag = fragmentSrcParticlesSquare;
	}
#endif

	if(!initShader3D(&gl3state.siParticle, "particle"))
	{
		R_Printf(PRINT_ALL, "WARNING: Failed to create shader program for rendering particles!\n");
		return false;
	}

	if(!initShader2D(&gl3state.siDrcCopy, "drc_copy"))
	{
		R_Printf(PRINT_ALL, "WARNING: Failed to create shader program for copying to DRC!\n");
		return false;
	}

	gl3state.currentShaderProgram = 0;

	return true;
}

qboolean WiiU_InitShaders(void)
{
	initUBOs();

	return createShaders();
}

static void deleteShaders(void)
{
	gl3state.currentShaderProgram = NULL;
	WHBGfxFreeShaderGroup(&gl3state.si2D.shaderGroup);
	WHBGfxFreeShaderGroup(&gl3state.si2Dcolor.shaderGroup);
	WHBGfxFreeShaderGroup(&gl3state.si3Dlm.shaderGroup);
	WHBGfxFreeShaderGroup(&gl3state.si3Dtrans.shaderGroup);
	WHBGfxFreeShaderGroup(&gl3state.si3DcolorOnly.shaderGroup);
	WHBGfxFreeShaderGroup(&gl3state.si3Dturb.shaderGroup);
	WHBGfxFreeShaderGroup(&gl3state.si3DlmFlow.shaderGroup);
	WHBGfxFreeShaderGroup(&gl3state.si3DtransFlow.shaderGroup);
	WHBGfxFreeShaderGroup(&gl3state.si3Dsky.shaderGroup);
	WHBGfxFreeShaderGroup(&gl3state.si3Dsprite.shaderGroup);
	WHBGfxFreeShaderGroup(&gl3state.si3DspriteAlpha.shaderGroup);
	WHBGfxFreeShaderGroup(&gl3state.si3Dalias.shaderGroup);
	WHBGfxFreeShaderGroup(&gl3state.si3DaliasColor.shaderGroup);
	WHBGfxFreeShaderGroup(&gl3state.siParticle.shaderGroup);
	WHBGfxFreeShaderGroup(&gl3state.siDrcCopy.shaderGroup);
}

void WiiU_ShutdownShaders(void)
{
	deleteShaders();
}

qboolean WiiU_RecreateShaders(void)
{
	// delete and recreate the existing shaders (but not the UBOs)
	deleteShaders();
	return createShaders();
}

#define GFD_SWAP_8_IN_32(x) ((((x) >> 24) & 0xff) | (((x) >> 8) & 0xff00) | (((x) << 8) & 0xff0000) | (((x) << 24) & 0xff000000))
float endianSwap(float fnum) {
	uint32_t i;
	float ret;
	memcpy(&i, &fnum, sizeof(float));
	const int si = GFD_SWAP_8_IN_32(i);
	memcpy(&ret, &si, sizeof(float));

	return ret;
}

static inline void endianSwap2(float* dst, const float* src) {
	uint8_t* dstP=(uint8_t*)dst;
	uint8_t* srcP=(uint8_t*)src;

	dstP[0] = srcP[3];
	dstP[1] = srcP[2];
	dstP[2] = srcP[1];
	dstP[3] = srcP[0];
}

void WiiU_UpdateUBOCommon(void)
{
	float* ub = (float*)WiiU_UBAlloc(sizeof(gl3state.uniCommonData));

	endianSwap2(&ub[0], &gl3state.uniCommonData.gamma);
	endianSwap2(&ub[1], &gl3state.uniCommonData.intensity);
	endianSwap2(&ub[2], &gl3state.uniCommonData.intensity2D);
	endianSwap2(&ub[3], &gl3state.uniCommonData._padding);
	endianSwap2(&ub[4], &gl3state.uniCommonData.color.R);
	endianSwap2(&ub[5], &gl3state.uniCommonData.color.G);
	endianSwap2(&ub[6], &gl3state.uniCommonData.color.B);
	endianSwap2(&ub[7], &gl3state.uniCommonData.color.A);

	// 0 --> uniCommon --> PS:0
    GX2Invalidate(GX2_INVALIDATE_MODE_UNIFORM_BLOCK | GX2_INVALIDATE_MODE_CPU,
		ub, 0x100);
	GX2SetPixelUniformBlock(0, 0x100, ub);
}

void WiiU_UpdateUBO2D(void)
{
	float* ub = (float*)WiiU_UBAlloc(sizeof(gl3state.uni2DData));
	endianSwap2(&ub[0],  &gl3state.uni2DData.transMat4.Elements[0][0]);
	endianSwap2(&ub[1],  &gl3state.uni2DData.transMat4.Elements[0][1]);
	endianSwap2(&ub[2],  &gl3state.uni2DData.transMat4.Elements[0][2]);
	endianSwap2(&ub[3],  &gl3state.uni2DData.transMat4.Elements[0][3]);
	endianSwap2(&ub[4],  &gl3state.uni2DData.transMat4.Elements[1][0]);
	endianSwap2(&ub[5],  &gl3state.uni2DData.transMat4.Elements[1][1]);
	endianSwap2(&ub[6],  &gl3state.uni2DData.transMat4.Elements[1][2]);
	endianSwap2(&ub[7],  &gl3state.uni2DData.transMat4.Elements[1][3]);
	endianSwap2(&ub[8],  &gl3state.uni2DData.transMat4.Elements[2][0]);
	endianSwap2(&ub[9],  &gl3state.uni2DData.transMat4.Elements[2][1]);
	endianSwap2(&ub[10], &gl3state.uni2DData.transMat4.Elements[2][2]);
	endianSwap2(&ub[11], &gl3state.uni2DData.transMat4.Elements[2][3]);
	endianSwap2(&ub[12], &gl3state.uni2DData.transMat4.Elements[3][0]);
	endianSwap2(&ub[13], &gl3state.uni2DData.transMat4.Elements[3][1]);
	endianSwap2(&ub[14], &gl3state.uni2DData.transMat4.Elements[3][2]);
	endianSwap2(&ub[15], &gl3state.uni2DData.transMat4.Elements[3][3]);

	// 1 --> uni2D --> VS:0
    GX2Invalidate(GX2_INVALIDATE_MODE_UNIFORM_BLOCK | GX2_INVALIDATE_MODE_CPU,
		ub, 0x100);
	GX2SetVertexUniformBlock(0, 0x100, ub);
}

void WiiU_UpdateUBO3D(void)
{
	float* ub = (float*)WiiU_UBAlloc(sizeof(gl3state.uni3DData));

	endianSwap2(&ub[0],  &gl3state.uni3DData.transProjViewMat4.Elements[0][0]);
	endianSwap2(&ub[1],  &gl3state.uni3DData.transProjViewMat4.Elements[0][1]);
	endianSwap2(&ub[2],  &gl3state.uni3DData.transProjViewMat4.Elements[0][2]);
	endianSwap2(&ub[3],  &gl3state.uni3DData.transProjViewMat4.Elements[0][3]);
	endianSwap2(&ub[4],  &gl3state.uni3DData.transProjViewMat4.Elements[1][0]);
	endianSwap2(&ub[5],  &gl3state.uni3DData.transProjViewMat4.Elements[1][1]);
	endianSwap2(&ub[6],  &gl3state.uni3DData.transProjViewMat4.Elements[1][2]);
	endianSwap2(&ub[7],  &gl3state.uni3DData.transProjViewMat4.Elements[1][3]);
	endianSwap2(&ub[8],  &gl3state.uni3DData.transProjViewMat4.Elements[2][0]);
	endianSwap2(&ub[9],  &gl3state.uni3DData.transProjViewMat4.Elements[2][1]);
	endianSwap2(&ub[10], &gl3state.uni3DData.transProjViewMat4.Elements[2][2]);
	endianSwap2(&ub[11], &gl3state.uni3DData.transProjViewMat4.Elements[2][3]);
	endianSwap2(&ub[12], &gl3state.uni3DData.transProjViewMat4.Elements[3][0]);
	endianSwap2(&ub[13], &gl3state.uni3DData.transProjViewMat4.Elements[3][1]);
	endianSwap2(&ub[14], &gl3state.uni3DData.transProjViewMat4.Elements[3][2]);
	endianSwap2(&ub[15], &gl3state.uni3DData.transProjViewMat4.Elements[3][3]);

	endianSwap2(&ub[16+0],  &gl3state.uni3DData.transModelMat4.Elements[0][0]);
	endianSwap2(&ub[16+1],  &gl3state.uni3DData.transModelMat4.Elements[0][1]);
	endianSwap2(&ub[16+2],  &gl3state.uni3DData.transModelMat4.Elements[0][2]);
	endianSwap2(&ub[16+3],  &gl3state.uni3DData.transModelMat4.Elements[0][3]);
	endianSwap2(&ub[16+4],  &gl3state.uni3DData.transModelMat4.Elements[1][0]);
	endianSwap2(&ub[16+5],  &gl3state.uni3DData.transModelMat4.Elements[1][1]);
	endianSwap2(&ub[16+6],  &gl3state.uni3DData.transModelMat4.Elements[1][2]);
	endianSwap2(&ub[16+7],  &gl3state.uni3DData.transModelMat4.Elements[1][3]);
	endianSwap2(&ub[16+8],  &gl3state.uni3DData.transModelMat4.Elements[2][0]);
	endianSwap2(&ub[16+9],  &gl3state.uni3DData.transModelMat4.Elements[2][1]);
	endianSwap2(&ub[16+10], &gl3state.uni3DData.transModelMat4.Elements[2][2]);
	endianSwap2(&ub[16+11], &gl3state.uni3DData.transModelMat4.Elements[2][3]);
	endianSwap2(&ub[16+12], &gl3state.uni3DData.transModelMat4.Elements[3][0]);
	endianSwap2(&ub[16+13], &gl3state.uni3DData.transModelMat4.Elements[3][1]);
	endianSwap2(&ub[16+14], &gl3state.uni3DData.transModelMat4.Elements[3][2]);
	endianSwap2(&ub[16+15], &gl3state.uni3DData.transModelMat4.Elements[3][3]);

	endianSwap2(&ub[32], &gl3state.uni3DData.scroll);
	endianSwap2(&ub[33], &gl3state.uni3DData.time);
	endianSwap2(&ub[34], &gl3state.uni3DData.alpha);
	endianSwap2(&ub[35], &gl3state.uni3DData.overbrightbits);
	endianSwap2(&ub[36], &gl3state.uni3DData.particleFadeFactor);
	endianSwap2(&ub[37], &gl3state.uni3DData.lightScaleForTurb);

	// 2 --> uni3D --> PS&VS:1
    GX2Invalidate(GX2_INVALIDATE_MODE_UNIFORM_BLOCK | GX2_INVALIDATE_MODE_CPU,
		ub, 0x100);
	GX2SetPixelUniformBlock(1, 0x100, ub);
	GX2SetVertexUniformBlock(1, 0x100, ub);
}

extern int LongSwap(int l);

void WiiU_UpdateUBOLights(void)
{
	float* ub = (float*)WiiU_UBAlloc(sizeof(gl3state.uniLightsData));
	memset(ub, 0, sizeof(gl3state.uniLightsData));

	for(int i=0;i<gl3_newrefdef.num_dlights;++i)
	{
		endianSwap2(&ub[i*sizeof(gl3UniDynLight)/sizeof(float)] + 0, &gl3state.uniLightsData.dynLights[i].origin[0]);
		endianSwap2(&ub[i*sizeof(gl3UniDynLight)/sizeof(float)] + 1, &gl3state.uniLightsData.dynLights[i].origin[1]);
		endianSwap2(&ub[i*sizeof(gl3UniDynLight)/sizeof(float)] + 2, &gl3state.uniLightsData.dynLights[i].origin[2]);
		endianSwap2(&ub[i*sizeof(gl3UniDynLight)/sizeof(float)] + 4, &gl3state.uniLightsData.dynLights[i].color[0]);
		endianSwap2(&ub[i*sizeof(gl3UniDynLight)/sizeof(float)] + 5, &gl3state.uniLightsData.dynLights[i].color[1]);
		endianSwap2(&ub[i*sizeof(gl3UniDynLight)/sizeof(float)] + 6, &gl3state.uniLightsData.dynLights[i].color[2]);
		endianSwap2(&ub[i*sizeof(gl3UniDynLight)/sizeof(float)] + 7, &gl3state.uniLightsData.dynLights[i].intensity);
	}

	// 3 --> uniLights --> PS:2
    GX2Invalidate(GX2_INVALIDATE_MODE_UNIFORM_BLOCK | GX2_INVALIDATE_MODE_CPU,
		ub, sizeof(gl3state.uniLightsData));
	GX2SetPixelUniformBlock(2, sizeof(gl3state.uniLightsData), ub);
}

void WiiU_UpdateUBOLMScales(hmm_vec4 lmScales[MAX_LIGHTMAPS_PER_SURFACE])
{
	float* ub = (float*)WiiU_UBAlloc(sizeof(hmm_vec4) * MAX_LIGHTMAPS_PER_SURFACE);

	int i;
	for(i=0; i< MAX_LIGHTMAPS_PER_SURFACE; ++i)
	{
		endianSwap2(&ub[i*4+0], &lmScales[i].R);
		endianSwap2(&ub[i*4+1], &lmScales[i].G);
		endianSwap2(&ub[i*4+2], &lmScales[i].B);
		endianSwap2(&ub[i*4+3], &lmScales[i].A);
	}

	// 3 --> lmScales --> PS:3
    GX2Invalidate(GX2_INVALIDATE_MODE_UNIFORM_BLOCK | GX2_INVALIDATE_MODE_CPU,
		ub, 0x100);
	GX2SetPixelUniformBlock(3, 0x100, ub);
}

void WiiU_UpdateUBOPostProcess(float time, const float v_blend[4])
{
	float* ub = (float*)WiiU_UBAlloc(sizeof(float) + sizeof(hmm_vec4));

	endianSwap2(&ub[0], &v_blend[0]);
	endianSwap2(&ub[1], &v_blend[1]);
	endianSwap2(&ub[2], &v_blend[2]);
	endianSwap2(&ub[3], &v_blend[3]);
	endianSwap2(&ub[4], &time);

	// 3 --> uniPostProcess --> PS:4
    GX2Invalidate(GX2_INVALIDATE_MODE_UNIFORM_BLOCK | GX2_INVALIDATE_MODE_CPU,
		ub, 0x100);
	GX2SetPixelUniformBlock(4, 0x100, ub);
}
