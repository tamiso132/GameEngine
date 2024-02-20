#version 460

// shader input
layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) flat in uint inFaceIndex;
layout(location = 3) in vec3 inFragPos;

// output write
layout(location = 0) out vec4 outFragColor;

layout(set = 2, binding = 1) uniform GPUTextureBuffer {
    uint faceIndices[6];
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
}
textureInfo;

layout(set = 2, binding = 0) uniform sampler2DArray textureArray;

void main() {
    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);

    vec4 color = texture(textureArray, vec3(inTexCoord, textureInfo.faceIndices[inFaceIndex]));

    vec3 ambient = lightColor * textureInfo.ambient * color.rgb;

    vec3 norm = normalize(inNormal);
    vec3 lightDir = normalize(vec3(0, 0, 0) - inFragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = (diff * textureInfo.diffuse) * lightColor * color.rgb;

    outFragColor = vec4(diffuse + ambient, 1.0f);
}