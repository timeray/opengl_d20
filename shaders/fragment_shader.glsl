#version 450

in vec3 vColor;
out vec4 diffuseColor;

void main() {
	diffuseColor = vec4(vColor, 1.0);
}
