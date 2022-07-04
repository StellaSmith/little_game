#version 330 core

in vec2 f_uv;
in vec3 f_color;
in vec3 f_light;
flat in uvec2 f_textures;

uniform sampler2DArray texture0;
uniform sampler2DArray texture1;

out vec4 o_color;

void main()
{
    vec4 color;
    vec4 color_mask = texture(texture1, vec3(f_uv, f_textures.y));
    if (color_mask.r > 0.01) {
        color = vec4(color_mask.r * f_color, color_mask.a);
    } else {
        color = texture(texture0, vec3(f_uv, f_textures.x));
        if (color.a < 0.1) discard; // discard (almost) transparent fragment
    }

    color = vec4(f_light.r * color.r, f_light.g * color.g, f_light.b * color.b, color.a);

    o_color = color;
}
