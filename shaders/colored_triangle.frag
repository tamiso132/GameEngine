#version 460

// shader input
layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) flat in uint inFaceIndex;
layout(location = 3) in vec3 inFragPos;
layout(location = 4) in vec3 camPos;

// output write
layout(location = 0) out vec4 outFragColor;

layout(set = 2, binding = 0) uniform sampler2DArray textureArray;

layout(set = 2, binding = 1) uniform GPUTextureBuffer {
    uint faceIndices[6];
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
}
textureInfo;

layout(set = 2, binding = 2) uniform sampler2DArray normalTextureArray;

void main() {
    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
    vec3 lightPos = vec3(8, 0, 0);

    vec3 normal = texture(normalTextureArray, vec3(inTexCoord,textureInfo.faceIndices[inFaceIndex])).rgb;
  
    // transform normal vector to range [-1,1]
    normal = normalize(normal * 2.0 - 1.0);   
  

    vec4 color = texture(textureArray, vec3(inTexCoord, textureInfo.faceIndices[inFaceIndex]));

    vec3 ambient = lightColor *0.2* color.rgb;

    //diff
    vec3 lightDir = normalize(lightPos - inFragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = (diff) * lightColor * color.rgb;

    // specular
    vec3 viewDir = normalize(camPos - inFragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), textureInfo.shininess);
    vec3 specular = lightColor * (spec * textureInfo.specular);

    float distance = length(lightPos - inFragPos);
    float attenuation = 1.0 / (distance * distance);

    diffuse *= attenuation;
    specular *= attenuation;
    
    // Gamma correction
    float gamma = 2.2;

    vec3 finalColor = pow(diffuse + ambient, vec3(1.0/gamma));
    
    outFragColor = vec4(finalColor, 1.0f);
}