#version 120

struct PositionLight
{
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 position;
    vec4 attenuationCoefs;
};
struct Material
{
    vec4  diffuse;
    vec4  specular;
    float shininess;
    float fresnel;
};

uniform PositionLight light;
uniform Material material;
uniform sampler2D DiffuseTex;

varying vec4 fragPos;
varying vec3 norm;
varying vec2 texc;

vec4 NormalizedBlinnPhong(in vec3 N, in vec3 L, in vec3 V)
{
    vec4 diffuseColor = material.diffuse * texture2D(DiffuseTex, texc);
    vec4 diffuse = light.diffuse * diffuseColor;
    float specInt = material.shininess * material.fresnel * pow(max(dot(reflect(-L,N), V), 0.0), material.shininess);
    vec4  specular = light.specular * material.specular*specInt;

    vec4 lightContribution = (diffuse + specular) * max(dot(N, L), 0.0) / 3.1416;
    vec4 ambient = light.ambient * diffuseColor;

    return ambient + lightContribution;
}

void main(void)
{
    vec3 fragToLight = (light.position - fragPos).xyz;

    vec3 V = normalize(-fragPos.xyz);
    vec3 L = normalize(fragToLight);
    vec3 N = normalize(norm);

    vec4 color = NormalizedBlinnPhong(N, L, V);

    float lightDist = length(fragToLight);
    float attenuation = 1.0 /
            (light.attenuationCoefs.x +
            (light.attenuationCoefs.y +
            (light.attenuationCoefs.z * lightDist)*lightDist));

    gl_FragColor = vec4(color.rgb, max(material.diffuse.a, color.a));
}
