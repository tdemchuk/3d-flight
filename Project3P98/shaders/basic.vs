
#version 330 core

layout (location = 0) in vec3 pos;

out vec3 fragpos;

uniform mat4 projectionViewMatrix;

void main() {
	fragpos = pos;
	gl_Position = projectionViewMatrix * vec4(pos, 1.0);
}