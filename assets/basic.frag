#version 330 core

in vec2 f_uv;

uniform sampler2D texture0;

out vec4 color;

void main()
{
    color = texture(texture0, f_uv);
}