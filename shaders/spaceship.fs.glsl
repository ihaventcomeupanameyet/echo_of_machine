#version 330 core

// Input from vertex shader
in vec3 vFragPos;
in vec3 vNormal;

// Output color
out vec4 FragColor;

// Light and material properties
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 ambientColor;  // Material ambient (Ka)
uniform vec3 diffuseColor;  // Material diffuse (Kd)
uniform vec3 specularColor; // Material specular (Ks)
uniform float shininess;    // Material shininess (Ns)

void main()
{
    // Ambient lighting
    vec3 ambient = ambientColor;

    // Diffuse lighting
    vec3 norm = normalize(vNormal);
    vec3 lightDir = normalize(lightPos - vFragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diffuseColor * diff;

    // Specular lighting
    vec3 viewDir = normalize(viewPos - vFragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularColor * spec;

    // Combine results for final fragment color
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
