#version 460

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoord;
layout(location = 3) in uint vFaceIndex;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 texCoord;
layout(location = 2) out uint outFaceIndex;
layout(location = 3) out vec3 outFrag;
layout(location = 4) out vec3 camPos;




struct ObjectData {
    mat4 model;
};

layout(set = 1, binding = 0) uniform CameraBuffer {
    mat4 viewproj;
    vec3 camPos;

}
cameraData;



layout(std140, set = 1, binding = 1) readonly buffer ObjectBuffer {
    ObjectData objects[]; // Declare as an array
}
objectBuffer;

void main() {
    gl_Position = cameraData.viewproj * objectBuffer.objects[gl_InstanceIndex].model * vec4(vPosition, 1.0);
    outNormal = vNormal;
    texCoord = vTexCoord;
    outFaceIndex = vFaceIndex;
    camPos = cameraData.camPos;

    outFrag = (objectBuffer.objects[gl_InstanceIndex].model * vec4(vPosition, 1.0)).rgb;
}
