#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    float tessLevel;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D tex;

layout (vertices = 3) out;

layout(location = 0) in vec3 inPosition[];
layout(location = 1) in vec3 inColor[];
layout(location = 2) in vec2 inTexCoord[];

layout(location = 0) out vec3 outPosition[];
layout(location = 1) out vec3 outColor[];
layout(location = 2) out vec2 outTexCoord[];


void main() {
    // float tessLevel = 60.0;
    gl_TessLevelInner[0] = ubo.tessLevel;
    gl_TessLevelInner[1] = ubo.tessLevel;
    gl_TessLevelOuter[0] = ubo.tessLevel;
    gl_TessLevelOuter[1] = ubo.tessLevel;
    gl_TessLevelOuter[2] = ubo.tessLevel;
    gl_TessLevelOuter[3] = ubo.tessLevel;
    
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    outPosition[gl_InvocationID] = inPosition[gl_InvocationID];
    outColor[gl_InvocationID] = inColor[gl_InvocationID];
    outTexCoord[gl_InvocationID] = inTexCoord[gl_InvocationID];
}

// void main() {
//     gl_TessLevelInner[0] = 40.0;
//     gl_TessLevelInner[1] = 40.0;
//     gl_TessLevelOuter[0] = 40.0;
//     gl_TessLevelOuter[1] = 40.0;
//     gl_TessLevelOuter[2] = 40.0;
//     gl_TessLevelOuter[3] = 40.0;

//     gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
//     outPosition[gl_InvocationID] = inPosition[gl_InvocationID];
//     outColor[gl_InvocationID] = inColor[gl_InvocationID];
//     outTexCoord[gl_InvocationID] = inTexCoord[gl_InvocationID];
// }
