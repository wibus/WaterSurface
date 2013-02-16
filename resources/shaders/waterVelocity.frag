#version 120

uniform sampler2D HeightTex;
uniform sampler2D VelocityTex;
uniform vec2 Size;

void main(void)
{
    float currHeight   = texture2D(HeightTex,   gl_FragCoord.st / Size).z;
    float currVelocity = texture2D(VelocityTex, gl_FragCoord.st / Size).z;

    float r = 1.0;
    int nbSamp = 0;
    float around = 0.0;

    around += texture2D(HeightTex, (gl_FragCoord.st + vec2(-r, -r)) / Size).z; ++nbSamp;
    around += texture2D(HeightTex, (gl_FragCoord.st + vec2(-r,  0)) / Size).z; ++nbSamp;
    around += texture2D(HeightTex, (gl_FragCoord.st + vec2(-r,  r)) / Size).z; ++nbSamp;
    around += texture2D(HeightTex, (gl_FragCoord.st + vec2( 0,  r)) / Size).z; ++nbSamp;
    around += texture2D(HeightTex, (gl_FragCoord.st + vec2( r,  r)) / Size).z; ++nbSamp;
    around += texture2D(HeightTex, (gl_FragCoord.st + vec2( r,  0)) / Size).z; ++nbSamp;
    around += texture2D(HeightTex, (gl_FragCoord.st + vec2( r, -r)) / Size).z; ++nbSamp;
    around += texture2D(HeightTex, (gl_FragCoord.st + vec2( 0, -r)) / Size).z; ++nbSamp;

    currVelocity += (around/nbSamp - currHeight)/16.0;
    if(currVelocity < 0.0) currVelocity += 1.0;
    if(currVelocity > 1.0) currVelocity -= 1.0;

    gl_FragColor = vec4(0, 0, currVelocity, 0.0);
}
