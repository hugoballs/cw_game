#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragCol;

layout(location = 0) out vec4 outCol;

void main() {
    outCol = vec4(fragCol, 1.0);
}