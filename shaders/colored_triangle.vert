#version 450

layout(location = 0) in vec3 vPosition;
layout(location = 0) out vec3 inColor;

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
  // Transform the input vertex position using the model matrix and camera's
  // view-projection matrix
  gl_Position = cameraData.viewproj * objectBuffer.objects[0].model *
                vec4(vPosition, 1.0);
}
