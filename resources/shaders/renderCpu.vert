#version 120

uniform mat4 Projection;
uniform mat4 View;
uniform mat3 Normal;

attribute vec3 position;
attribute vec3 normal;
attribute vec2 texCoord;

varying vec4 fragPos;
varying vec3 norm;
varying vec2 texc;

void main(void)
{
    texc = texCoord;
    norm = Normal * normal;
    fragPos = View * vec4(position, 1);
    gl_Position = Projection * fragPos;
}
