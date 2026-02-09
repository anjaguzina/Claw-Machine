#version 330 core

in vec2 chTex;
in vec3 FragPos;
in vec3 Normal;

out vec4 outCol;

uniform sampler2D uTex;
uniform vec4 uColor;
uniform bool useTex;
uniform bool transparent;
uniform bool overlayMode;  // kada true: samo tekstura * uColor, bez osvetljenja (za 2D overlay)

// Osvetljenje
uniform vec3 uLightPos;
uniform vec3 uViewPos;
uniform vec3 uLightColor;

// Materijal
uniform vec3 uMaterialAmbient;
uniform vec3 uMaterialDiffuse;
uniform vec3 uMaterialSpecular;
uniform float uMaterialShininess;

void main()
{
    vec3 color = uColor.rgb;
    vec4 texCol = vec4(1.0);
    if (useTex)
    {
        texCol = texture(uTex, chTex);
        color = texCol.rgb;
    }

    // Overlay režim: samo tekstura * uColor, bez osvetljenja (slova ostaju oštra i vidljiva)
    if (overlayMode && useTex)
    {
        outCol = texCol * uColor;
        return;
    }

    // Ambient
    vec3 ambient = uMaterialAmbient * uLightColor;
    
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(uLightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uMaterialDiffuse * uLightColor;
    
    // Specular
    vec3 viewDir = normalize(uViewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uMaterialShininess);
    vec3 specular = spec * uMaterialSpecular * uLightColor;
    
    vec3 result = (ambient + diffuse + specular) * color;
    
    if (transparent && uColor.a < 0.01)
        discard;

    outCol = vec4(result, uColor.a);
}
