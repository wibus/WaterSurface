#version 130

uniform sampler2D HeightTex;
uniform mat4 Projection;
uniform mat4 View;
uniform vec3 Scale;

in  vec3 position_att;
out vec2 position;
out vec3 normal;

void main(void)
{
    position = position_att.xy;
    vec3 pos = texture(HeightTex, position).xyz;
    vec3 posRt = textureOffset(HeightTex, position, ivec2(1, 0)).xyz;
    vec3 posUp = textureOffset(HeightTex, position, ivec2(0, 1)).xyz;
    normal = cross(posRt - pos, posUp - pos) / Scale;
    gl_Position = Projection * View * vec4(pos * Scale, 1.0);
}
