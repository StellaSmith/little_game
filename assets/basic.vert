#version 330 core

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec4 v_color;

out vec4 f_color;

void main() {
    gl_Position = vec4(v_position, 1);
    f_color = v_color;
}