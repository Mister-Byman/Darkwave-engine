#version 450

layout(set = 0, binding = 0) uniform UBO {
  mat4 view;
  mat4 proj;
} ubo;

layout(push_constant) uniform PC {
  mat4 model;
} pc;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 vColor;

void main() {
  gl_Position = ubo.proj * ubo.view * pc.model * vec4(inPos, 1.0);
  vColor = inColor;
}
