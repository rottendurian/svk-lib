#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D tex;

layout(triangles,equal_spacing) in;

layout(location = 0) in vec3 inPosition[];
layout(location = 1) in vec3 inColor[];
layout(location = 2) in vec2 inTexCoord[];

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outColor;
layout(location = 2) out vec2 outTexCoord;



void main() {
    vec3 position = gl_TessCoord.x * inPosition[0] + gl_TessCoord.y * inPosition[1] + gl_TessCoord.z * inPosition[2];

    vec3 color = gl_TessCoord.x * inColor[0] + gl_TessCoord.y * inColor[1] + gl_TessCoord.z * inColor[2];
    vec2 texCoord = gl_TessCoord.x * inTexCoord[0] + gl_TessCoord.y * inTexCoord[1] + gl_TessCoord.z * inTexCoord[2];

    //sample texture for the blue channel and add to the y position based on the color
    float texSample = texture(tex, texCoord).b;
    position.z += (texSample*1.5);

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(position, 1.0);
    outPosition = position;
    outColor = color;
    outTexCoord = texCoord;
}

// void main() {
//     gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition[0], 1.0);
//     outPosition = inPosition[0];
//     outColor = inColor[0];
//     outTexCoord = inTexCoord[0];
// }