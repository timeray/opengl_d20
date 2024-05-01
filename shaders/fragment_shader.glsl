#version 450

in vec3 fColor;
in vec3 fPos;
in vec3 vNormalModelView;

uniform vec3 lightDirection;
uniform float ambientBrightness;
uniform float directBrightness;
uniform float specularBrightness;

out vec4 diffuseColor;

void main() {
	float diffuseIntensity = directBrightness * max(dot(lightDirection, vNormalModelView), 0.0);

	vec3 reflectDirection = reflect(lightDirection, vNormalModelView);
	vec3 viewDirection = normalize(fPos);
	float specularIntensity = pow(max(dot(reflectDirection, viewDirection), 0.0), 1024);
	specularIntensity *= specularBrightness;

	vec3 resultColor = min((ambientBrightness + diffuseIntensity + specularIntensity) * fColor, 1.0);

	diffuseColor = vec4(resultColor, 1.0);
}
