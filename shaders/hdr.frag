#version 450

layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec2 TexCoords;

layout(binding = 0) uniform sampler2D hdrBuffer;

void main()
{             
    const float gamma = 2.2;
    vec3 hdrColor = texture(hdrBuffer, TexCoords).rgb;
  
    // reinhard tone mapping
    vec3 mapped = hdrColor / (hdrColor + vec3(1.0));
    // gamma correction 
    mapped = pow(mapped, vec3(1.0 / gamma));
  
    FragColor = vec4(mapped, 1.0);
}