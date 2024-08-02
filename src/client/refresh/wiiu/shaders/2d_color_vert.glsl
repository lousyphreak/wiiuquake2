#version 450

layout(location = 0) in vec2 position; // WiiU_ATTRIB_POSITION

// for UBO shared between 2D shaders
layout (std140, binding = 0) uniform uni2D
{
    mat4 trans;
};

void main()
{
    gl_Position = trans * vec4(position, 0.0, 1.0);
}
