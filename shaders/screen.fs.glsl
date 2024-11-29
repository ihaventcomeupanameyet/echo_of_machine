#version 330

uniform sampler2D screen_texture;
uniform float fade_in_factor;
uniform float darken_screen_factor;
uniform float nighttime_factor;
uniform vec2 spotlight_center;
uniform float spotlight_radius;
uniform vec2 glow_center;
uniform float glow_radius;
uniform float glow_intensity;
uniform float time;

in vec2 texcoord;

layout(location = 0) out vec4 color;

vec4 apply_fade_and_darken(vec4 in_color) {
    if (darken_screen_factor > 0.0) {
        in_color = mix(in_color, vec4(0.1, 0.1, 0.1, 1.0), darken_screen_factor);
    }
    if (fade_in_factor > 0.0) {
        in_color = mix(vec4(0.0, 0.0, 0.0, 1.0), in_color, 1.0 - fade_in_factor);
    }
    return in_color;
}

vec4 apply_nighttime_with_diffused_spotlight(vec4 original_color, vec4 nighttime_color, vec2 uv) {
    if (nighttime_factor > 0.0) {
        vec2 spotlight_uv = uv - spotlight_center;
        spotlight_uv.y *= 0.7;
        float distance_to_center = length(spotlight_uv);
        float inner_radius = spotlight_radius * 0.2;
        float outer_radius = spotlight_radius * 0.6;
        float spotlight_effect = smoothstep(inner_radius, outer_radius, distance_to_center);
        vec4 blended_color = mix(original_color, nighttime_color, nighttime_factor);
        return mix(blended_color, original_color, 1.0 - spotlight_effect);
    }
    return original_color;
}
vec4 apply_glow(vec4 original_color, vec2 uv) {
    vec2 glow_uv = uv - glow_center;
    float distance_to_glow = length(glow_uv);

    // Create a wavy effect using sine function
    float wave = 0.5 * sin(time * 2.0) + 0.5; // Oscillates between 0 and 1
    float dynamic_radius = glow_radius * (1.0 + 0.2 * wave); // Slightly modulate radius
    float glow_effect = glow_intensity * exp(-distance_to_glow / dynamic_radius);

    // Add red glow effect
    vec4 glow_color = vec4(1.0, 0.0, 0.0, 1.0) * glow_effect; // Red color
    return original_color + glow_color;
}
void main() {
    vec4 in_color = texture(screen_texture, texcoord);
    in_color = apply_fade_and_darken(in_color);
    vec4 nighttime_color = vec4(0.05, 0.05, 0.2, 1.0);
    in_color = apply_nighttime_with_diffused_spotlight(in_color, nighttime_color, texcoord);
    in_color = apply_glow(in_color, texcoord);

    color = in_color;
}
