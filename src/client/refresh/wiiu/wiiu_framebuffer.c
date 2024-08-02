#include "header/local.h"
#include <coreinit/memdefaultheap.h>
#include <gx2/state.h>
#include <malloc.h>


//#define USE_MSAA
#define RTT_WIDTH 320
#define RTT_HEIGHT 200


static void
GfxInitTexture(GX2Texture *tex,
            uint32_t width,
            uint32_t height,
            GX2SurfaceFormat format,
            GX2AAMode aa)
{
    memset(tex, 0, sizeof(GX2Texture));

    tex->surface.dim = GX2_SURFACE_DIM_TEXTURE_2D;
    tex->surface.aa = aa;
    tex->surface.width = width;
    tex->surface.height = height;
    tex->surface.depth = 1;
    tex->surface.mipLevels = 1;
    tex->surface.format = format;
    tex->surface.use = (GX2SurfaceUse)(GX2_SURFACE_USE_TEXTURE | GX2_SURFACE_USE_COLOR_BUFFER);
    tex->surface.tileMode = GX2_TILE_MODE_DEFAULT;
    tex->viewNumSlices = 1;

    BOOL res;
    res = GX2RCreateSurface(&tex->surface,
                            GX2R_RESOURCE_BIND_TEXTURE |
                            GX2R_RESOURCE_USAGE_GPU_WRITE |
                            GX2R_RESOURCE_USAGE_GPU_READ);
    GX2InitTextureRegs(tex);

    if(res != true)
    {
        WHBLogWritef("Failed to create FBO surface\n");
    }
}

// from wut
static void
GfxInitDepthBuffer(GX2DepthBuffer *db,
                   uint32_t width,
                   uint32_t height,
                   GX2SurfaceFormat format,
                   GX2AAMode aa)
{
    memset(db, 0, sizeof(GX2DepthBuffer));

    db->surface.use = GX2_SURFACE_USE_DEPTH_BUFFER;
    db->surface.dim = GX2_SURFACE_DIM_TEXTURE_2D;
    db->surface.width = width;
    db->surface.height = height;
    db->surface.depth = 1;
    db->surface.mipLevels = 1;
    db->surface.format = format;
    db->surface.aa = aa;
    db->surface.tileMode = GX2_TILE_MODE_DEFAULT;
    db->viewNumSlices = 1;
    db->depthClear = 1.0f;
    GX2CalcSurfaceSizeAndAlignment(&db->surface);
    GX2InitDepthBufferRegs(db);
    db->surface.image = memalign(db->surface.alignment, db->surface.imageSize);
}

static void
GfxInitColorBufferFromTexture(GX2ColorBuffer* buf,
const GX2Texture *tex)
{
    memset(buf, 0, sizeof(GX2ColorBuffer));
    memcpy(&buf->surface, &tex->surface, sizeof(buf->surface));
    buf->viewNumSlices = 1;
    GX2InitColorBufferRegs(buf);
}

void WiiU_FBO_Init(framebuffer_t *fbo, int width, int height)
{
    GfxInitTexture(&fbo->colourTexture, width, height, GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, GX2_AA_MODE1X);
    GfxInitDepthBuffer(&fbo->depth, width, height, GX2_SURFACE_FORMAT_FLOAT_D24_S8, GX2_AA_MODE1X);
    GfxInitColorBufferFromTexture(&fbo->colourBuffer, &fbo->colourTexture);

    // Initialise FBO context state.
    //fbo->ctx = MEMAllocFromDefaultHeapEx(sizeof(GX2ContextState), GX2_CONTEXT_STATE_ALIGNMENT);
    fbo->ctx = memalign(GX2_CONTEXT_STATE_ALIGNMENT, sizeof(GX2ContextState));
    if (!fbo->ctx) {
        WHBLogWritef("%s: failed to allocate ctx\n", __FUNCTION__);
    }
    //GX2SetupContextStateEx(fbo->ctx, 0);
    //GX2SetContextState(fbo->ctx);
// /*  Some flags */
//     GX2SetAlphaTest(TRUE, GX2_COMPARE_FUNC_GREATER, 0.0f);
//     GX2SetDepthOnlyControl(TRUE, TRUE, GX2_COMPARE_FUNC_LESS);
//     GX2SetCullOnlyControl(GX2_FRONT_FACE_CCW, FALSE, FALSE);
//     GX2SetColorControl(GX2_LOGIC_OP_COPY, 0xFF, FALSE, TRUE);

    //GX2SetColorBuffer(&fbo->colourBuffer, GX2_RENDER_TARGET_0);
    //GX2SetDepthBuffer(&fbo->depth);
    //GX2SetViewport(0, 0, (float)fbo->colourBuffer.surface.width, (float)fbo->colourBuffer.surface.height, 0.0f, 1.0f);
    //GX2SetScissor(0, 0, (float)fbo->colourBuffer.surface.width, (float)fbo->colourBuffer.surface.height);

}

void WiiU_FBO_Cleanup(framebuffer_t *fbo)
{

}

void WiiU_FBO_Activate(framebuffer_t *fbo)
{
    //GX2SetContextState(fbo->ctx);
	//GX2SetShaderMode(GX2_SHADER_MODE_UNIFORM_BLOCK);

    GX2SetColorBuffer(&fbo->colourBuffer, GX2_RENDER_TARGET_0);
    GX2SetDepthBuffer(&fbo->depth);
    GX2SetViewport(0, 0, (float)fbo->colourBuffer.surface.width, (float)fbo->colourBuffer.surface.height, 0.0f, 1.0f);
    GX2SetScissor(0, 0, (float)fbo->colourBuffer.surface.width, (float)fbo->colourBuffer.surface.height);

    GX2ClearColor(&fbo->colourBuffer, 0.0f, 1.0f, 0.0f, 0.0f);
    GX2ClearDepthStencilEx(&fbo->depth, fbo->depth.depthClear, fbo->depth.stencilClear,
        (GX2ClearFlags)(GX2_CLEAR_FLAGS_BOTH));

    gl3state.activeFBO = fbo;
}

GX2Texture *WiiU_FBO_ContentDone(framebuffer_t *fbo)
{
    GX2Flush();
    GX2Invalidate(GX2_INVALIDATE_MODE_COLOR_BUFFER, 0, 0xffffffff);
    GX2Invalidate(GX2_INVALIDATE_MODE_TEXTURE, 0, 0xffffffff);

    GX2ColorBuffer *colourBuffer = WHBGfxGetTVColourBuffer();
    GX2DepthBuffer *depthBuffer = WHBGfxGetTVDepthBuffer();
    GX2SetColorBuffer(colourBuffer, GX2_RENDER_TARGET_0);
    GX2SetDepthBuffer(depthBuffer);
    GX2SetViewport(0, 0, (float)colourBuffer->surface.width, (float)colourBuffer->surface.height, 0.0f, 1.0f);
    GX2SetScissor(0, 0, (float)colourBuffer->surface.width, (float)colourBuffer->surface.height);

    gl3state.activeFBO = NULL;

    return &fbo->colourTexture;
}
