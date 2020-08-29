#version 330 core

layout(location = 0) in vec3 v_position;
layout(location = 1) in uvec2 v_uv;
layout(location = 2) in vec3 v_color;
layout(location = 3) in vec3 v_light;

out vec2 f_uv;
out vec3 f_color;
out vec3 f_light;

uniform mat4 projection;
uniform mat4 view;
uniform uvec2 texture_size;

void main()
{
    gl_Position = projection * view * vec4(v_position, 1);
    f_uv = vec2(v_uv) / vec2(texture_size); // vec2(v_uv.x, -v_uv.y);
    f_color = v_color;
    f_light = v_light;
}
