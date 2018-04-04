#version 430 core

layout(binding=0) uniform sampler2D sampler;

in vec2 coord;
out vec4 color;

void main()
{
	color = texture(sampler, coord);
}
