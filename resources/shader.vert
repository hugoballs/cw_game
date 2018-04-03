#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

//NOTE: when adding a z coordinate, don't forget to change vec2 to vec3!
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inCol;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragCol;
layout(location = 1) out vec2 fragTexCoord;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	//NOTE: inverting the -y axis is a possible solution to Vulkan's new coordinate system.
    //another is modifying the projection matrix
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPos.x, inPos.y, inPos.z, 1.0);
   	//gl_Position = vec4(inPos.x, inPos.y, 0.0, 1.0);
   	vec4 temp = ubo.proj * ubo.view * ubo.model * vec4(inCol, 1.0);
    fragCol = vec3(temp.x, temp.y, temp.z);
    //NOTE: don't forget to change the texture coordinates too
    fragTexCoord = inTexCoord;
}
