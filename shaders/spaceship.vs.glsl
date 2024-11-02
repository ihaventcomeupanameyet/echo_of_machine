#version 330 core

// Input attributes
in vec3 in_position;
in vec3 in_normal;

// Outputs to the fragment shader
out vec3 vFragPos;
out vec3 vNormal;

// Application data
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // Compute position and normal in world coordinates
    vFragPos = vec3(model * vec4(in_position, 1.0));
    vNormal = mat3(transpose(inverse(model))) * in_normal;

    // Final transformed position for rendering
    gl_Position = projection * view * vec4(vFragPos, 1.0);
}

