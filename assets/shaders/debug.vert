#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

uniform mat4 VP; // View-Projection matrix

out vec4 vertexColor;

void main() {
    gl_Position = VP * vec4(position, 1.0);
    vertexColor = color;
}