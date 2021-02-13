
#version 330 core

out vec4 fragcolor;

in vec3 normal;
in vec3 fragpos;

uniform vec3 viewpos;
uniform vec3 lightpos;
uniform vec3 lightcolor;
uniform vec3 objcolor;

void main() {
	vec3 norm = normalize(normal);
	float ambientStrength = 0.3;
	float specularStrength = 0.1;
	
	vec3 ambient = ambientStrength * lightcolor;
	
	vec3 lightdir = lightpos - fragpos;
	float distcomponent = 20.0 / length(lightdir);
	lightdir = normalize(lightdir);
	float diff = max(dot(norm, lightdir), 0.0);
	vec3 diffuse = diff * lightcolor * distcomponent;

	vec3 viewdir = normalize(viewpos - fragpos);
	vec3 reflectdir = reflect(-lightdir, norm);
	float spec = pow(max(dot(viewdir, reflectdir), 0.0), 32);
	vec3 specular = specularStrength * spec * lightcolor;
	
	vec3 result = (ambient + diffuse + specular) * objcolor;
	fragcolor = vec4(result, 1.0);
}