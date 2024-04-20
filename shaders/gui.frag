#version 460

// Input variable from vertex shader
layout(location = 0) in vec2 fragTexCoord;

// Output color
layout(location = 0) out vec4 outColor;

void main() {
    // Sample a texture or calculate color based on fragTexCoord
    // For simplicity, just output a solid color (e.g., white)
    outColor = vec4(0.0f, 0.0f, 0.0f, 1.0f); // White color with full opacity
}