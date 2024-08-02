#version 450

layout (binding = 0) uniform sampler2D tex;
layout(location = 0) in vec2 passTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    vec2 neighbor = vec2(1.0/1280.0, 1.0/768.0);
    vec4 outColor1 = texture(tex, vec2(passTexCoord.x + 0, passTexCoord.y + 0));
    vec4 outColor2 = texture(tex, vec2(passTexCoord.x + neighbor.x, passTexCoord.y + 0));
    vec4 outColor3 = texture(tex, vec2(passTexCoord.x + 0, passTexCoord.y + neighbor.y));
    vec4 outColor4 = texture(tex, vec2(passTexCoord.x + neighbor.x, passTexCoord.y + neighbor.y));
    //outColor = vec4(1,0,0,1);
    //if(passTexCoord.x > 0.5)
    //    outColor = vec4(passTexCoord,0,1);

    vec4 outColorm1 = mix(outColor1, outColor2, 0.5);
    vec4 outColorm2 = mix(outColor3, outColor4, 0.5);
    outColor = mix(outColorm1, outColorm2, 0.5);
}
