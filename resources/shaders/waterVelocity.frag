#version 130

uniform sampler2D GroundHeightTex;
uniform sampler2D WaterHeightTex;
uniform sampler2D WaterVelocityTex;

in vec2 position;

const ivec2 offsets[25] = ivec2[25](
    ivec2(-2, -2), ivec2(-1, -2), ivec2(0, -2), ivec2(1, -2), ivec2(2, -2),
    ivec2(-2, -1), ivec2(-1, -1), ivec2(0, -1), ivec2(1, -1), ivec2(2, -1),
    ivec2(-2,  0), ivec2(-1,  0), ivec2(0,  0), ivec2(1,  0), ivec2(2,  0),
    ivec2(-2,  1), ivec2(-1,  1), ivec2(0,  1), ivec2(1,  1), ivec2(2,  1),
    ivec2(-2,  2), ivec2(-1,  2), ivec2(0,  2), ivec2(1,  2), ivec2(2,  2)
);
const float coeffs[25] = float[25](
    0, 3, 4, 3, 0,
    3, 6, 7, 6, 3,
    4, 7, 0, 7, 4,
    3, 6, 7, 6, 3,
    0, 3, 4, 3, 0
);

void main(void)
{
    vec4 mgh = texture(GroundHeightTex,  position);
    vec4 mwh = texture(WaterHeightTex,   position);
    vec4 mwv = texture(WaterVelocityTex, position);
    vec4 mwp = max(mwh - mgh, 0.0);

    float acc = 0.0;
    vec4  sum = vec4(0.0);
    for(int i=0; i < 25; ++i)
    {
        vec4 gh = textureOffset(GroundHeightTex, position, offsets[i]);
        vec4 wh = textureOffset(WaterHeightTex,  position, offsets[i]);
        vec4 wp = max(wh - gh, 0.0);

        vec4 door = vec4(1.0);
        door *= step(0.0, mwh - gh);
        door *= step(0.0, wh - mgh);

        sum += (wp - mwp) * door * coeffs[i];
        acc += coeffs[i];
    }

    vec4 f = -mwv * 0.001;

    gl_FragColor = mwv + f + (sum/acc)*0.8;
}
