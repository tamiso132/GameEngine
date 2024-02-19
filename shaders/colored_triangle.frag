//glsl version 4.5
#version 450

layout(set = 2, binding = 1) uniform GPUTexture {
  uint faceIndices[6];
}
textureIndex;

//shader input
layout (location = 0) in vec3 inColor;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) flat in uint inFaceIndex;

layout(set = 2, binding = 0) uniform sampler2DArray textureArray;



//output write
layout (location = 0) out vec4 outFragColor;


void main() 
{
	vec4 color = texture(textureArray, vec3(inTexCoord, textureIndex.faceIndices[inFaceIndex]));
	outFragColor = color;
}