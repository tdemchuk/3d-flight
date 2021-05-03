
#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;
layout (location = 2) in vec2 tex;

out vec3 fragpos;
out vec3 normal;
out vec2 texcoord;

uniform mat4 projectionViewMatrix;

void main() {
	fragpos = pos;
	normal = norm;
	texcoord = tex;
	gl_Position = projectionViewMatrix * vec4(pos, 1.0);
}