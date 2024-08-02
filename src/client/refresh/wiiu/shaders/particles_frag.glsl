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

layout(location = 1) in vec4 passColor;

void main()
{
    vec2 offsetFromCenter = 2.0*(gl_PointCoord - vec2(0.5, 0.5)); // normalize so offset is between 0 and 1 instead 0 and 0.5
    float distSquared = dot(offsetFromCenter, offsetFromCenter);
    // if(distSquared > 1.0) // this makes sure the particle is round
    //     discard;

    vec4 texel = passColor;

    // apply gamma correction and intensity
    //texel.rgb *= intensity; TODO: intensity? Probably not?
    outColor.rgb = pow(texel.rgb, vec3(gamma));

    // I want the particles to fade out towards the edge, the following seems to look nice
    texel.a *= min(1.0, particleFadeFactor*(1.0 - distSquared));

    outColor.a = texel.a; // I think alpha shouldn't be modified by gamma and intensity
    // outColor = vec4(0,1,0,1);
    // outColor.r = gl_PointCoord.s;
    // outColor.g = gl_PointCoord.t;
    // outColor.b = 0;//sqrt(sqrt(sqrt(distSquared/5)));
    outColor.a = passColor.a;
}
