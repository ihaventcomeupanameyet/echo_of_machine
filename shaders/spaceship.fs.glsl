#version 330

// From Vertex Shader
in vec3 vcolor;
in vec2 vpos; // distance from origin

// Application data
uniform sampler2D sampler0;
uniform vec3 fcolor;
uniform int light_up;

// Output color
layout(location = 0) out vec4 color;

void main()
{
    // Base color
    color = vec4(fcolor * vcolor, 1.0);

    // Spacecraft-specific glow effect
    float radius = distance(vec2(0.0), vpos);
    if (light_up == 1 && radius < 0.25)
    {
        color.xyz += (0.25 - radius) * 0.6 * vec3(0.0, 0.5, 1.0);
    }
}
