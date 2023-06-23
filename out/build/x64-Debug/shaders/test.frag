#version 330 core

in vec3 fragPos;
in vec2 fragColor;
in vec3 n;
in vec3 fragLight;
in float fragSun;

uniform vec3 lightPosition;

out vec4 color;
uniform sampler2D sampler;
uniform float diffStrength = 1.0;
uniform float ambient = 0.02;
uniform float specStrength = 0.02;
uniform float shineinessCoefficient = 0.3;

void main() {
	vec3 viewDir = lightPosition - fragPos;
	vec3 lightDir = normalize(fragLight - fragPos);
	vec3 normal = normalize(n);
	float diff = max(dot(lightDir, normal), 0.0);
	diff = diffStrength * diff;
	vec3 r = 2.0f * diff * normal + lightDir;
	float specular = max(dot(viewDir, r), 0.0);
	specular = specStrength * pow(specular, shineinessCoefficient);

	color = texture(sampler, fragColor);
	if (fragSun == 1.0f){
		color = color;
	}
	else {
		color = (diff + ambient + specular) * color;
	}
}
