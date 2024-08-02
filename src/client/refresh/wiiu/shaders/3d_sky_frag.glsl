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

layout (binding = 0) uniform sampler2D tex;

void main()
{
    vec4 texel = texture(tex, passTexCoord);

    // TODO: something about GL_BLEND vs GL_ALPHATEST etc

    // apply gamma correction
    // texel.rgb *= intensity; // TODO: really no intensity for sky?
    outColor.rgb = pow(texel.rgb, vec3(gamma));
    outColor.a = texel.a*alpha; // I think alpha shouldn't be modified by gamma and intensity
}
