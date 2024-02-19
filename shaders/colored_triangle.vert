#version 450

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vColor;
layout(location = 2) in vec2 vTexCoord;
layout(location = 3) in uint vFaceIndex;

layout(location = 0) out vec3 inColor;
layout(location = 1) out vec2 texCoord;
layout(location = 2) out uint inFaceIndex;

layout(set = 0, binding = 0) uniform CameraBuffer {
  mat4 view;
  mat4 proj;
  mat4 viewproj;
}
cameraData;

struct ObjectData {
  mat4 model;
};

layout(std140, set = 1, binding = 0) readonly buffer ObjectBuffer {
  ObjectData objects[]; // Declare as an array
}
objectBuffer;

void main() {
  gl_Position = cameraData.viewproj * objectBuffer.objects[0].model *
                vec4(vPosition, 1.0);
  inColor = vColor;
  texCoord = vTexCoord;
  inFaceIndex = vFaceIndex;
}
