#version 120

attribute vec4 position_att;

void main(void)
{
    gl_Position = position_att;
}
