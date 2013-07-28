#version 130

uniform sampler2D GroundHeightTex;
uniform sampler2D WaterHeightTex;
uniform sampler2D WaterVelocityTex;

in vec2 position;

void main(void)
{
    vec4 g = texture(GroundHeightTex, position);
    vec4 h = texture(WaterHeightTex, position);
    vec4 v = texture(WaterVelocityTex, position);

    if(h.z <= g.z)
    {
        h.z = g.z;
    }

    gl_FragColor = h + v;
}
