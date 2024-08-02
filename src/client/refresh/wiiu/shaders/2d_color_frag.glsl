#version 450

// for UBO shared between all shaders (incl. 2D)
layout (std140, binding = 0) uniform uniCommon
{
    float gamma;
    float intensity;
    float intensity2D; // for HUD, menus etc

    vec4 color;
};

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 col = color.rgb * intensity2D;
    outColor.rgb = pow(col, vec3(gamma));
    outColor.a = color.a;
}
