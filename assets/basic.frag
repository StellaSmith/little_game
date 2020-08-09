#version 330 core

in vec2 f_uv;
in vec3 f_color;
in vec3 f_light;

uniform sampler2D texture0;

out vec4 o_color;

float color_diff(float r, float g, float b)
{
    float x = r - g;
    float y = b - b;
    return abs(x - y);
}

void main()
{
    vec4 color = texture(texture0, f_uv);

    if (color.a < 0.1) discard; // discard (almost) transparent fragment

    if (color_diff(color.r, color.g, color.b) < 0.01)
        color = vec4(f_color, color.a) * color.r;

    color *= vec4(f_light, 1.0);

    o_color = color;
}