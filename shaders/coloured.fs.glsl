#version 330 core

in vec2 frag_texcoord;    // Incoming texture coordinates
uniform vec3 fcolor;      // Color uniform
out vec4 color;           // Output color

void main()
{
    color = vec4(fcolor, 1.0);  // Use the fcolor uniform for the output color
}
