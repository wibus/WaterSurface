#version 120

uniform sampler2D HeightTex;
uniform sampler2D VelocityTex;
uniform vec2 Size;

void main(void)
{
    float vel = texture2D(VelocityTex, gl_FragCoord.st / Size).z;
    if(vel > 0.5) vel -= 1.0;

    gl_FragColor = texture2D(HeightTex, gl_FragCoord.st / Size) + vec4(0, 0, vel, 0)*2;
}
