#version 330 core

// Input attributes
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 tex_coord;
layout(location = 3) in vec3 normal;

// Transformation matrices
uniform mat4 transform;  // MVP matrix
uniform mat4 model;      // Model matrix
uniform mat4 modelIT;    // Model inverse transpose for normals

// Camera position
uniform vec3 eye;

// Output to fragment shader
out Vertex {
    vec4 color;
    vec2 tex_coord;
    vec3 normal;        // World-space normal
    vec3 view;          // View direction (eye - worldPos)
    vec3 world_position; // World-space position
} vs_out;

void main() {
    // Transform position to world space
    vec3 world_position = (model * vec4(position, 1.0)).xyz;
    vs_out.world_position = world_position;
    
    // Transform normal to world space (using inverse transpose)
    vs_out.normal = normalize((modelIT * vec4(normal, 0.0)).xyz);
    
    // Calculate view direction
    vs_out.view = normalize(eye - world_position);
    
    // Pass through other attributes
    vs_out.color = color;
    vs_out.tex_coord = tex_coord;
    
    // Final position
    gl_Position = transform * vec4(position, 1.0);
}