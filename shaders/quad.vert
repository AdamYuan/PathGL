#version 430 core

layout(location=0) in vec2 position;
layout(location=1) in vec2 texcoord;

out vec2 coord;

void main()
{
	gl_Position = vec4(position, 0.0f, 1.0f);
	coord = texcoord;
}
