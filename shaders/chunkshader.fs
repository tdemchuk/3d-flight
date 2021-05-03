
#version 330 core

out vec4 fragcolor;

in vec3 normal;
in vec3 fragpos;
in vec2 texcoord;

struct DLight {
	vec3 direction;		// directional light direction vector. keep w component 0.0f if vec4
	vec3 diffuse;
	vec3 ambient;
	vec3 specular;
};

uniform vec3 viewpos;
uniform sampler2D grasstex;
uniform sampler2D sandtex;
uniform sampler2D stonetex;
uniform DLight dlight;	// directional light (ie. the sun)

void main() {
	// compute fragment normal 
	vec3 norm = normalize(normal);
	
	// compute fragment ambient component
	vec3 ambient = dlight.ambient;
	
	// compute fragment diffuse component
	vec3 lightdir = normalize(-dlight.direction);
	float diff = max(dot(norm, lightdir), 0.0);
	vec3 diffuse = dlight.diffuse * diff;
	
	// compute combined result
	vec3 result = (ambient + diffuse);
	if (fragpos.y < 0.3f) result = result * texture(sandtex, texcoord).rgb;			// sand
	else if (fragpos.y < 15) result = result * vec3(0.0f, 0.8f, 0.1f) * texture(grasstex, texcoord).rgb;		// grass
	else result = result * texture(stonetex, texcoord).rgb;					// mountain
	fragcolor = vec4(result, 1.0);
}