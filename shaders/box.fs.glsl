#version 330 core
out vec4 FragColor;


uniform vec3 input_col;

void main() {
    FragColor = vec4(input_col*vec3(1.0, 1.0, 1.0), 1.0);  
}