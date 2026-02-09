#version 330 core

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inCol;
layout(location = 2) in vec2 inTex;
layout(location = 3) in vec3 inNorm;

out vec2 chTex;
out vec3 FragPos;
out vec3 Normal;

uniform mat4 uM;
uniform mat4 uV;
uniform mat4 uP;

void main()
{
    chTex = inTex;
    FragPos = vec3(uM * vec4(inPos, 1.0));
    Normal = mat3(transpose(inverse(uM))) * inNorm;
    gl_Position = uP * uV * uM * vec4(inPos, 1.0);
}
