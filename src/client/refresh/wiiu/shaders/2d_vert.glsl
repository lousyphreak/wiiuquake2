#version 450
#extension GL_ARB_explicit_uniform_location : enable


layout(location = 0) in vec2 position; // WiiU_ATTRIB_POSITION
layout(location = 1) in vec2 texCoord; // WiiU_ATTRIB_TEXCOORD

// for UBO shared between 2D shaders
layout (std140, binding = 0) uniform uni2D
{
    mat4 trans;
};

layout(location = 0) out vec2 passTexCoord;

void main()
{
    gl_Position = trans * vec4(position, 0.0, 1.0);
    passTexCoord = texCoord;
}
