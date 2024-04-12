#version 450

// Input variable from vertex shader
in vec2 fragTexCoord;

// Output color
layout(location = 0) out vec4 outColor;

void main() {
    // Sample a texture or calculate color based on fragTexCoord
    // For simplicity, just output a solid color (e.g., white)
    outColor = vec4(1.0, 1.0, 1.0, 1.0); // White color with full opacity
}