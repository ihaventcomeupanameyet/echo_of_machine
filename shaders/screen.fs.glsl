#version 330

uniform sampler2D screen_texture;
uniform float time;
uniform float darken_screen_factor;
uniform float fade_in_factor;  // Uniform for fade-in effect

in vec2 texcoord;

layout(location = 0) out vec4 color;

vec2 distort(vec2 uv) {
    // You can add distortion here if needed
    return uv;
}

vec4 color_shift(vec4 in_color) {
    // You can add color shifting logic here if needed
    return in_color;
}

vec4 fade_color(vec4 in_color) 
{
    // Apply darken effect (on death) using linear interpolation for smoothness
    if (darken_screen_factor > 0)
        in_color = mix(in_color, vec4(0.2, 0.2, 0.2, 1.0), darken_screen_factor); // Darken smoothly towards darker color
    
    // Apply fade-in effect using linear interpolation
    if (fade_in_factor > 0)
        in_color = mix(vec4(0.0, 0.0, 0.0, 1.0), in_color, 1.0 - fade_in_factor); // Fade from black to in_color
    
    return in_color;
}

void main() {
    vec2 coord = distort(texcoord);
    vec4 in_color = texture(screen_texture, coord);
    color = color_shift(in_color);
    color = fade_color(color);
}
