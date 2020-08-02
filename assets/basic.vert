#version 330 core

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec2 v_uv;

out vec2 f_uv;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    gl_Position = projection * view * vec4(v_position, 1);
    f_uv = v_uv; // vec2(v_uv.x, -v_uv.y);
}
