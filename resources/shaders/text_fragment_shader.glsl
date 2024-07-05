#version 450

in vec2 TexCoords;

uniform sampler2D text;
uniform vec3 textColor;

out vec4 diffuseColor;

void main()
{    
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
    diffuseColor = vec4(textColor, 1.0) * sampled;
} 