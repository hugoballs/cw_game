#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec3 inCol;

layout(location = 0) out vec3 fragCol;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	//NOTE: IMPORTANT! not inverting the -y axis gives a back screen: investigate.
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPos.x, -inPos.y, 0.0, 1.0);
   	//gl_Position = vec4(inPos.x, inPos.y, 0.0, 1.0);
   	vec4 temp = ubo.proj * ubo.view * ubo.model * vec4(inCol, 1.0);
    fragCol = vec3(temp.x, temp.y, temp.z);
}
