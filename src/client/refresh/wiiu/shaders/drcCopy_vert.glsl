#version 450

layout(location = 0) in vec2 position; // WiiU_ATTRIB_POSITION
layout(location = 1) in vec2 texCoord; // WiiU_ATTRIB_TEXCOORD

layout(location = 0) out vec2 passTexCoord;

void main()
{
    gl_Position = vec4(position,position.x, 1.0);
    passTexCoord = texCoord;
}
