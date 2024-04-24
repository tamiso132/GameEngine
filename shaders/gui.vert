#version 460

struct ObjectData {
    mat4 model;
};


// Input vertex attributes
layout(location = 0) in vec3 inPosition;

// Output variables to fragment shader
layout(location = 0) out vec2 fragTexCoord;

layout(std140, set = 0, binding = 0) readonly buffer ObjectBuffer {
    ObjectData objects[]; // Declare as an array
}
objectBuffer;


void main() {
    // Pass the vertex position to the fragment shader
    fragTexCoord = vec2(0.2f,0.2f);
    
    // Transform the vertex position
    gl_Position = objectBuffer.objects[gl_BaseInstance].model * vec4(inPosition, 1.0f); 
}