//we will be using glsl version 4.5 syntax
#version 450

layout (location = 0) in vec3 vPosition;

layout (location = 0) out vec3 outColor;


layout(set = 0, binding = 0) uniform  CameraBuffer{   
    mat4 view;
    mat4 proj;
	mat4 viewproj; 
} cameraData;

struct ObjectData{
	mat4 model;
}; 


layout(std140,set = 1, binding = 0) readonly buffer ObjectBuffer{   
	ObjectData objects[];
} objectBuffer;

void main() 
{

vec3 vertices[3] = vec3[3](
        vec3(0.0f, -0.5f, 1.0f),
        vec3(0.5f, 0.5f, 1.0f),
        vec3(-0.5f, 0.5f, 1.0f)
    );

    gl_Position = vec4(vPosition, 1.0);
	outColor = vec3(1.0f, 1.0f, 1.0f);
}