#version 460

// Input vertex attributes
layout(location = 0) in vec2 inPosition;

// Output variables to fragment shader
layout(location = 0) out vec2 fragTexCoord;

void main() {
    // Pass the vertex position to the fragment shader
    fragTexCoord = vec2(0,0);
    
    // Transform the vertex position
    gl_Position = vec4(inPosition, 0.0, 1.0); 
}