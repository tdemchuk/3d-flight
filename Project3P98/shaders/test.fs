
#version 330 core

out vec4 fragcolor;

in vec3 normal;
in vec3 fragpos;

void main() {
	fragcolor = vec4(0.1, 0.8, 0.3, 1.0);
}