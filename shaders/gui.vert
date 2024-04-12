#version 450

// Input vertex attributes
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

// Output variables to fragment shader
out vec2 fragTexCoord;

void main() {
    // Pass the vertex position to the fragment shader
    fragTexCoord = inTexCoord;
    
    // Transform the vertex position
    gl_Position = vec4(inPosition, 0.0, 1.0); // Assume z = 0.0 for 2D rendering
}