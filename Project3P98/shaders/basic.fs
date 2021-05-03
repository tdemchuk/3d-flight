
#version 330 core

out vec4 fragcolor;

in vec3 fragpos;
in vec3 normal;

struct DLight {
	vec3 direction;		// directional light direction vector. keep w component 0.0f if vec4
	vec3 diffuse;		// diffuse color
	vec3 ambient;		// ambient color
	vec3 specular;		// specular color
};

uniform vec3 viewpos;
uniform vec3 objcolor;
uniform float spec_intensity;
uniform DLight dlight;	// directional light (ie. the sun)

void main() {
	vec3 norm = normalize(normal);
	float specular_intensity = spec_intensity; //0.1;
	
	// ambient
	vec3 ambient = dlight.ambient;
	
	// diffuse
	vec3 lightdir = normalize(-dlight.direction);
	float diff = max(dot(norm, lightdir), 0.0);
	vec3 diffuse = dlight.diffuse * diff;

	// specular
	vec3 viewdir = normalize(viewpos - fragpos);
	vec3 reflectdir = reflect(-lightdir, norm);
	float spec = pow(max(dot(viewdir, reflectdir), 0.0), 32);
	vec3 specular = specular_intensity * spec * dlight.specular;

	vec3 result = (ambient + diffuse + specular) * objcolor;
	fragcolor = vec4(result, 1.0);
}