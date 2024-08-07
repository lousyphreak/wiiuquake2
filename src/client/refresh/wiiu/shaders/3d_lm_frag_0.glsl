#version 450

layout(location = 0) in vec2 passTexCoord;

layout(location = 0) out vec4 outColor;

// for UBO shared between all shaders (incl. 2D)
layout (std140, binding = 0) uniform uniCommon
{
    float gamma; // this is 1.0/vid_gamma
    float intensity;
    float intensity2D; // for HUD, menus etc

    vec4 color; // really?
};
// for UBO shared between all 3D shaders
layout (std140, binding = 1) uniform uni3D
{
    mat4 transProjView;
    mat4 transModel;

    float scroll; // for SURF_FLOWING
    float time;
    float alpha;
    float overbrightbits;
    float particleFadeFactor;
    float lightScaleForTurb; // surfaces with SURF_DRAWTURB (water, lava) don't have lightmaps, use this instead
    float _pad_1; // AMDs legacy windows driver needs this, otherwise uni3D has wrong size
    float _pad_2;
};


struct DynLight { // gl3UniDynLight in C
    vec4 lightOrigin;
    vec4 lightColor; // .a is intensity; this way it also works on OSX...
    // (otherwise lightIntensity always contained 1 there)
};

layout (std140, binding = 2) uniform uniLights
{
    DynLight dynLights[32];
};

layout (binding = 0) uniform sampler2D tex;

layout (binding = 2) uniform sampler2D lightmap0;
layout (binding = 3) uniform sampler2D lightmap1;
layout (binding = 4) uniform sampler2D lightmap2;
layout (binding = 5) uniform sampler2D lightmap3;

layout (std140, binding = 3) uniform uniLmScales
{
    uniform vec4 lmScales[4];
};


layout(location = 1) in vec2 passLMcoord;
layout(location = 2) in vec3 passWorldCoord;
layout(location = 3) in vec3 passNormal;
layout(location = 4) flat in uint passLightFlags;

void main()
{
    vec4 texel = texture(tex, passTexCoord);

    // apply intensity
    texel.rgb *= intensity;

    // apply lightmap
    vec4 lmTex = texture(lightmap0, passLMcoord);// * lmScales[0];
    lmTex     += texture(lightmap1, passLMcoord);// * lmScales[1];
    lmTex     += texture(lightmap2, passLMcoord);// * lmScales[2];
    lmTex     += texture(lightmap3, passLMcoord);// * lmScales[3];

    lmTex.rgb *= overbrightbits;
    outColor = lmTex*texel;
    outColor.rgb = pow(outColor.rgb, vec3(gamma)); // apply gamma correction to result

    outColor.a = 1.0; // lightmaps aren't used with translucent surfaces
}
