#version 430 core

layout(binding=0) uniform sampler2D sampler;

in vec2 coord;
out vec4 color;

void main()
{
	vec3 color3 = pow(texture(sampler, coord).xyz, vec3(1.0f / 2.2f));
	color = vec4(color3, 1.0f);
}
