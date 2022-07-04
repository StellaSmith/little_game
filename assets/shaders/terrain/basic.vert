#version 330 core

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec3 v_color;
layout(location = 3) in vec3 v_light;
layout(location = 4) in uvec2 v_textures;

out vec2 f_uv;
out vec3 f_color;
out vec3 f_light;
flat out uvec2 f_textures;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    gl_Position = projection * view * vec4(v_position, 1);
    f_uv = v_uv;
    f_color = v_color;
    f_light = v_light;
    f_textures = v_textures;
}
