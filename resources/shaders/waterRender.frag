#version 130

uniform sampler2D GroundHeightTex;
uniform sampler2D WaterHeightTex;
uniform sampler2D WaterVelocityTex;
uniform vec4 Color;

in vec2 position;
in vec3 normal;

void main(void)
{
    normal = normalize(normal);
    gl_FragColor = vec4(texture(WaterHeightTex, position).rgb * Color.rgb, Color.a);
}
