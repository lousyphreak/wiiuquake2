#version 450

layout(location = 0) in vec3 position;   // WiiU_ATTRIB_POSITION
layout(location = 1) in vec2 texCoord;   // WiiU_ATTRIB_TEXCOORD
layout(location = 2) in vec2 lmTexCoord; // WiiU_ATTRIB_LMTEXCOORD
layout(location = 3) in vec4 vertColor;  // WiiU_ATTRIB_COLOR
layout(location = 4) in vec3 normal;     // WiiU_ATTRIB_NORMAL
layout(location = 5) in uint lightFlags; // WiiU_ATTRIB_LIGHTFLAGS

layout(location = 0) out vec2 passTexCoord;

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

layout(location = 1) out vec2 passLMcoord;
layout(location = 2) out vec3 passWorldCoord;
layout(location = 3) out vec3 passNormal;
layout(location = 4) flat out uint passLightFlags;

void main()
{
    passTexCoord = texCoord + vec2(scroll, 0.0);
    passLMcoord = lmTexCoord;
    vec4 worldCoord = transModel * vec4(position, 1.0);
    passWorldCoord = worldCoord.xyz;
    vec4 worldNormal = transModel * vec4(normal, 0.0f);
    passNormal = normalize(worldNormal.xyz);
    passLightFlags = lightFlags;

    gl_Position = transProjView * worldCoord;
}
