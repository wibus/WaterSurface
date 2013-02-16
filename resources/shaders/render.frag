#version 120

uniform sampler2D DiffuseTex;
uniform vec4 Color;
varying vec2 texCoords;

void main(void)
{
    float intensity = texture2D(DiffuseTex, texCoords).z;
    vec4 color = vec4(0.0);
    color.g = 0.2;
    if(intensity < 0.5) color.r = intensity*2.0;
    if(intensity > 0.5) color.b = (1.0-intensity)*2.0;
    color.a = 0.8;

    gl_FragColor = color;
}
