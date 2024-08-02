#version 450

in vec3 position;   // WiiU_ATTRIB_POSITION
in vec2 texCoord;   // WiiU_ATTRIB_TEXCOORD
in vec2 lmTexCoord; // WiiU_ATTRIB_LMTEXCOORD
in vec4 vertColor;  // WiiU_ATTRIB_COLOR
in vec3 normal;     // WiiU_ATTRIB_NORMAL
in uint lightFlags; // WiiU_ATTRIB_LIGHTFLAGS

out vec2 passTexCoord;

// for UBO shared between all 3D shaders
layout (std140) uniform uni3D
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

void main()
{
    passTexCoord = texCoord + vec2(scroll, 0.0);
    gl_Position = transProjView * transModel * vec4(position, 1.0);
}
