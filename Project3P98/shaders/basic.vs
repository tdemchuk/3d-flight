
#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;

out vec3 fragpos;
out vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform mat3 normalMatrix;

void main() {
	fragpos = vec3(model * vec4(pos, 1.0));
	normal = normalMatrix * norm;
	gl_Position = proj * view * model * vec4(pos, 1.0);
}