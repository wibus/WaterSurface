#version 120

uniform mat4 Projection;
uniform mat4 View;
uniform vec2 Size;
uniform float ZDepth;

attribute vec3 position_att;
varying vec2 texCoords;

void main(void)
{
    texCoords = position_att.xy / Size;
    gl_Position = Projection * View * vec4(position_att/vec3(Size, ZDepth), 1.0);
}
