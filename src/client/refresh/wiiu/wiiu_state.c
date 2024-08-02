#include "header/local.h"

/*
   GX2SetScissor(0, 0, (float)sDrcColourBuffer.surface.width, (float)sDrcColourBuffer.surface.height);
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


FBO (post effects)
stencil (shadows?)
tex filtering
mipmaps
multisampling
scissor (clear out the portion of the screen that the NOWORLDMODEL defines)
*/

static BOOL depthTestEnabled = false;
static BOOL depthWriteEnabled = true;
static GX2CompareFunction depthCompareFunction = GX2_COMPARE_FUNC_LEQUAL;
void WiiU_EnableDepthTest(qboolean enable)
{
    if(enable)
    {
        depthTestEnabled = true;
        //depthCompareFunction = GX2_COMPARE_FUNC_LEQUAL;
        //depthCompareFunction = GX2_COMPARE_FUNC_ALWAYS;
    }
    else
    {
        depthTestEnabled = false;
        //depthCompareFunction = GX2_COMPARE_FUNC_ALWAYS;
    }
    GX2SetDepthOnlyControl(depthTestEnabled, depthWriteEnabled, depthCompareFunction);
}

void WiiU_EnableDepthWrite(qboolean enable)
{
    depthWriteEnabled = enable;
    GX2SetDepthOnlyControl(depthTestEnabled, depthWriteEnabled, depthCompareFunction);
}

void WiiU_EnableBlend(qboolean enable)
{
    if(enable)
    {
        GX2SetBlendControl(GX2_RENDER_TARGET_0,
                        GX2_BLEND_MODE_SRC_ALPHA, GX2_BLEND_MODE_INV_SRC_ALPHA, GX2_BLEND_COMBINE_MODE_ADD,
                        TRUE,
                        GX2_BLEND_MODE_SRC_ALPHA, GX2_BLEND_MODE_INV_SRC_ALPHA, GX2_BLEND_COMBINE_MODE_ADD);
    }
    else
    {
        GX2SetBlendControl(GX2_RENDER_TARGET_0,
                           GX2_BLEND_MODE_ONE, GX2_BLEND_MODE_ZERO, GX2_BLEND_COMBINE_MODE_ADD,
                           false,
                           GX2_BLEND_MODE_SRC_ALPHA, GX2_BLEND_MODE_INV_SRC_ALPHA, GX2_BLEND_COMBINE_MODE_ADD);
    }
}

static int vpX = 0,vpY = 0,vpW = 1280,vpH = 768;
static float depthMin = 0, depthMax = 1;
void WiiU_SetViewport(int x, int y, int w, int h)
{
    vpX = x;
    vpY = y;
    vpW = w;
    vpH = h;
   GX2SetViewport(vpX, vpY, vpW, vpH, depthMin, depthMax);
}

void WiiU_SetDepthRange(float near, float far)
{
    depthMin = near;
    depthMax = far;
    GX2SetViewport(vpX, vpY, vpW, vpH, depthMin, depthMax);
}


BOOL cullFront = false;
bool cullEnable = false;
GX2FrontFace frontFace = GX2_FRONT_FACE_CCW;
bool enablePolygonOffset = false;

void WiiU_EnableCullFace(qboolean enable)
{
    cullEnable = enable;

    GX2SetPolygonControl(frontFace, cullFront, !cullFront,
        false, GX2_POLYGON_MODE_TRIANGLE, GX2_POLYGON_MODE_TRIANGLE,
        enablePolygonOffset, enablePolygonOffset, enablePolygonOffset);
}

void WiiU_CullFront(void)
{
    cullFront = true;

    GX2SetPolygonControl(frontFace, cullFront, !cullFront,
        false, GX2_POLYGON_MODE_TRIANGLE, GX2_POLYGON_MODE_TRIANGLE,
        enablePolygonOffset, enablePolygonOffset, enablePolygonOffset);
}

void WiiU_CullBack(void)
{
    cullFront = false;

    GX2SetPolygonControl(frontFace, cullFront, !cullFront,
        false, GX2_POLYGON_MODE_TRIANGLE, GX2_POLYGON_MODE_TRIANGLE,
        enablePolygonOffset, enablePolygonOffset, enablePolygonOffset);
}

void WiiU_PolygonOffset(float factor, float units)
{
    GX2SetPolygonOffset(units, factor, units, factor, 0);
}

void WiiU_EnablePolygonOffset(qboolean enable)
{
    enablePolygonOffset = enable;
    GX2SetPolygonControl(frontFace, cullFront, !cullFront,
        false, GX2_POLYGON_MODE_TRIANGLE, GX2_POLYGON_MODE_TRIANGLE,
        enablePolygonOffset, enablePolygonOffset, enablePolygonOffset);
}
