//glsl version 4.5
#version 450

//shader input
layout (location = 0) in vec3 inColor;
layout(location = 1) in vec3 direction;

layout(set = 2, binding = 0) uniform samplerCubeArray cubeMapArray;


//output write
layout (location = 0) out vec4 outFragColor;


void main() 
{
	vec4 color = texture(cubeMapArray, vec4(0.0,1.0,0.0,1.0));
	outFragColor = vec4(color.rgb,1.0f);
	
}