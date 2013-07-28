#version 130

in  vec3 position_att;
out vec2 position;

void main(void)
{
    position = position_att.xy;
    gl_Position = vec4(position_att*2.0 -vec3(1.0), 1.0);
}
