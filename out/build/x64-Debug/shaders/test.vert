#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 color;
layout (location = 2) in vec3 normal;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;
uniform mat3 Norm;
uniform vec3 light = vec3(0.0f, 0.0f, 0.0f);
uniform float sun;
uniform vec3 center;

out vec3 fragPos;
out vec2 fragColor;
out vec3 n;
out vec3 fragLight;
out float fragSun;

void main() {
	fragSun = sun;
	fragLight = light;
	fragPos = vec3(M * vec4(pos, 1.0));
	fragColor = color;
	//n = Norm * normal;
	n = fragPos - vec3(M * vec4(center, 1.0));
	gl_Position = P * V * M * vec4(pos, 1.0);
}
