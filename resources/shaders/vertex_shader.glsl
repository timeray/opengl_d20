#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vColor;
layout (location = 2) in vec3 vNormal;
layout (location = 3) in vec2 vTextureCoord;

uniform mat4 model;
uniform mat3 normalMatrix;
uniform mat4 view;
uniform mat4 projection;

out vec3 fColor;
out vec3 vNormalModelView;
out vec3 fPos;
out vec2 fTextureCoord;

void main() {
	vNormalModelView = normalize(normalMatrix * vNormal);

	// Calculate position for fragment shader in after Model->View transformation
	vec4 vPositionModelView = view * model * vec4(vPosition, 1.0);
	fPos = vPositionModelView.xyz;

	gl_Position = projection * vPositionModelView;
	fColor = vColor;
	fTextureCoord = vTextureCoord;
}
